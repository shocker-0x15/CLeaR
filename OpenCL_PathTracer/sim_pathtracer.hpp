#include "sim_global.hpp"
#include "sim_bvh_traversal.hpp"
#include "sim_matrix.hpp"
#include "sim_rng.hpp"
#include "sim_light.hpp"
#include "sim_reflection.hpp"
#include "sim_camera.hpp"

namespace sim {
    void pathtracing(float3* vertices, float3* normals, float3* tangents, float2* uvs, uchar* faces,
                     uint* lights, uint numLights,
                     uchar* materialsData, uchar* texturesData,
                     uchar* BVHNodes,
                     uchar* others,
                     uint* randStates, float3* pixels) {
        Scene scene = {
            vertices, normals, tangents, uvs, (Face*)faces,
            (LightInfo*)lights, numLights,
            materialsData, texturesData,
            (BVHNode*)BVHNodes,
            (CameraHead*)(others + *((uint*)others + 0)),
            (EnvironmentHead*)(others + *((uint*)others + 1)),
            others + *((uint*)others + 2)
        };
        
        const uint gid0 = get_global_id(0);
        const uint gid1 = get_global_id(1);
        const uint gsize0 = get_global_size(0);
        const uint gsize1 = get_global_size(1);
        const uint tileX = gid0 - get_global_offset(0);
        const uint tileY = gid1 - get_global_offset(1);
        uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
        float3* pix = pixels + (scene.camera->width * gid1 + gid0);
        
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
        float initY = luminance(&alpha);
        
        uchar BSDF[256] __attribute__((aligned(16)));
        uchar EDF[256] __attribute__((aligned(16)));
        Intersection isect;
        LightPosition lpos;
        bool traceContinue = true;
        vector3 vout;
        BxDFType sampledType;
        //        if (gid0 >= 0 && gid0 < 32 && gid1 >= 0 && gid1 < 32) {
        //        if (gid0 == 512 && gid1 == 600) {
        if (!rayIntersection(&scene, &ray.org, &ray.dir, &isect))
        return;
        
        const Face* face = &scene.faces[isect.faceID];
        vout = -ray.dir;
        if (face->lightPtr != USHRT_MAX) {
            LightPositionFromIntersection(&isect, &lpos);
            EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
            *pix += alpha * Le(EDF, &vout);
        }
        
        while (true) {
            BSDFAlloc(&scene, face->matPtr, &isect, BSDF);
            
            float MISWeight;
            float dist2;
            float lightPDF;
            
            if (hasNonSpecular(BSDF)) {
                LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                sampleLightPos(&scene, &l_sample, &isect.p, &lpos, &lightPDF);
                EDFAlloc(&scene, scene.faces[lpos.faceID].lightPtr, &lpos, EDF);
                
                vector3 vinL = lpos.p - isect.p;
                dist2 = dot(vinL, vinL);
                vinL = vinL * (1.0f / sqrtf(dist2));
                
                point3 shadowRayOrg = isect.p + vinL * EPSILON;
                Intersection lIsect;
                rayIntersection(&scene, &shadowRayOrg, &vinL, &lIsect);
                if (lIsect.faceID == lpos.faceID) {
                    vector3 vinLrev = -vinL;
                    float fsPDF = fs_pdf(BSDF, &vout, &vinL) * absCosNsEDF(EDF, &vinL) / dist2;
                    MISWeight = (lightPDF * lightPDF) / (lightPDF * lightPDF + fsPDF * fsPDF);
                    *pix += (MISWeight * absCosNsEDF(EDF, &vinL) * absCosNsBSDF(BSDF, &vinL) / dist2 / lightPDF) *
                    (alpha * Le(EDF, &vinLrev) * fs(BSDF, &vout, &vinL));
                }
            }
            
            traceContinue = true;
            
            BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
            color fs = sample_fs(BSDF, &vout, &fs_sample, &ray.dir, &dirPDF, &sampledType);
            if (zeroVec(&fs) || dirPDF == 0.0f) {
                traceContinue = false;
                break;
            }
            alpha *= fs * (absCosNsBSDF(BSDF, &ray.dir) / dirPDF);
            
            ray.org = isect.p + ray.dir * EPSILON;
            ++ray.depth;
            if (!rayIntersection(&scene, &ray.org, &ray.dir, &isect))
            break;
            
            face = &scene.faces[isect.faceID];
            vout = -ray.dir;
            
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
            
            if (ray.depth > 99) {
                traceContinue = false;
                break;
            }
            
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
        //        }
        
        //        if (gid0 == 0 && gid1 == 0) {
        //            printf("Ray %lu\t Face %lu\n", sizeof(Ray), sizeof(Face));
        //            printf("BBox %lu\t BVHNode %lu\n", sizeof(BBox), sizeof(BVHNode));
        //            printf("Intersection %lu\t LightPosition %lu\t LensPosition %lu\t CameraHead %lu\n", sizeof(Intersection), sizeof(LightPosition), sizeof(LensPosition), sizeof(CameraHead));
        //            printf("Scene %lu\n", sizeof(Scene));
        //            printf("\n");
        //            printf("BxDFType %lu\t BSDFSample %lu\t BxDFHead %lu\t BSDFHead %lu\n", sizeof(BxDFType), sizeof(BSDFSample), sizeof(BxDFHead), sizeof(BSDFHead));
        //            printf("Diffuse %lu\t SpecularReflection %lu\t SpecularTransmission %lu\n", sizeof(Diffuse), sizeof(SpecularReflection), sizeof(SpecularTransmission));
        //            printf("NewWard %lu\t AshikhminS %lu\t AshikhminD %lu\n", sizeof(NewWard), sizeof(AshikhminS), sizeof(AshikhminD));
        //            printf("Fresnel Head %lu\t Conductor %lu\t Dielectric %lu\n", sizeof(FresnelHead), sizeof(FresnelConductor), sizeof(FresnelDielectric));
        //            printf("\n");
        //            printf("EEDFType %lu\t LightSample %lu\t EDFSample %lu\n", sizeof(EEDFType), sizeof(LightSample), sizeof(EDFSample));
        //            printf("EEDFHead %lu\t EDFHead %lu\n", sizeof(EEDFHead), sizeof(EDFHead));
        //            printf("DiffuseEmission %lu\n", sizeof(DiffuseEmission));
        //            printf("\n");
        //            printf("CameraSample %lu\t IDFSample %lu\n", sizeof(CameraSample), sizeof(IDFSample));
        //        }
    }
}