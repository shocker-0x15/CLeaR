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

//----------------------------------------------------------------

kernel void calcAABBs(const global point3* vertices, const global uchar* faces, uint numFaces,
                      global uchar* AABBs);

kernel void unifyAABBs(const global uchar* AABBs, uint numAABBs, global uchar* mergedAABBs);

kernel void calcMortonCodes(const global uchar* AABBs, uint numPrimitives,
                            point3 minEntireAABB, point3 sizeEntireAABB, uint divLevel,
                            global uint3* mortonCodes, global uint* indices);

ushort scan(local ushort* prefixSumNumOne, const uint lid);
uint split(local ushort* prefixSumNumOne, local ushort* falseTotal, const uint lid, ushort pred);
kernel void blockwiseSort(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                          global uint* indices, global uchar* radixDigits);

kernel void calcBlockwiseHistograms(const global uchar* radixDigits, uint numPrimitives,
                                    uint numBlocks, global uint* histogram, global ushort* localOffsets);

kernel void globalScatter(const global uchar* radixDigits, uint numPrimitives, const global uint* globalOffsets, const global ushort* localOffsets,
                          const global uint* indicesSrc, global uint* indicesDst);

kernel void calcSplitList(const global uint3* mortonCodes, uint bitsPerDim, const global uint* indices, uint numPrimitives,
                          global uint2* splitList);

ushort scanSL(local ushort* prefixSumNumOne, const uint lid);
uint splitSL(local ushort* prefixSumNumOne, local ushort* falseTotal, const uint lid, ushort pred);
kernel void blockwiseSplitListSort(global uint2* splitList, uint listSize, uint bitFrom, uint bitTo,
                                   global uchar* radixDigits);

kernel void calcBlockwiseSplitListHistograms(const global uchar* radixDigits, uint listSize, uint numBuckets,
                                             uint numBlocks, global uint* histogram, global ushort* localOffsets);

kernel void globalScatterSplitList(const global uchar* radixDigits, uint listSize, uint numBuckets, const global uint* globalOffsets, const global ushort* localOffsets,
                                   const global uint2* splitListSrc, global uint2* splitListDst);

//----------------------------------------------------------------

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
                            point3 minEntireAABB, point3 sizeEntireAABB, uint bitsPerDim,
                            global uint3* mortonCodes, global uint* indices) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
        
    AABB box = *((const global AABB*)AABBs + gid0);
    
    uint numDiv = 1 << bitsPerDim;
    point3 normPos = (box.center - minEntireAABB) / sizeEntireAABB;
    mortonCodes[gid0] = min(convert_uint3_rtz(normPos * numDiv), numDiv - 1);
    indices[gid0] = gid0;
}



