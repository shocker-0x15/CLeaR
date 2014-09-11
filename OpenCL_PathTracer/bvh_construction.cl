//
//  bvh_construction.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "extra_atomics.cl"

#define printfF3(f3) printf(#f3": %f, %f, %f\n", (f3).x, (f3).y, (f3).z)
#define printSize(t) printf("sizeof("#t"): %u\n", sizeof(t))
#define convertPtrCG(dstT, ptr, offset) (const global dstT*)((const global uchar*)ptr + (offset))

typedef float3 point3;

//56bytes
typedef struct __attribute__((aligned(4))) {
    uint p0, p1, p2;
    uint vn0, vn1, vn2;
    uint vt0, vt1, vt2;
    uint uv0, uv1, uv2;
    uint alphaTexPtr;
    ushort matPtr, lightPtr;
} Face;

//48bytes
typedef struct __attribute__((aligned(16))) {
    point3 min;
    point3 max;
    point3 center;
} AABB;


kernel void calcAABBs(const global point3* vertices, const global uchar* faces, uint numFaces,
                      global uchar* AABBs);
#define blockSize 64
kernel void mergeAABBs(const global uchar* AABBs, uint numAABBs, global uchar* mergedAABBs);
kernel void LinearBVHConstruction(const global uchar* AABBs, uint numPrimitives,
                                  point3 minEntireAABB, point3 maxEntireAABB, uint divLevel);


kernel void calcAABBs(const global point3* vertices, const global uchar* faces, uint numFaces,
                      global uchar* AABBs) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numFaces)
        return;
    
    global AABB* box = (global AABB*)AABBs + gid0;
    const global Face* face = (const global Face*)faces + gid0;
    point3 p0 = *(vertices + face->p0);
    point3 p1 = *(vertices + face->p1);
    point3 p2 = *(vertices + face->p2);
    point3 min = fmin(p0, fmin(p1, p2));
    point3 max = fmax(p0, fmax(p1, p2));
    box->min = min;
    box->max = max;
    box->center = (box->min + box->max) * 0.5f;
}

kernel void mergeAABBs(const global uchar* AABBs, uint numAABBs, global uchar* mergedAABBs) {
    const global AABB* gAABBs = (const global AABB*)AABBs;
    global AABB* gMergedAABBs = (global AABB*)mergedAABBs;
    
    local point3 lmin[blockSize];
    local point3 lmax[blockSize];
    
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    if (gid0 < numAABBs) {
        lmin[lid0] = (gAABBs + gid0)->min;
        lmax[lid0] = (gAABBs + gid0)->max;
    }
    else {
        lmin[lid0] = (point3)(INFINITY, INFINITY, INFINITY);
        lmax[lid0] = -(point3)(INFINITY, INFINITY, INFINITY);
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (uint i = 1; i < blockSize; i <<= 1) {
        if (lid0 % (2 * i) == 0) {
            lmin[lid0] = fmin(lmin[lid0], lmin[lid0 + i]);
            lmax[lid0] = fmax(lmax[lid0], lmax[lid0 + i]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid0 == 0) {
        AABB mergedAABB;
        mergedAABB.min = lmin[0];
        mergedAABB.max = lmax[0];
        mergedAABB.center = (mergedAABB.min + mergedAABB.max) * 0.5f;
        gMergedAABBs[get_group_id(0)] = mergedAABB;
    }
}

kernel void LinearBVHConstruction(const global uchar* AABBs, uint numPrimitives,
                                  point3 minEntireAABB, point3 maxEntireAABB, uint divLevel) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
        
    AABB box = *((const global AABB*)AABBs + gid0);
    
    uint numDiv = 1 << divLevel;
    point3 normPos = (box.center - minEntireAABB) / (maxEntireAABB - minEntireAABB);
    uint3 mortonCode = (uint3)(normPos * divLevel);
}
