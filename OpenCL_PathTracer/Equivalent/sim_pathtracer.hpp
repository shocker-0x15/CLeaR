//
//  sim_pathtracer.hpp
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "sim_global.hpp"
#include "sim_bvh_traversal.hpp"
#include "sim_matrix.hpp"
#include "sim_rng.hpp"
#include "sim_light.hpp"
#include "sim_reflection.hpp"
#include "sim_camera.hpp"

namespace sim {
    void pathtracing(float3* vertices, float3* normals, float3* tangents, float2* uvs, uchar* faces,
                     uint* lights,
                     uchar* materialsData, uchar* texturesData, uchar* otherResources,
#ifdef __USE_LBVH
                     uchar* LBVHInternalNodes, uchar* LBVHLeafNodes,
#else
                     uchar* BVHNodes,
#endif
                     uint* randStates, float3* pixels);
    
    void pathtracing(float3* vertices, float3* normals, float3* tangents, float2* uvs, uchar* faces,
                     uint* lights,
                     uchar* materialsData, uchar* texturesData, uchar* otherResources,
#ifdef __USE_LBVH
                     uchar* LBVHInternalNodes, uchar* LBVHLeafNodes,
#else
                     uchar* BVHNodes,
#endif
                     uint* randStates, float3* pixels) {
        const Scene scene = {
            vertices, normals, tangents, uvs, (Face*)faces,
            (LightInfo*)lights,
            materialsData, texturesData, otherResources,
#ifdef __USE_LBVH
            (LBVHInternalNode*)LBVHInternalNodes, (LBVHLeafNode*)LBVHLeafNodes,
#else
            (BVHNode*)BVHNodes,
#endif
            (CameraHead*)(otherResources + *((uint*)otherResources + 0)),
            (EnvironmentHead*)(otherResources + *((uint*)otherResources + 1)),
            (Discrete1D*)(otherResources + *((uint*)otherResources + 2))
        };
        
        const uint gid0 = get_global_id(0);
        const uint gid1 = get_global_id(1);
        const uint gsize0 = get_global_size(0);
        //const uint gsize1 = get_global_size(1);
        const uint tileX = gid0 - get_global_offset(0);
        const uint tileY = gid1 - get_global_offset(1);
        uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
        float3* pix = pixels + (scene.camera->width * gid1 + gid0);
        
        uchar memPool[512] __attribute__((aligned(16)));
        
        //レンズ上の点をサンプル、IDFを生成してレンズへの入射方向をサンプルする。
        //IDFの値などからパスのウェイトを計算する。
        Ray ray;
        ray.depth = 0;
        float lensPDF;
        float dirPDF;
        float MISWeight;
        float lightPDF;
        uchar* IDF = memPool + 0;
        LensPoint lensPos;
        CameraSample c_sample = {{getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
        sampleLensPos(&scene, &c_sample, &lensPos, IDF, &lensPDF);
        ray.org = lensPos.p;
        IDFSample pixSample = {{gid0 + getFloat0cTo1o(rds), gid1 + getFloat0cTo1o(rds)}};
        color alpha = sample_We(IDF, &pixSample, &ray.dir, &dirPDF);
        alpha *= absCosNsIDF(IDF, &ray.dir) / dirPDF;
        float initY = luminance(&alpha);
        
        uchar* EDF = memPool + 256;
        SurfacePoint surfPt;
        LightPoint lpos;
        bool traceContinue = true;
        vector3 vout;
        BxDFType sampledType = (BxDFType)0;
//        if (gid0 >= 0 && gid0 < 32 && gid1 >= 0 && gid1 < 32) {
//        if (gid0 == 512 && gid1 == 600) {
        //レイとシーンとの交差判定、交点の情報を取得する。
        if (rayIntersection(&scene, &ray.org, &ray.dir, &surfPt)) {
            //光源に直接ヒットする1次レイはMISは使用せず値を評価。
            const Face* face = &scene.faces[surfPt.faceID];
            vout = -ray.dir;
            if (face->lightPtr != USHRT_MAX) {
                LightPointFromSurfacePoint(&surfPt, &lpos);
                EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
                *pix += alpha * Le(EDF, &vout);
            }
            
            //1次レイの交点のシェーディングおよび2次レイ以降の処理。
            uchar* BSDF = memPool + 0;
            while (true) {
                //交点情報からBSDFを構築する。
                BSDFAlloc(&scene, face->matPtr, &surfPt, BSDF);
                
                float dist2;
                
                //MISを用いたNext Event Estimation (explicit path)で光源からの寄与を計算する。
                //まず光源上の位置をサンプル、間に遮蔽物が無い場合はBSDFなどを用いて寄与を計算する。
                //BSDFがスペキュラー成分しか持っていない場合は寄与が取れる確率がゼロであるため処理しない。
                if (hasNonSpecular(BSDF)) {
                    LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                    sampleLightPos(&scene, &l_sample, &surfPt.p, &lpos, EDF, &lightPDF);
                    
                    vector3 vinL = lpos.p - surfPt.p;
                    dist2 = dot(vinL, vinL);
                    vinL = vinL * (1.0f / sqrtf(dist2));
                    if (lpos.atInfinity) {
                        vinL = lpos.p;
                        dist2 = 1.0f;
                    }
                    
                    point3 shadowRayOrg = surfPt.p + vinL * EPSILON;
                    SurfacePoint lSurfPt;
                    bool hit = rayIntersection(&scene, &shadowRayOrg, &vinL, &lSurfPt);
                    if (lSurfPt.faceID == lpos.faceID || (lpos.atInfinity && hit == false)) {
                        vector3 vinLrev = -vinL;
                        float fsPDF = fs_pdf(BSDF, &vout, &vinL) * absCosNsEDF(EDF, &vinL) / dist2;
                        MISWeight = (lightPDF * lightPDF) / (lightPDF * lightPDF + fsPDF * fsPDF);
                        *pix += (MISWeight * absCosNsEDF(EDF, &vinL) * absCosNsBSDF(BSDF, &vinL) / dist2 / lightPDF) *
                        (alpha * Le(EDF, &vinLrev) * fs(BSDF, &vout, &vinL));
                    }
                }
                
                traceContinue = true;
                
                //BSDFから入射方向をサンプルする。
                //サンプル方向のBSDFの値とPDFなどを用いてパスのウェイトを計算。
                BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                color fs = sample_fs(BSDF, &vout, &fs_sample, &ray.dir, &dirPDF, &sampledType);
                if (zeroVec(&fs) || dirPDF == 0.0f) {
                    traceContinue = false;
                    break;
                }
                alpha *= fs * (absCosNsBSDF(BSDF, &ray.dir) / dirPDF);
                
                //サンプルした入射方向から新たなレイを生成し、シーンとの交差判定を行う。
                ray.org = surfPt.p + ray.dir * EPSILON;
                ++ray.depth;
                if (!rayIntersection(&scene, &ray.org, &ray.dir, &surfPt))
                    break;
                
                face = &scene.faces[surfPt.faceID];
                vout = -ray.dir;
                
                //レイが光源にヒットした場合(implicit path)はMISを用いて寄与を計算する。
                //スペキュラー成分をサンプルしている場合は特殊処理。
                if (face->lightPtr != USHRT_MAX) {
                    MISWeight = 1.0f;
                    LightPointFromSurfacePoint(&surfPt, &lpos);
                    EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
                    if ((sampledType & BxDF_Non_Singular) != 0) {
                        lightPDF = getAreaPDF(&scene, &lpos) * distance2(&surfPt.p, &ray.org) / absCosNsEDF(EDF, &vout);
                        MISWeight = (dirPDF * dirPDF) / (lightPDF * lightPDF + dirPDF * dirPDF);
                    }
                    *pix += MISWeight * alpha * Le(EDF, &vout);
                }
                
                //パスが規定の長さを超える場合は打ち切る。
                if (ray.depth > 99) {
                    traceContinue = false;
                    break;
                }
                
                //パススループットを基準にしてロシアンルーレットを行い、トレースを続けるかを決定する。
                float continueProb = fminf(luminance(&alpha) * (1.0f / initY), 1.0f);
                if (getFloat0cTo1o(rds) < continueProb) {
                    alpha *= 1.0f / continueProb;
                    dirPDF *= continueProb;
                }
                else {
                    traceContinue = false;
                    break;
                }
            }
        }
        if (traceContinue) {
            vout = -ray.dir;
            lpos.atInfinity = true;
            float theta, phi;
            dirToPolarYTop(&ray.dir, &theta, &phi);
            lpos.uv = float2(phi / (2.0f * M_PI_F), theta / M_PI_F);
            EDFAlloc(&scene, USHRT_MAX, &lpos, EDF);
            MISWeight = 1.0f;
            if ((sampledType & BxDF_Non_Singular) != 0 && ray.depth > 0) {
                lightPDF = getAreaPDF(&scene, &lpos) * 1.0f / absCosNsEDF(EDF, &vout);
                MISWeight = (dirPDF * dirPDF) / (lightPDF * lightPDF + dirPDF * dirPDF);
            }
            *pix += MISWeight * alpha * Le(EDF, &vout);
        }
//        }
        
//        if (gid0 == 0 && gid1 == 0) {
//            printSize(DDFType);
//            printSize(Ray);
//            printSize(Face);
//            printSize(LightInfo);
//            printSize(BBox);
//            printSize(BVHNode);
//            printSize(DDFHead);
//            printSize(Intersection);
//            printSize(LightPosition);
//            printSize(LensPosition);
//            printSize(CameraHead);
//            printSize(EnvironmentHead);
//            printSize(Scene);
//            
//            printf("\n");
//            printSize(TextureType);
//            printSize(ColorProceduralType);
//            printSize(FloatProceduralType);
//            
//            printf("\n");
//            printSize(MatElem);
//            printSize(LPElem);
//            printSize(EnvLPElem);
//            printSize(MaterialInfo);
//            printSize(DiffuseRElem);
//            printSize(SpecularRElem);
//            printSize(SpecularTElem);
//            printSize(NewWardElem);
//            printSize(AshikhminSElem);
//            printSize(AshikhminDElem);
//            printSize(LightPropertyInfo);
//            printSize(DiffuseLElem);
//            printSize(ImageBasedEnvLElem);
//            
//            printf("\n");
//            printSize(BxDFID);
//            printSize(FresnelID);
//            printSize(BxDFType);
//            printSize(BSDFSample);
//            printSize(BxDFHead);
//            printSize(Diffuse);
//            printSize(FresnelHead);
//            printSize(FresnelConductor);
//            printSize(FresnelDielectric);
//            printSize(SpecularReflection);
//            printSize(SpecularTransmission);
//            printSize(NewWard);
//            printSize(AshikhminS);
//            printSize(AshikhminD);
//            printSize(BSDFHead);
//            
//            printf("\n");
//            printSize(EEDFType);
//            printSize(EEDFID);
//            printSize(EnvEEDFID);
//            printSize(LightSample);
//            printSize(EDFSample);
//            printSize(EDFHead);
//            printSize(EEDFHead);
//            printSize(DiffuseEmission);
//            printSize(EnvEDFHead);
//            printSize(EnvEEDFHead);
//            printSize(EntireSceneEmission);
//            
//            printf("\n");
//            printSize(CameraSample);
//            printSize(IDFSample);
//            printSize(PerspectiveInfo);
//            printSize(PerspectiveIDF);
//            printSize(IDFHead);
//        }
    }
}
