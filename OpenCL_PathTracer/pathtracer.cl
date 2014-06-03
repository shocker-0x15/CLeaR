#include "global.cl"
#include "bvh.cl"
#include "matrix.cl"
#include "rng.cl"
#include "light.cl"
#include "reflection.cl"
#include "camera.cl"

kernel void pathtracing(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs,
                        global uchar* faces, global uint* lights, uint numLights,
                        global uchar* materialsData, global uchar* lightsData, global uchar* texturesData,
                        global uchar* BVHNodes, global uchar* camera,
                        global uint* randStates,
                        global float3* pixels) {
    Scene scene = {vertices, normals, tangents, uvs, (global Face*)faces, lights, numLights,
                   materialsData, lightsData, texturesData, (global BVHNode*)BVHNodes, (global CameraHead*)camera};
    
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const uint gsize0 = get_global_size(0);
    const uint gsize1 = get_global_size(1);
    const uint tileX = gid0 - get_global_offset(0);
    const uint tileY = gid1 - get_global_offset(1);
    global uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
    global float3* pix = pixels + (scene.camera->width * gid1 + gid0);

    Ray ray;
    ray.depth = 0;
    float lensPDF;
    float dirPDF;
    uchar IDF[128] __attribute__((aligned(16)));
    LensPosition lensPos;
    CameraSample c_sample = {{getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
    sampleLensPos(&scene, &c_sample, &lensPos, &lensPDF);
    ray.org = lensPos.p;
    IDFAlloc(&scene, &lensPos, IDF);
    IDFSample pixSample = {{gid0 + getFloat0cTo1o(rds), gid1 + getFloat0cTo1o(rds)}};
    color alpha = sample_We(IDF, &pixSample, &ray.dir, &dirPDF);
    alpha *= absCosNsIDF(IDF, &ray.dir) / dirPDF;
    
    uchar BSDF[256] __attribute__((aligned(16)));
    uchar EDF[256] __attribute__((aligned(16)));
    Intersection isect;
    LightPosition lpos;
    bool traceContinue = true;
    BxDFType sampledType;
//    if (gid0 >= 0 && gid0 < 32 && gid1 >= 0 && gid1 < 32) {
//    if (gid0 == 512 && gid1 == 600) {
    while (rayIntersection(&scene, &ray.org, &ray.dir, &isect)) {
        const global Face* face = &scene.faces[isect.faceID];
        vector3 vout = -ray.dir;
        BSDFAlloc(&scene, face->matPtr, &isect, BSDF);
        
        if (face->lightPtr != USHRT_MAX) {
            LightPositionFromIntersection(&isect, &lpos);
            EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
            float expDirPDF = getAreaPDF(&scene, isect.faceID, isect.uv) * dist2(&isect.p, &ray.org) / absCosNsEDF(EDF, &vout);
            if (isfinite(expDirPDF)) {
                float MISWeight = (dirPDF * dirPDF) / (expDirPDF * expDirPDF + dirPDF * dirPDF);
                if (ray.depth == 0 || (sampledType & BxDF_Specular) == BxDF_Specular)
                    MISWeight = 1.0f;
                
                *pix += MISWeight * (alpha * Le(EDF, &vout));
            }
        }
        
        if (hasNonSpecular(BSDF)) {
            LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
            float areaPDF;
            sampleLightPos(&scene, &l_sample, &isect.p, &lpos, &areaPDF);
            EDFAlloc(&scene, scene.faces[lpos.faceID].lightPtr, &lpos, EDF);
            
            vector3 lightDir = isect.p - lpos.p;
            float dist2 = dot(lightDir, lightDir);
            lightDir = normalize(lightDir);
            vector3 lightDirRev = -lightDir;
            
            point3 shadowRayOrg = lpos.p + lightDir * EPSILON;
            Intersection lIsect;
            rayIntersection(&scene, &shadowRayOrg, &lightDir, &lIsect);
            if (lIsect.faceID == isect.faceID) {
                float impDirPDF = fs_pdf(BSDF, &vout, &lightDirRev);
                if (impDirPDF > 0.0f) {
                    color fsCos = fs(BSDF, &vout, &lightDirRev) * absCosNsBSDF(BSDF, &lightDirRev);
                    color fraction = fsCos / impDirPDF;
                    float impAreaPDF = impDirPDF * absCosNsEDF(EDF, &lightDirRev) / dist2 * fmin(luminance(&fraction), 1.0f);
                    float MISWeight = (areaPDF * areaPDF) / (areaPDF * areaPDF + impAreaPDF * impAreaPDF);
                    *pix += (MISWeight * absCosNsEDF(EDF, &lightDirRev) / dist2 / areaPDF) * (alpha * Le(EDF, &lightDir) * fsCos);
                }
            }
        }
        
        if (ray.depth > 99) {
            traceContinue = false;
            break;
        }
        
        BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
        vector3 vin;
        color fs = sample_fs(BSDF, &vout, &fs_sample, &vin, &dirPDF, &sampledType);
        if (zeroVec(&fs) || dirPDF == 0.0f) {
            traceContinue = false;
            break;
        }
        color fraction = fs * (absCosNsBSDF(BSDF, &vin) / dirPDF);
        float continueProb = fmin(luminance(&fraction), 1.0f);
        if (getFloat0cTo1o(rds) < continueProb) {
            alpha *= fraction / continueProb;
            dirPDF *= continueProb;
            ray.org = isect.p + vin * EPSILON;
            ray.dir = vin;
            ++ray.depth;
            traceContinue = true;
        }
        else {
            traceContinue = false;
            break;
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