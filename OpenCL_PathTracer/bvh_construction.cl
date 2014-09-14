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
kernel void unifyAABBs(const global uchar* AABBs, uint numAABBs, global uchar* mergedAABBs);
kernel void calcMortonCodes(const global uchar* AABBs, uint numPrimitives,
                            point3 minEntireAABB, point3 sizeEntireAABB, uint divLevel,
                            global uint3* mortonCodes, global uint* indices);
uint scan(local ushort* prefixSumNumOne, const uint blockSize, const uint lid);
uint split(local ushort* prefixSumNumOne, const uint blockSize, const uint lid, uint pred);
kernel void blockwiseSort(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                          global uint* indices);


kernel void calcAABBs(const global point3* vertices, const global uchar* faces, uint numFaces,
                      global uchar* AABBs) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numFaces)
        return;
    
    const global Face* face = (const global Face*)faces + gid0;
    point3 p0 = *(vertices + face->p0);
    point3 p1 = *(vertices + face->p1);
    point3 p2 = *(vertices + face->p2);
    AABB box;
    box.min = fmin(p0, fmin(p1, p2));
    box.max = fmax(p0, fmax(p1, p2));
    box.center = (box.min + box.max) * 0.5f;
    *((global AABB*)AABBs + gid0) = box;
}

kernel void unifyAABBs(const global uchar* AABBs, uint numAABBs, global uchar* mergedAABBs) {
    const uint blockSize = 128;
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

//    // Interleaved Addressing
//    for (uint i = 1; i < blockSize; i <<= 1) {
//        if (lid0 % (2 * i) == 0) {
//            lmin[lid0] = fmin(lmin[lid0], lmin[lid0 + i]);
//            lmax[lid0] = fmax(lmax[lid0], lmax[lid0 + i]);
//        }
//        barrier(CLK_LOCAL_MEM_FENCE);
//    }
    
//    // Interleaved Addressing (Non-divergent)
//    for (uint i = 1; i < blockSize; i <<= 1) {
//        uint idx = 2 * i * lid0;
//        if (idx < blockSize) {
//            lmin[idx] = fmin(lmin[idx], lmin[idx + i]);
//            lmax[idx] = fmax(lmax[idx], lmax[idx + i]);
//        }
//        barrier(CLK_LOCAL_MEM_FENCE);
//    }
    
    // Sequential Addressing
    for (uint i = blockSize / 2; i > 0; i >>= 1) {
        if (lid0 < i) {
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

kernel void calcMortonCodes(const global uchar* AABBs, uint numPrimitives,
                            point3 minEntireAABB, point3 sizeEntireAABB, uint divLevel,
                            global uint3* mortonCodes, global uint* indices) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
        
    AABB box = *((const global AABB*)AABBs + gid0);
    
    uint numDiv = 1 << divLevel;
    point3 normPos = (box.center - minEntireAABB) / sizeEntireAABB;
    mortonCodes[gid0] = convert_uint3_rtz(normPos * numDiv);
    indices[gid0] = gid0;
}

uint scan(local ushort* prefixSumNumOne, const uint blockSize, const uint lid) {
    for (uint i = 2; i <= blockSize; i <<= 1) {
        uint idx = (lid + 1) * i - 1;
        if (idx < blockSize)
            prefixSumNumOne[idx] += prefixSumNumOne[idx - (i >> 1)];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    prefixSumNumOne[blockSize - 1] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint i = blockSize; i >= 2; i >>= 1) {
        uint idx1 = (lid + 1) * i - 1;
        if (idx1 < blockSize) {
            uint idx0 = idx1 - (i >> 1);
            uint temp = prefixSumNumOne[idx0];
            prefixSumNumOne[idx0] = prefixSumNumOne[idx1];
            prefixSumNumOne[idx1] += temp;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    return prefixSumNumOne[lid];
}

uint split(local ushort* prefixSumNumOne, const uint blockSize, const uint lid, uint pred) {
    uint trueBefore = scan(prefixSumNumOne, blockSize, lid);
    
    local uint falseTotal;
    if (lid == blockSize - 1)
        falseTotal = blockSize - (trueBefore + pred);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (pred)
        return trueBefore + falseTotal;
    else
        return lid - trueBefore;
}

kernel void blockwiseSort(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                          global uint* indices) {
    const uint blockSize = 128;
    
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    local uchar mortonCodesInBlock[blockSize];
    local ushort indicesInBlock[blockSize];
    local ushort prefixSumNumOne[blockSize];
    
    if (gid0 < numPrimitives) {
        uint idx = indices[gid0];
        uchar3 extracted = convert_uchar3(mortonCodes[idx] >> bitID) & (uchar)(0x01);
        mortonCodesInBlock[lid0] = extracted.x + (extracted.y << 1) + (extracted.z << 2);
    }
    else {
        mortonCodesInBlock[lid0] = 0x07;
    }
    indicesInBlock[lid0] = lid0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
//    if (get_group_id(0) == get_num_groups(0) - 1) {
//        if (gid0 < numPrimitives) {
//            uint idx = indicesInBlock[lid0];
//            printf("b %04u, %u\n", lid0, mortonCodesInBlock[idx]);
//        }
//        else {
//            printf("b %04u %08u, 111(NA, NA, NA)\n", lid0, 0);
//        }
//    }
    
    for (uint axis = 0; axis < 3; ++axis) {
        prefixSumNumOne[lid0] = (mortonCodesInBlock[indicesInBlock[lid0]] >> axis) & 0x01;
        barrier(CLK_LOCAL_MEM_FENCE);
        
        uint dstIdx = split(prefixSumNumOne, blockSize, lid0, prefixSumNumOne[lid0]);
        uint curIdx = indicesInBlock[lid0];
        barrier(CLK_LOCAL_MEM_FENCE);
        
        indicesInBlock[dstIdx] = curIdx;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (gid0 < numPrimitives) {
        uint idx = indices[gid0 + indicesInBlock[lid0] - lid0];
        barrier(CLK_GLOBAL_MEM_FENCE);
        indices[gid0] = idx;
    }
    
//    if (get_group_id(0) == get_num_groups(0) - 1) {
//        if (gid0 < numPrimitives) {
//            uint idx = indices[gid0];
//            uint3 mc = mortonCodes[idx];
//            printf("a %04u, %08u: %#010x %#010x %#010x\n", lid0, idx, mc.x, mc.y, mc.z);
//        }
//        else {
//            printf("a %04u %08u, 111(NA, NA, NA)\n", lid0, 0);
//        }
//    }
}

kernel void calcBlockwiseHistograms(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                                    global uint* indices, global uint* histogram) {
    const uint blockSize = 128;
    const uint gid0 = get_global_id(0);
    
    uint currentMortonCode = 0;
    for (uint i = 0; i < blockSize; ++i) {
        uint3 rawMC = mortonCodes[gid0 * blockSize + i];
    }
}
