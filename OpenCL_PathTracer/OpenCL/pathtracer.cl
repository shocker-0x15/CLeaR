//
//  pathtracer.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "global.cl"
#include "bvh_traversal.cl"
#include "matrix.cl"
#include "rng.cl"
#include "light.cl"
#include "reflection.cl"
#include "camera.cl"

kernel void pathtracing(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs, global uchar* faces,
                        global uint* lights,
                        global uchar* materialsData, global uchar* texturesData, global uchar* otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
                        global uchar* BVHInternalNodes, global uchar* BVHLeafNodes,
#else
                        global uchar* BVHNodes,
#endif
                        global uint* randStates, global float3* pixels);

kernel void pathtracing(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs, global uchar* faces,
                        global uint* lights,
                        global uchar* materialsData, global uchar* texturesData, global uchar* otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
                        global uchar* BVHInternalNodes, global uchar* BVHLeafNodes,
#else
                        global uchar* BVHNodes,
#endif
                        global uint* randStates, global float3* pixels) {
    const Scene scene = {
        vertices, normals, tangents, uvs, (global Face*)faces,
        (global LightInfo*)lights, 
        materialsData, texturesData, otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
        (global BVHInternalNode*)BVHInternalNodes, (global BVHLeafNode*)BVHLeafNodes,
#else
        (global BVHNode*)BVHNodes,
#endif
        (global CameraHead*)(otherResources + *((global uint*)otherResources + 0)),
        (global EnvironmentHead*)(otherResources + *((global uint*)otherResources + 1)),
        (global Discrete1D*)(otherResources + *((global uint*)otherResources + 2))
    };
    
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const uint gsize0 = get_global_size(0);
    //const uint gsize1 = get_global_size(1);
    const uint tileX = gid0 - get_global_offset(0);
    const uint tileY = gid1 - get_global_offset(1);
    global uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
    global float3* pix = pixels + (scene.camera->width * gid1 + gid0);
    
    uchar memPool[512] __attribute__((aligned(16)));

    Ray ray;
    ray.depth = 0;
    float lensPDF, lightPDF, dirPDF;
    float MISWeight;
    
    // レンズ上の点をサンプル、IDFを生成する。
    uchar* IDF = memPool + 0;
    LensPoint lensPos;
    CameraSample c_sample = {{getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
    sampleLensPos(&scene, &c_sample, &lensPos, IDF, &lensPDF);
    ray.org = lensPos.p;
    
    // IDFからレンズへの入射方向をサンプル、パスのウェイトも計算する。
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
//    if (gid0 >= 502 && gid0 < 522 && gid1 >= 480 && gid1 < 500) {
//    if (gid0 == 512 && gid1 == 490) {
    // レイとシーンとの交差判定、交点の情報を取得する。
    if (rayIntersection(&scene, &ray.org, &ray.dir, &surfPt)) {
        // 光源に直接ヒットする1次レイはMISは使用せず値を評価。
        const global Face* face = &scene.faces[surfPt.faceID];
        vout = -ray.dir;
        if (face->lightPtr != USHRT_MAX) {
            LightPointFromSurfacePoint(&surfPt, &lpos);
            EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
            *pix += alpha * Le(EDF, &vout);
        }
        
        // 1次レイの交点のシェーディングおよび2次レイ以降の処理。
        uchar* BSDF = memPool + 0;
        while (true) {
            // 交点情報からBSDFを構築する。
            BSDFAlloc(&scene, face->matPtr, &surfPt, BSDF);
            
            float dist2;
            
            // Next Event Estimation (explicit path)で光源からの寄与をMISを考慮して計算する。
            // まず光源上の位置をサンプル、間に遮蔽物が無い場合はBSDFなどを用いて寄与を計算する。
            // BSDFがスペキュラー成分しか持っていない場合は寄与が取れる確率密度がゼロであるため処理しない。
            if (hasNonSpecular(BSDF)) {
                LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                sampleLightPos(&scene, &l_sample, &surfPt.p, &lpos, EDF, &lightPDF);
                
                vector3 vinL = lpos.p - surfPt.p;
                dist2 = dot(vinL, vinL);
                vinL = vinL * (1.0f / sqrt(dist2));
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
            
            // BSDFから入射方向をサンプルする。
            // サンプル方向のBSDFの値とPDFなどを用いてパスのウェイトを計算。
            BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
            color fs = sample_fs(BSDF, &vout, &fs_sample, &ray.dir, &dirPDF, &sampledType);
            if (zeroVec(&fs) || dirPDF == 0.0f) {
                traceContinue = false;
                break;
            }
            alpha *= fs * (absCosNsBSDF(BSDF, &ray.dir) / dirPDF);
            
            // サンプルした入射方向から新たなレイを生成し、シーンとの交差判定を行う。
            ray.org = surfPt.p + ray.dir * EPSILON;
            ++ray.depth;
            if (!rayIntersection(&scene, &ray.org, &ray.dir, &surfPt))
                break;
            
            face = &scene.faces[surfPt.faceID];
            vout = -ray.dir;
            
            // レイが光源にヒットした場合(implicit path)はMISを考慮して寄与を計算する。
            // スペキュラー成分をサンプルしている場合は特殊処理。
            if (face->lightPtr != USHRT_MAX) {
                LightPointFromSurfacePoint(&surfPt, &lpos);
                EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
                MISWeight = 1.0f;
                if ((sampledType & BxDF_Non_Singular) != 0) {
                    lightPDF = getAreaPDF(&scene, &lpos) * distance2(&surfPt.p, &ray.org) / absCosNsEDF(EDF, &vout);
                    MISWeight = (dirPDF * dirPDF) / (lightPDF * lightPDF + dirPDF * dirPDF);
                }
                *pix += MISWeight * alpha * Le(EDF, &vout);
            }
            
            // パスが規定の長さを超える場合は打ち切る。
            if (ray.depth > 99) {
                traceContinue = false;
                break;
            }
            
            // パススループットを基準にしてロシアンルーレットを行い、トレースを続けるかを決定する。
            float continueProb = fmin(luminance(&alpha) * (1.0f / initY), 1.0f);
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
    // 何にも当たらなかった場合はIBLの寄与をMISを考慮して計算する。
    if (traceContinue) {
        vout = -ray.dir;
        lpos.atInfinity = true;
        float theta, phi;
        dirToPolarYTop(&ray.dir, &theta, &phi);
        lpos.uv = (float2)(phi / (2.0f * M_PI_F), theta / M_PI_F);
        EDFAlloc(&scene, USHRT_MAX, &lpos, EDF);
        MISWeight = 1.0f;
        if ((sampledType & BxDF_Non_Singular) != 0 && ray.depth > 0) {
            lightPDF = getAreaPDF(&scene, &lpos) * 1.0f / absCosNsEDF(EDF, &vout);
            MISWeight = (dirPDF * dirPDF) / (lightPDF * lightPDF + dirPDF * dirPDF);
        }
        *pix += MISWeight * alpha * Le(EDF, &vout);
    }
//    }
}
