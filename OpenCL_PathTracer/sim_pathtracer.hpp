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
        const Scene scene = {
            vertices, normals, tangents, uvs, (Face*)faces,
            (LightInfo*)lights, numLights,
            materialsData, texturesData, others,
            (BVHNode*)BVHNodes,
            (CameraHead*)(others + *((uint*)others + 0)),
            (EnvironmentHead*)(others + *((uint*)others + 1)),
            others + *((uint*)others + 2)
        };
        
        const uint gid0 = get_global_id(0);
        const uint gid1 = get_global_id(1);
        const uint gsize0 = get_global_size(0);
        //const uint gsize1 = get_global_size(1);
        const uint tileX = gid0 - get_global_offset(0);
        const uint tileY = gid1 - get_global_offset(1);
        uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
        float3* pix = pixels + (scene.camera->width * gid1 + gid0);
        
        Ray ray;
        ray.depth = 0;
        float lensPDF;
        float dirPDF;
        uchar BSDF_IDF[256] __attribute__((aligned(16)));
        LensPosition lensPos;
        CameraSample c_sample = {{getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
        sampleLensPos(&scene, &c_sample, &lensPos, BSDF_IDF, &lensPDF);
        ray.org = lensPos.p;
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
//        if (gid0 >= 0 && gid0 < 32 && gid1 >= 0 && gid1 < 32) {
//        if (gid0 == 512 && gid1 == 600) {
        if (rayIntersection(&scene, &ray.org, &ray.dir, &isect)) {
            const Face* face = &scene.faces[isect.faceID];
            vout = -ray.dir;
            if (face->lightPtr != USHRT_MAX) {
                LightPositionFromIntersection(&isect, &lpos);
                EDFAlloc(&scene, face->lightPtr, &lpos, EDF);
                *pix += alpha * Le(EDF, &vout);
            }
            
            while (true) {
                BSDFAlloc(&scene, face->matPtr, &isect, BSDF_IDF);
                
                float MISWeight;
                float dist2;
                float lightPDF;
                
                if (hasNonSpecular(BSDF_IDF)) {
                    LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                    sampleLightPos(&scene, &l_sample, &isect.p, &lpos, EDF, &lightPDF);
                    
                    vector3 vinL = lpos.p - isect.p;
                    dist2 = dot(vinL, vinL);
                    vinL = vinL * (1.0f / sqrtf(dist2));
                    
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
                
                BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                color fs = sample_fs(BSDF_IDF, &vout, &fs_sample, &ray.dir, &dirPDF, &sampledType);
                if (zeroVec(&fs) || dirPDF == 0.0f) {
                    traceContinue = false;
                    break;
                }
                alpha *= fs * (absCosNsBSDF(BSDF_IDF, &ray.dir) / dirPDF);
                
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
        }
        if (traceContinue) {
            vout = -ray.dir;
            lpos.atInfinity = true;
            float theta, phi;
            dirToPolarYTop(&ray.dir, &theta, &phi);
            lpos.uv = float2(phi * (1.0f / (2.0f * M_PI_F)), theta * (1.0f / M_PI_F));
            EDFAlloc(&scene, USHRT_MAX, &lpos, EDF);
            *pix += alpha * Le(EDF, &vout) * 50;
            if (pix->hasNeg()) {
                printf("");
                EDFAlloc(&scene, USHRT_MAX, &lpos, EDF);
                Le(EDF, &vout);
            }
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