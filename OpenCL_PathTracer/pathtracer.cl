#include "global.cl"
#include "bvh.cl"
#include "rng.cl"
#include "light.cl"
#include "reflection.cl"

kernel void pathtracing(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs,
                        global uchar* faces, global uint* lights, uint numLights,
                        global uchar* materialsData, global uchar* lightsData, global uchar* texturesData,
                        global uchar* BVHNodes,
                        global uint* randStates, uint width, uint height, uint spp,
                        global float3* pixels) {
    Scene scene = {vertices, normals, tangents, uvs, (global Face*)faces, lights, numLights,
                   materialsData, lightsData, texturesData, (global BVHNode*)BVHNodes};
    
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const uint gsize0 = get_global_size(0);
    const uint gsize1 = get_global_size(1);
    const uint tileX = gid0 - get_global_offset(0);
    const uint tileY = gid1 - get_global_offset(1);
    global uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
    global float3* pix = pixels + (width * gid1 + gid0);
    
    for (uint i = 0; i < spp; ++i) {
        float px = gid0 + getFloat0cTo1o(rds);
        float py = gid1 + getFloat0cTo1o(rds);
        
        Ray ray = {(point3)(0.0f, 0.0f, 3.999f),
                    normalize((vector3)(-1.0f + 2.0f * px / width,
                                        1.0f - 2.0f * py / height,
                                        -3.0f)), 0};
        
        float dirPDF = 1.0f / (pow(dot(ray.dir, (vector3)(0, 0, -1)), 3) * (1.0f * 1.0f));
        
        uchar BSDF[256] __attribute__((aligned(16)));
        uchar EDF[256] __attribute__((aligned(16)));
        color alpha = (float3)(1.0f, 1.0f, 1.0f) / dirPDF * dot(ray.dir, (vector3)(0, 0, -1));
        Intersection isect;
        LightPosition lpos;
        bool traceContinue = true;
        BxDFType sampledType;
//        if (gid0 >= 0 && gid0 < 32 && gid1 >= 0 && gid1 < 32) {
//        if (gid0 == 512 && gid1 == 600) {
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
                    if (ray.depth == 0)
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
//        }
    }

//    if (gid0 == 0 && gid1 == 0) {
//        printf("uchar %d\t ushort %d\t uint %d\t ulong %d\n", sizeof(uchar), sizeof(ushort), sizeof(uint), sizeof(ulong));
//        printf("uchar %d\t uchar2 %d\t uchar3 %d\t uchar4 %d\n", sizeof(uchar), sizeof(uchar2), sizeof(uchar3), sizeof(uchar4));
//        printf("ushort %d\t ushort2 %d\t ushort3 %d\t ushort4 %d\n", sizeof(ushort), sizeof(ushort2), sizeof(ushort3), sizeof(ushort4));
//        printf("float %d\t float2 %d\t float3 %d\t float4 %d\n", sizeof(float), sizeof(float2), sizeof(float3), sizeof(float4));
//        printf("bool %d\t half %d\t uintptr_t %d\n", sizeof(bool), sizeof(half), sizeof(uintptr_t));
//        printf("Ray %d\t Face %d\n", sizeof(Ray), sizeof(Face));
//        printf("BBox %d\t BVHNode %d\t Intersection %d\t LightPosition %d\n", sizeof(BBox), sizeof(BVHNode), sizeof(Intersection), sizeof(LightPosition));
//        printf("Scene %d\n", sizeof(Scene));
//        printf("BxDFType %d\t BSDFSample %d\t BxDFHead %d\t BSDFHead %d\n", sizeof(BxDFType), sizeof(BSDFSample), sizeof(BxDFHead), sizeof(BSDFHead));
//        printf("Diffuse %d\t SpecularReflection %d\t SpecularTransmission %d\n", sizeof(Diffuse), sizeof(SpecularReflection), sizeof(SpecularTransmission));
//        printf("Ward %d\n", sizeof(Ward));
//        printf("Fresnel Head %d\t Conductor %d\t Dielectric %d\n", sizeof(FresnelHead), sizeof(FresnelConductor), sizeof(FresnelDielectric));
//        printf("EEDFType %d\t LightSample %d\t EDFSample %d\n", sizeof(EEDFType), sizeof(LightSample), sizeof(EDFSample));
//        printf("EEDFHead %d\t EDFHead %d\n", sizeof(EEDFHead), sizeof(EDFHead));
//        printf("DiffuseEmission %d\n", sizeof(DiffuseEmission));
//    }
}