#define SortSize 128
ushort scan(local ushort* prefixSumNumOne, const uint lid) {
    for (uint i = 2; i <= SortSize; i <<= 1) {
        uint idx = (lid + 1) * i - 1;
        if (idx < SortSize)
            prefixSumNumOne[idx] += prefixSumNumOne[idx - (i >> 1)];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0)
        prefixSumNumOne[SortSize - 1] = 0;
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

uint split(local ushort* prefixSumNumOne, local ushort* falseTotal, const uint lid, ushort pred) {
    ushort trueBefore = scan(prefixSumNumOne, lid);
    
    if (lid == SortSize - 1)
        *falseTotal = SortSize - (trueBefore + pred);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (pred)
        return trueBefore + *falseTotal;
    else
        return lid - trueBefore;
}

kernel void blockwiseSort(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                          global uint* indices, global uchar* radixDigits) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    local uchar mortonCodesInBlock[SortSize];
    local ushort indicesInBlock[SortSize];
    local ushort prefixSumNumOne[SortSize];
    local ushort falseTotal;
    
    if (gid0 < numPrimitives) {
        uint idx = indices[gid0];
        uchar3 extracted = convert_uchar3((mortonCodes[idx] >> bitID) & 0x01);
        mortonCodesInBlock[lid0] = extracted.x + (extracted.y << 1) + (extracted.z << 2);
    }
    else {
        mortonCodesInBlock[lid0] = 0x07;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    uint curIdx = lid0;
    for (uint axis = 0; axis < 3; ++axis) {
        // このprivate変数にコピーしてsplit()との間にバリアを張ることで同期ミスを防ぐ。
        uchar pred = (mortonCodesInBlock[curIdx] >> axis) & 0x01;
        prefixSumNumOne[lid0] = pred;
        barrier(CLK_LOCAL_MEM_FENCE);
        
        uint dstIdx = split(prefixSumNumOne, &falseTotal, lid0, pred);
        
        indicesInBlock[dstIdx] = curIdx;
        barrier(CLK_LOCAL_MEM_FENCE);
        curIdx = indicesInBlock[lid0];
    }

    uint idx = (gid0 < numPrimitives) ? indices[(gid0 - lid0) + curIdx] : UINT_MAX;
    barrier(CLK_GLOBAL_MEM_FENCE);
    if (idx != UINT_MAX) {
        indices[gid0] = idx;
        radixDigits[gid0] = mortonCodesInBlock[curIdx];
    }
}



kernel void calcBlockwiseHistograms(const global uchar* radixDigits, uint numPrimitives, 
                                    uint numBlocks, global uint* histogram, global ushort* localOffsets) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numBlocks)
        return;
    
    uint i = 0;
    uint count = 0;
    uchar mcOfBucket = 0;
    localOffsets[8 * gid0 + 0] = 0;
    while (true) {
        uint idx = SortSize * gid0 + i;
        if (i >= SortSize || idx >= numPrimitives) {
            histogram[numBlocks * mcOfBucket + gid0] = count;
            ++mcOfBucket;
            while (mcOfBucket < 8) {
                localOffsets[8 * gid0 + mcOfBucket] = i;
                histogram[numBlocks * mcOfBucket + gid0] = 0;
                ++mcOfBucket;
            }
            break;
        }
        
        uchar mc = radixDigits[idx];
        if (mc == mcOfBucket) {
            ++count;
            ++i;
        }
        
        if (mc > mcOfBucket) {
            histogram[numBlocks * mcOfBucket + gid0] = count;
            count = 0;
            ++mcOfBucket;
            localOffsets[8 * gid0 + mcOfBucket] = i;
        }
    }
}



kernel void globalScatter(const global uchar* radixDigits, uint numPrimitives, const global uint* globalOffsets, const global ushort* localOffsets,
                          const global uint* indicesSrc, global uint* indicesDst) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    if (gid0 >= numPrimitives)
        return;
    
    uint idxSrc = indicesSrc[gid0];
    uchar digit = radixDigits[gid0];
    uint gOffset = globalOffsets[get_num_groups(0) * digit + get_group_id(0)];
    uint lOffset = localOffsets[8 * get_group_id(0) + digit];
    indicesDst[gOffset + (lid0 - lOffset)] = idxSrc;
}



kernel void calcSplitList(const global uint3* mortonCodes, uint bitsPerDim, const global uint* indices, uint numPrimitives,
                          global uint2* splitList) {
    const uint gid0 = get_global_id(0);
    
    if (gid0 >= numPrimitives - 1)
        return;
    
    uint idx0 = indices[gid0];
    uint idx1 = indices[gid0 + 1];
    uint3 diffPosFromMSB = clz(mortonCodes[idx0] ^ mortonCodes[idx1]) - (32 - bitsPerDim);
    uint diffLevel = min(diffPosFromMSB.z * 3 + 0, min(diffPosFromMSB.y * 3 + 1, diffPosFromMSB.x * 3 + 2));
    
    splitList[gid0] = (uint2)(diffLevel, gid0);
}



