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
uint scan(local ushort* prefixSumNumOne, const uint lid);
uint split(local ushort* prefixSumNumOne, local uint* falseTotal, const uint lid, uint pred);
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

kernel void unifyAABBs(const global uchar* AABBs, uint numAABBs, global uchar* unifiedAABBs) {
    const uint blockSize = 128;
    const global AABB* gAABBs = (const global AABB*)AABBs;
    global AABB* gUnifiedAABBs = (global AABB*)unifiedAABBs;
    
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
        AABB unifiedAABB;
        unifiedAABB.min = lmin[0];
        unifiedAABB.max = lmax[0];
        unifiedAABB.center = (unifiedAABB.min + unifiedAABB.max) * 0.5f;
        gUnifiedAABBs[get_group_id(0)] = unifiedAABB;
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
    mortonCodes[gid0] = min(convert_uint3_rtz(normPos * numDiv), numDiv - 1);
    indices[gid0] = gid0;
}

#define SortSize 128
uint scan(local ushort* prefixSumNumOne, const uint lid) {
    for (uint i = 2; i <= SortSize; i <<= 1) {
        uint idx = (lid + 1) * i - 1;
        if (idx < SortSize)
            prefixSumNumOne[idx] += prefixSumNumOne[idx - (i >> 1)];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0)
        prefixSumNumOne[lid] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint i = SortSize; i >= 2; i >>= 1) {
        uint idx1 = (lid + 1) * i - 1;
        if (idx1 < SortSize) {
            uint idx0 = idx1 - (i >> 1);
            ushort temp = prefixSumNumOne[idx0];
            prefixSumNumOne[idx0] = prefixSumNumOne[idx1];
            prefixSumNumOne[idx1] += temp;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    return prefixSumNumOne[lid];
}

uint split(local ushort* prefixSumNumOne, local uint* falseTotal, const uint lid, uint pred) {
    uint trueBefore = scan(prefixSumNumOne, lid);
    
    if (lid == SortSize - 1)
        *falseTotal = SortSize - (trueBefore + pred);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (pred)
        return trueBefore + *falseTotal;
    else
        return lid - trueBefore;
}

kernel void blockwiseSort(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                          global uint* indices) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    local uchar mortonCodesInBlock[SortSize];
    local ushort indicesInBlock[SortSize];
    local ushort prefixSumNumOne[SortSize];
    local uint falseTotal;
    
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
    
    uint curIdx = indicesInBlock[lid0];
    for (uint axis = 0; axis < 3; ++axis) {
        prefixSumNumOne[lid0] = (mortonCodesInBlock[curIdx] >> axis) & 0x01;
        barrier(CLK_LOCAL_MEM_FENCE);
        
        uint dstIdx = split(prefixSumNumOne, &falseTotal, lid0, prefixSumNumOne[lid0]);
        
        indicesInBlock[dstIdx] = curIdx;
        barrier(CLK_LOCAL_MEM_FENCE);
        curIdx = indicesInBlock[lid0];
    }

    uint idx = (gid0 < numPrimitives) ? indices[gid0 + curIdx - lid0] : UINT_MAX;
    barrier(CLK_GLOBAL_MEM_FENCE);
    if (idx != UINT_MAX)
        indices[gid0] = idx;
    
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
                                    const global uint* indices, uint numBlocks, global uint* histogram) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numBlocks)
        return;
    
    uint i = 0;
    uint count = 0;
    uchar mcOfBucket = 0;
    while (true) {
        uint idxIdx = SortSize * gid0 + i;
        if (i >= SortSize || idxIdx >= numPrimitives) {
            histogram[numBlocks * mcOfBucket + gid0] = count;
            ++mcOfBucket;
            while (mcOfBucket < 8) {
                histogram[numBlocks * mcOfBucket + gid0] = 0;
                ++mcOfBucket;
            }
            break;
        }
        
        uint idx = indices[idxIdx];
        uchar3 extracted = convert_uchar3(mortonCodes[idx] >> bitID) & (uchar)(0x01);
        uchar mc = extracted.x + (extracted.y << 1) + (extracted.z << 2);
        if (mc == mcOfBucket) {
            ++count;
            ++i;
        }
        
        if (mc > mcOfBucket) {
            histogram[numBlocks * mcOfBucket + gid0] = count;
            count = 0;
            ++mcOfBucket;
        }
    }
}
