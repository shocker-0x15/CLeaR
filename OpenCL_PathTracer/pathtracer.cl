#include "global.cl"
#include "bvh_traversal.cl"
#include "matrix.cl"
#include "rng.cl"
#include "light.cl"
#include "reflection.cl"
#include "camera.cl"

kernel void pathtracing(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs, global uchar* faces,
                        global uint* lights, uint numLights,
                        global uchar* materialsData, global uchar* texturesData,
                        global uchar* BVHNodes,
                        global uchar* others,
                        global uint* randStates, global float3* pixels) {
    const Scene scene = {
        vertices, normals, tangents, uvs, (global Face*)faces,
        (global LightInfo*)lights, numLights,
        materialsData, texturesData,
        (global BVHNode*)BVHNodes,
        (global CameraHead*)(others + *((global uint*)others + 0)),
        (global EnvironmentHead*)(others + *((global uint*)others + 1)),
        others + *((global uint*)others + 2)
    };
    
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const uint gsize0 = get_global_size(0);
    const uint gsize1 = get_global_size(1);
    const uint tileX = gid0 - get_global_offset(0);
    const uint tileY = gid1 - get_global_offset(1);
    global uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
    global float3* pix = pixels + (scene.camera->width * gid1 + gid0);

    //レンズ上の点をサンプル、IDFを生成してレンズへの入射方向をサンプルする。
    //IDFの値などからパスのウェイトを計算する。
    Ray ray;
    ray.depth = 0;
    float lensPDF;
    float dirPDF;
    uchar BSDF_IDF[256] __attribute__((aligned(16)));
    LensPosition lensPos;
    CameraSample c_sample = {{getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
    sampleLensPos(&scene, &c_sample, &lensPos, &lensPDF);
    ray.org = lensPos.p;
    IDFAlloc(&scene, &lensPos, BSDF_IDF);
    IDFSample pixSample = {{gid0 + getFloat0cTo1o(rds), gid1 + getFloat0cTo1o(rds)}};
    color alpha = sample_We(BSDF_IDF, &pixSample, &ray.dir, &dirPDF);
    alpha *= absCosNsIDF(BSDF_IDF, &ray.dir) / dirPDF;
    float initY = luminance(&alpha);
    
    uchar EDF[256] __attribute__((aligned(16)));
    Intersection isect;
    LightPosition lpos;
    bool traceContinue = true;
    vector3 vout;
    BxDFType sampledType;
//    if (gid0 >= 0 && gid0 < 32 && gid1 >= 0 && gid1 < 32) {
//    if (gid0 == 512 && gid1 == 130) {
    //レイとシーンとの交差判定、交点の情報を取得する。
    if (rayIntersection(&scene, &ray.org, &ray.dir, &isect)) {
        //光源に直接ヒットする1次レイはMISは使用せず値を評価。
        const global Face* face = &scene.faces[isect.faceID];
        vout = -ray.dir;
        if (face->lightPtr != USHRT_MAX) {
            LightPositionFromIntersection(&isect, &lpos);
            EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
            *pix += alpha * Le(EDF, &vout);
        }
        
        //1次レイの交点のシェーディングおよび2次レイ以降の処理。
        while (true) {
            //交点情報からBSDFを構築する。
            BSDFAlloc(&scene, face->matPtr, &isect, BSDF_IDF);
            
            float MISWeight;
            float dist2;
            float lightPDF;
            
            //MISを用いたNext Event Estimation (explicit path)で光源からの寄与を計算する。
            //まず光源上の位置をサンプル、間に遮蔽物が無い場合はBSDFなどを用いて寄与を計算する。
            //BSDFがスペキュラー成分しか持っていない場合は寄与が取れる確率がゼロであるため処理しない。
            if (hasNonSpecular(BSDF_IDF)) {
                LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                sampleLightPos(&scene, &l_sample, &isect.p, &lpos, &lightPDF);
                EDFAlloc(&scene, scene.faces[lpos.faceID].lightPtr, &lpos, EDF);
                
                vector3 vinL = lpos.p - isect.p;
                dist2 = dot(vinL, vinL);
                vinL = vinL * (1.0f / sqrt(dist2));
                
                point3 shadowRayOrg = isect.p + vinL * EPSILON;
                Intersection lIsect;
                rayIntersection(&scene, &shadowRayOrg, &vinL, &lIsect);
                if (lIsect.faceID == lpos.faceID) {
                    vector3 vinLrev = -vinL;
                    float fsPDF = fs_pdf(BSDF_IDF, &vout, &vinL) * absCosNsEDF(EDF, &vinL) / dist2;
                    MISWeight = (lightPDF * lightPDF) / (lightPDF * lightPDF + fsPDF * fsPDF);
                    *pix += (MISWeight * absCosNsEDF(EDF, &vinL) * absCosNsBSDF(BSDF_IDF, &vinL) / dist2 / lightPDF) *
                    (alpha * Le(EDF, &vinLrev) * fs(BSDF_IDF, &vout, &vinL));
                }
            }

            traceContinue = true;
            
            //BSDFから入射方向をサンプルする。
            //サンプル方向のBSDFの値とPDFなどを用いてパスのウェイトを計算。
            BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
            color fs = sample_fs(BSDF_IDF, &vout, &fs_sample, &ray.dir, &dirPDF, &sampledType);
            if (zeroVec(&fs) || dirPDF == 0.0f) {
                traceContinue = false;
                break;
            }
            alpha *= fs * (absCosNsBSDF(BSDF_IDF, &ray.dir) / dirPDF);
            
            //サンプルした入射方向から新たなレイを生成し、シーンとの交差判定を行う。
            ray.org = isect.p + ray.dir * EPSILON;
            ++ray.depth;
            if (!rayIntersection(&scene, &ray.org, &ray.dir, &isect))
                break;
            
            face = &scene.faces[isect.faceID];
            vout = -ray.dir;
            
            //レイが光源にヒットした場合(implicit path)はMISを用いて寄与を計算する。
            //スペキュラー成分をサンプルしている場合は特殊処理。
            if (face->lightPtr != USHRT_MAX) {
                MISWeight = 1.0f;
                if ((sampledType & BxDF_Non_Singular) != 0) {
                    LightPositionFromIntersection(&isect, &lpos);
                    EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
                    lightPDF = getAreaPDF(&scene, isect.faceID, isect.uv) * distance2(&isect.p, &ray.org) / absCosNsEDF(EDF, &vout);
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
//    }
    
//    if (gid0 == 0 && gid1 == 0) {
//        printf("uchar %u\t ushort %u\t uint %u\t ulong %u\n", sizeof(uchar), sizeof(ushort), sizeof(uint), sizeof(ulong));
//        printf("uchar %u\t uchar2 %u\t uchar3 %u\t uchar4 %u\n", sizeof(uchar), sizeof(uchar2), sizeof(uchar3), sizeof(uchar4));
//        printf("ushort %u\t ushort2 %u\t ushort3 %u\t ushort4 %u\n", sizeof(ushort), sizeof(ushort2), sizeof(ushort3), sizeof(ushort4));
//        printf("float %u\t float2 %u\t float3 %u\t float4 %u\n", sizeof(float), sizeof(float2), sizeof(float3), sizeof(float4));
//        printf("bool %u\t half %u\t uintptr_t %u\n", sizeof(bool), sizeof(half), sizeof(uintptr_t));
//        printf("\n");
//        printf("Ray %u\t Face %u\n", sizeof(Ray), sizeof(Face));
//        printf("BBox %u\t BVHNode %u\n", sizeof(BBox), sizeof(BVHNode));
//        printf("Intersection %u\t LightPosition %u\t LensPosition %u\t CameraHead %u\n", sizeof(Intersection), sizeof(LightPosition), sizeof(LensPosition), sizeof(CameraHead));
//        printf("Scene %u\n", sizeof(Scene));
//        printf("\n");
//        printf("BxDFType %u\t BSDFSample %u\t BxDFHead %u\t BSDFHead %u\n", sizeof(BxDFType), sizeof(BSDFSample), sizeof(BxDFHead), sizeof(BSDFHead));
//        printf("Diffuse %u\t SpecularReflection %u\t SpecularTransmission %u\n", sizeof(Diffuse), sizeof(SpecularReflection), sizeof(SpecularTransmission));
//        printf("NewWard %u\t AshikhminS %u\t AshikhminD %u\n", sizeof(NewWard), sizeof(AshikhminS), sizeof(AshikhminD));
//        printf("Fresnel Head %u\t Conductor %u\t Dielectric %u\n", sizeof(FresnelHead), sizeof(FresnelConductor), sizeof(FresnelDielectric));
//        printf("\n");
//        printf("EEDFType %u\t LightSample %u\t EDFSample %u\n", sizeof(EEDFType), sizeof(LightSample), sizeof(EDFSample));
//        printf("EEDFHead %u\t EDFHead %u\n", sizeof(EEDFHead), sizeof(EDFHead));
//        printf("DiffuseEmission %u\n", sizeof(DiffuseEmission));
//        printf("\n");
//        printf("CameraSample %u\t IDFSample %u\n", sizeof(CameraSample), sizeof(IDFSample));
//    }
}
