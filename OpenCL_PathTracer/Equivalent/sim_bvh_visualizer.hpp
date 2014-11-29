//
//  sim_bvh_visualizer.hpp
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "sim_global.hpp"
#include "sim_bvh_traversal.hpp"
//#include "sim_matrix.hpp"
//#include "sim_rng.hpp"
//#include "sim_light.hpp"
#include "sim_reflection.hpp"
#include "sim_camera.hpp"

namespace sim {
    void bvh_visualizer(float3* vertices, float3* normals, float3* tangents, float2* uvs, uchar* faces,
                        uint* lights,
                        uchar* materialsData, uchar* texturesData, uchar* otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
                        uchar* BVHInternalNodes, uchar* BVHLeafNodes,
#else
                        uchar* BVHNodes,
#endif
                        uint* randStates, float3* pixels);
    
    void bvh_visualizer(float3* vertices, float3* normals, float3* tangents, float2* uvs, uchar* faces,
                        uint* lights,
                        uchar* materialsData, uchar* texturesData, uchar* otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
                        uchar* BVHInternalNodes, uchar* BVHLeafNodes,
#else
                        uchar* BVHNodes,
#endif
                        uint* randStates, float3* pixels) {
        const Scene scene = {
            vertices, normals, tangents, uvs, (Face*)faces,
            (LightInfo*)lights,
            materialsData, texturesData, otherResources,
#if defined(__USE_LBVH) || defined(__USE_TRBVH)
            (BVHInternalNode*)BVHInternalNodes, (BVHLeafNode*)BVHLeafNodes,
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
        
        uchar memPool[256] __attribute__((aligned(16)));
        
        uint numAABBHit = 0;
        Ray ray;
        ray.depth = 0;
        float lensPDF;
        float dirPDF;
        uchar* IDF = memPool + 0;
        LensPoint lensPos;
        CameraSample c_sample = {{0.5f, 0.5f}};
        sampleLensPos(&scene, &c_sample, &lensPos, IDF, &lensPDF);
        ray.org = lensPos.p;
        IDFSample pixSample = {{gid0 + 0.5f, gid1 + 0.5f}};
        color alpha = sample_We(IDF, &pixSample, &ray.dir, &dirPDF);
        
        SurfacePoint surfPt;
        //レイとシーンとの交差判定、交点の情報を取得する。
        if (rayIntersectionVis(&scene, &ray.org, &ray.dir, &surfPt, &numAABBHit)) {
            const Face* face = &scene.faces[surfPt.faceID];
            
            uchar* BSDF = memPool + 0;
            BSDFAlloc(&scene, face->matPtr, &surfPt, BSDF);
            
            *pix += alpha * getBaseColor_fs(BSDF);
        }
        
        *pix += numAABBHit * 0.025f;
    }
}
