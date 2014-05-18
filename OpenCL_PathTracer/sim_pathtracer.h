#include "sim_global.h"
#include "sim_bvh.h"
#include "sim_rng.h"
#include "sim_light.h"
#include "sim_reflection.h"

namespace sim {
    void pathtracing(float3* vertices, float3* normals, float3* tangents, float2* uvs,
                     uchar* faces, uint* lights, uint numLights,
                     uchar* materialsData, uchar* lightsData, uchar* texturesData,
                     uchar* BVHNodes,
                     uint* randStates, uint width, uint height, uint spp,
                     float3* pixels) {
        Scene scene = {vertices, normals, tangents, uvs, (Face*)faces, lights, numLights,
            materialsData, lightsData, texturesData, (BVHNode*)BVHNodes};
        
        const uint gid0 = get_global_id(0);
        const uint gid1 = get_global_id(1);
        const uint gsize0 = get_global_size(0);
        const uint gsize1 = get_global_size(1);
        const uint tileX = gid0 - get_global_offset(0);
        const uint tileY = gid1 - get_global_offset(1);
        uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
        float3* pix = pixels + (width * gid1 + gid0);
        
        for (uint i = 0; i < spp; ++i) {
            float px = gid0 + getFloat0cTo1o(rds);
            float py = gid1 + getFloat0cTo1o(rds);
            
            Ray ray = {point3(0.0f, 0.0f, 3.999f),
                normalize(vector3(-1.0f + 2.0f * px / width,
                                  1.0f - 2.0f * py / height,
                                  -3.0f)), 0};
            
            float dirPDF = 1.0f / (powf(dot(ray.dir, vector3(0, 0, -1)), 3) * (1.0f * 1.0f));
            
            uchar BSDF[256] __attribute__((aligned(16)));
            uchar EDF[256] __attribute__((aligned(16)));
            color alpha = float3(1.0f, 1.0f, 1.0f) / dirPDF * dot(ray.dir, vector3(0, 0, -1));
            Intersection isect;
            LightPosition lpos;
            bool traceContinue = true;
            BxDFType sampledType;
//            if (gid0 > 250 && gid0 <= 260 && gid1 > 250 && gid1 <= 260) {
//            if (gid0 == 340 && gid1 == 512) {
            while (rayIntersection(&scene, &ray.org, &ray.dir, &isect)) {
                const Face* face = &scene.faces[isect.faceID];
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
                float continueProb = fminf(luminance(&fraction), 1.0f);
                if (getFloat0cTo1o(rds) < continueProb) {
                    alpha *= fraction / continueProb;
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
//            }
        }
        
//        if (gid0 == 0 && gid1 == 0) {
//            printf("uchar %lu\t ushort %lu\t uint %lu\t ulong %lu\n", sizeof(uchar), sizeof(ushort), sizeof(uint), sizeof(ulong));
////            printf("uchar %lu\t uchar2 %lu\t uchar3 %lu\t uchar4 %lu\n", sizeof(uchar), sizeof(uchar2), sizeof(uchar3), sizeof(uchar4));
////            printf("ushort %lu\t ushort2 %lu\t ushort3 %lu\t ushort4 %lu\n", sizeof(ushort), sizeof(ushort2), sizeof(ushort3), sizeof(ushort4));
////            printf("float %lu\t float2 %lu\t float3 %lu\t float4 %lu\n", sizeof(float), sizeof(float2), sizeof(float3), sizeof(float4));
////            printf("bool %lu\t half %lu\t uintptr_t %lu\n", sizeof(bool), sizeof(half), sizeof(uintptr_t));
//            printf("Ray %lu\t Face %lu\n", sizeof(Ray), sizeof(Face));
//            printf("BBox %lu\t BVHNode %lu\t Intersection %lu\t LightPosition %lu\n", sizeof(BBox), sizeof(BVHNode), sizeof(Intersection), sizeof(LightPosition));
//            printf("Scene %lu\n", sizeof(Scene));
//            printf("BxDFType %lu\t BSDFSample %lu\t BxDFHead %lu\t BSDFHead %lu\n", sizeof(BxDFType), sizeof(BSDFSample), sizeof(BxDFHead), sizeof(BSDFHead));
//            printf("Diffuse %lu\t SpecularReflection %lu\t SpecularTransmission %lu\n", sizeof(Diffuse), sizeof(SpecularReflection), sizeof(SpecularTransmission));
//            printf("Ward %lu\n", sizeof(Ward));
//            printf("Fresnel Head %lu\t Conductor %lu\t Dielectric %lu\n", sizeof(FresnelHead), sizeof(FresnelConductor), sizeof(FresnelDielectric));
//            printf("EEDFType %lu\t LightSample %lu\t EDFSample %lu\n", sizeof(EEDFType), sizeof(LightSample), sizeof(EDFSample));
//            printf("EEDFHead %lu\t EDFHead %lu\n", sizeof(EEDFHead), sizeof(EDFHead));
//            printf("DiffuseEmission %lu\n", sizeof(DiffuseEmission));
//        }
    }
}