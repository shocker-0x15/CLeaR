#include "global.cl"
#include "bvh.cl"
#include "rng.cl"
#include "light.cl"
#include "reflection.cl"

kernel void pathtracing(global float3* vertices, global float3* normals, global float3* binormals, global float2* uvs,
                        global uchar* faces, global uint* lights, uint numLights,
                        global uchar* materialsData, global uchar* lightsData, global uchar* texturesData,
                        global uchar* BVHNodes,
                        global uint* randStates, uint width, uint height, uint spp,
                        global float3* pixels) {
    Scene scene = {vertices, normals, binormals, uvs, (global Face*)faces, lights, numLights,
                   materialsData, lightsData, texturesData, (global BVHNode*)BVHNodes};
    
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const uint gsize0 = get_global_size(0);
    const uint gsize1 = get_global_size(1);
    const uint tileX = gid0 - get_global_offset(0);
    const uint tileY = gid1 - get_global_offset(1);
    global uint* rds = randStates + 4 * (gsize0 * tileY + tileX);
    global float3* pix = pixels + (width * gid1 + gid0);
    
    for (int i = 0; i < spp; ++i) {
        float px = gid0 + getFloat0cTo1o(rds);
        float py = gid1 + getFloat0cTo1o(rds);
        
        Ray ray = {(point3)(0.0f, 0.0f, 3.999f),
                    normalize((vector3)(-1.0f + 2.0f * px / width,
                                        1.0f - 2.0f * py / height,
                                        -3.0f)), 0};
        
        uchar BSDF[256] __attribute__((aligned(16)));
        uchar EDF[256] __attribute__((aligned(16)));
        color alpha = (float3)(1.0f, 1.0f, 1.0f);
        Intersection isect;
        LightPosition lpos;
        bool enableImplicit = true;
        bool traceContinue = true;
        BxDFType sampledType;
//        if (gid0 > 250 && gid0 <= 260 && gid1 > 250 && gid1 <= 260) {
//        if (gid0 == 500 && gid1 == 512) {
        while (rayIntersection(&scene, &ray.org, &ray.dir, &isect)) {
            const global Face* face = &scene.faces[isect.faceID];
            vector3 vout = -ray.dir;
            BSDFAlloc(&scene, materialsData, face->matPtr, &isect, BSDF);
            
            if (face->lightPtr != USHRT_MAX && enableImplicit) {
                LightPositionFromIntersection(&isect, &lpos);
                EDFAlloc(&scene, lightsData, face->lightPtr, &lpos, EDF);
                
                *pix += alpha * Le(EDF, &vout);
            }
            
            if (hasNonSpecular(BSDF)) {
                LightSample l_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
                float areaPDF;
                sampleLightPos(&scene, &l_sample, &isect.p, &lpos, &areaPDF);
                EDFAlloc(&scene, lightsData, scene.faces[lpos.faceID].lightPtr, &lpos, EDF);
                
                vector3 lightDir = isect.p - lpos.p;
                float dist2 = dot(lightDir, lightDir);
                lightDir = normalize(lightDir);
                vector3 lightDirRev = -lightDir;
                
                point3 shadowRayOrg = lpos.p + lightDir * EPSILON;
                Intersection lIsect;
                rayIntersection(&scene, &shadowRayOrg, &lightDir, &lIsect);
                if (lIsect.faceID == isect.faceID) {
                    *pix += alpha * Le(EDF, &lightDir) * fs(BSDF, &vout, &lightDirRev) * absCosNsBSDF(BSDF, &lightDir) * absCosNsEDF(EDF, &lightDirRev) / dist2;
                }
            }
            
            BSDFSample fs_sample = {getFloat0cTo1o(rds), {getFloat0cTo1o(rds), getFloat0cTo1o(rds)}};
            vector3 vin;
            float dirPDF;
            color fs = sample_fs(BSDF, &vout, &fs_sample, &vin, &dirPDF, &sampledType);
            if (zeroVec(&fs) || dirPDF == 0.0f) {
                traceContinue = false;
                break;
            }
            color fraction = fs * absCosNsBSDF(BSDF, &vin) / dirPDF;
            float continueProb = maxComp(&fraction);
            if (getFloat0cTo1o(rds) < continueProb) {
                alpha *= fraction / continueProb;
                ray.org = isect.p + vin * EPSILON;
                ray.dir = vin;
                ++ray.depth;
                traceContinue = true;
                
                enableImplicit = (bool)(sampledType & BxDF_Specular);
            }
            else {
                traceContinue = false;
                break;
            }
        }
//        }
    }

//    if (gid0 == 0 && gid1 == 0) {
//        printf("sizeof(uchar): %u\n"
//               "sizeof(float): %u\n"
//               "sizeof(float3): %u\n"
//               "sizeof(Vertex): %u\n"
//               "sizeof(Face): %u\n"
//               "sizeof(BBox): %u\n"
//               "sizeof(BVHNode): %u\n"
//               "sizeof(Intersection): %u\n",
//               sizeof(uchar), sizeof(float), sizeof(float3), 
//               sizeof(Vertex), sizeof(Face), sizeof(BBox),
//               sizeof(BVHNode), sizeof(Intersection));
//    }
}