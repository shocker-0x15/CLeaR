//
//  bvh_visualizer.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "global.cl"
#include "bvh_traversal.cl"
//#include "matrix.cl"
//#include "rng.cl"
//#include "light.cl"
#include "reflection.cl"
#include "camera.cl"

kernel void bvh_visualizer(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs, global uchar* faces,
                           global uint* lights,
                           global uchar* materialsData, global uchar* texturesData, global uchar* otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
                           global uchar* BVHInternalNodes, global uchar* BVHLeafNodes,
#else
                           global uchar* BVHNodes,
#endif
                           global uint* randStates, global float3* pixels);

kernel void bvh_visualizer(global float3* vertices, global float3* normals, global float3* tangents, global float2* uvs, global uchar* faces,
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
    
    uchar memPool[256] __attribute__((aligned(16)));
    
    uint numAABBHit = 0;
    Ray ray;
    ray.depth = 0;
    float lensPDF, dirPDF;
    
    uchar* IDF = memPool + 0;
    LensPoint lensPos;
    CameraSample c_sample = {{0.5f, 0.5f}};
    sampleLensPos(&scene, &c_sample, &lensPos, IDF, &lensPDF);
    ray.org = lensPos.p;
    
    IDFSample pixSample = {{gid0 + 0.5f, gid1 + 0.5f}};
    color alpha = sample_We(IDF, &pixSample, &ray.dir, &dirPDF);
    
    SurfacePoint surfPt;
    // レイとシーンとの交差判定、交点の情報を取得する。
    if (rayIntersectionVis(&scene, &ray.org, &ray.dir, &surfPt, &numAABBHit)) {
        const global Face* face = &scene.faces[surfPt.faceID];
        
        uchar* BSDF = memPool + 0;
        BSDFAlloc(&scene, face->matPtr, &surfPt, BSDF);
        
        *pix += alpha * getBaseColor_fs(BSDF);
    }
    
    *pix += numAABBHit * 0.025f;
}