#define SplitListSortSize 128
ushort scanSL(local ushort* prefixSumNumOne, const uint lid) {
    for (uint i = 2; i <= SplitListSortSize; i <<= 1) {
        uint idx = (lid + 1) * i - 1;
        if (idx < SplitListSortSize)
            prefixSumNumOne[idx] += prefixSumNumOne[idx - (i >> 1)];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0)
        prefixSumNumOne[SplitListSortSize - 1] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint i = SplitListSortSize; i >= 2; i >>= 1) {
        uint idx1 = (lid + 1) * i - 1;
        if (idx1 < SplitListSortSize) {
            uint idx0 = idx1 - (i >> 1);
            ushort temp = prefixSumNumOne[idx0];
            prefixSumNumOne[idx0] = prefixSumNumOne[idx1];
            prefixSumNumOne[idx1] += temp;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    return prefixSumNumOne[lid];
}

uint splitSL(local ushort* prefixSumNumOne, local ushort* falseTotal, const uint lid, ushort pred) {
    ushort trueBefore = scan(prefixSumNumOne, lid);
    
    if (lid == SplitListSortSize - 1)
        *falseTotal = SplitListSortSize - (trueBefore + pred);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (pred)
        return trueBefore + *falseTotal;
    else
        return lid - trueBefore;
}

kernel void blockwiseSplitListSort(global uint2* splitList, uint listSize, uint bitFrom, uint bitTo,
                                   global uchar* radixDigits) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    local uchar splitDepthsInBlock[SplitListSortSize];
    local ushort indicesInBlock[SplitListSortSize];
    local ushort prefixSumNumOne[SplitListSortSize];
    local ushort falseTotal;
    
    if (gid0 < listSize)
        splitDepthsInBlock[lid0] = (splitList[gid0].s0 >> bitFrom) & ((1 << (bitTo - bitFrom + 1)) - 1);
    else
        splitDepthsInBlock[lid0] = 0xFF;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    uint curIdx = lid0;
    for (uint bit = 0; bit <= (bitTo - bitFrom); ++bit) {
        // このprivate変数にコピーしてsplit()との間にバリアを張ることで同期ミスを防ぐ。
        uchar pred = (splitDepthsInBlock[curIdx] >> bit) & 0x01;
        prefixSumNumOne[lid0] = pred;
        barrier(CLK_LOCAL_MEM_FENCE);
        
        uint dstIdx = splitSL(prefixSumNumOne, &falseTotal, lid0, pred);
        
        indicesInBlock[dstIdx] = curIdx;
        barrier(CLK_LOCAL_MEM_FENCE);
        curIdx = indicesInBlock[lid0];
    }
    
    bool valid = gid0 < listSize;
    uint2 splitLine;
    if (valid)
        splitLine = splitList[(gid0 - lid0) + curIdx];
    barrier(CLK_GLOBAL_MEM_FENCE);
    if (valid) {
        splitList[gid0] = splitLine;
        radixDigits[gid0] = splitDepthsInBlock[curIdx];
    }
}



kernel void calcBlockwiseSplitListHistograms(const global uchar* radixDigits, uint listSize, uint numBuckets,
                                             uint numBlocks, global uint* histogram, global ushort* localOffsets) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numBlocks)
        return;
    
    uint i = 0;
    uint count = 0;
    uchar mcOfBucket = 0;
    localOffsets[numBuckets * gid0 + 0] = 0;
    while (true) {
        uint idx = SplitListSortSize * gid0 + i;
        if (i >= SplitListSortSize || idx >= listSize) {
            histogram[numBlocks * mcOfBucket + gid0] = count;
            ++mcOfBucket;
            while (mcOfBucket < numBuckets) {
                localOffsets[numBuckets * gid0 + mcOfBucket] = i;
                histogram[numBlocks * mcOfBucket + gid0] = 0;
                ++mcOfBucket;
            }
            break;
        }
        
        uchar mc = radixDigits[idx];
        if (mc == mcOfBucket) {
            ++count;
            ++i;
        }
        
        if (mc > mcOfBucket) {
            histogram[numBlocks * mcOfBucket + gid0] = count;
            count = 0;
            ++mcOfBucket;
            localOffsets[numBuckets * gid0 + mcOfBucket] = i;
        }
    }
}



kernel void globalScatterSplitList(const global uchar* radixDigits, uint listSize, uint numBuckets, const global uint* globalOffsets, const global ushort* localOffsets,
                                   const global uint2* splitListSrc, global uint2* splitListDst) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    if (gid0 >= listSize)
        return;
    
    uint2 listSrc = splitListSrc[gid0];
    uchar digit = radixDigits[gid0];
    uint gOffset = globalOffsets[get_num_groups(0) * digit + get_group_id(0)];
    uint lOffset = localOffsets[numBuckets * get_group_id(0) + digit];
    splitListDst[gOffset + (lid0 - lOffset)] = listSrc;
}
