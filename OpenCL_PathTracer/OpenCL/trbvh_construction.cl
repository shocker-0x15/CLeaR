typedef float3 point3;
typedef float3 vector3;

// 32bytes
typedef struct __attribute__((aligned(16))) {
    point3 min;
    point3 max;
} AABB;

// 48bytes
typedef struct __attribute__((aligned(16))) {
    AABB bbox;
    bool isLeaf[2];
    uint c[2];
} InternalNode;

// 48bytes
typedef struct __attribute__((aligned(16))) {
    AABB bbox;
    uint objIdx;
} LeafNode;

//----------------------------------------------------------------

uchar maxSurfaceAreaIndex(uint lid0, local uchar* localIndices, local float* surfaceAreas,
                          local uchar* depths, uchar maxDepth,
                          local uchar* parents, local bool* isActualLeaf, uint numElems);
kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, global uint* numTotalLeaves,
                                 const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs,
                                 uint gamma);

//----------------------------------------------------------------

#ifndef LOCAL_SIZE
#define LOCAL_SIZE 32
#endif

constant uint n = 7;

typedef enum {
    ActualLeaf = 1 << 0,
    TreeletInternal = 1 << 1,
} TreeletNodeType;

uchar maxSurfaceAreaIndex(uint lid0, local uchar* localIndices, local float* surfaceAreas,
                          local uchar* depths, uchar maxDepth,
                          local uchar* parents, local bool* isActualLeaf, uint numElems) {
    bool validThread = lid0 < numElems;
    if (validThread)
        localIndices[lid0] = lid0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uchar d = maxDepth; d > 0; --d) {
        if (validThread && depths[(lid0 - 1) >> 1] == d && (lid0 & 0x01) == 0x01) {
            uchar idx0 = localIndices[lid0];
            uchar idx1 = localIndices[lid0 + 1];
            float SA0 = isActualLeaf[idx0] ? 0 : surfaceAreas[idx0];
            float SA1 = isActualLeaf[idx1] ? 0 : surfaceAreas[idx1];
            uchar pIdx = parents[(lid0 - 1) >> 1];
            localIndices[pIdx] = SA0 > SA1 ? idx0 : idx1;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    return localIndices[0];
}

kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, global uint* numTotalLeaves,
                                 const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs,
                                 uint gamma) {
    const uint gid0 = get_global_id(0);
    const uint lid0 = get_local_id(0);
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    local uchar localMemPool[640];
    
    local uint numRoots;
    local uint treeletRoots[8];
    if (lid0 == 0)
        numRoots = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    //----------------------------------------------------------------
    // Bottom-up Traversal
    // 各スレッドがleaf nodeからスタートしてtreeletの形成に十分なサイズのサブツリーを含むまで木を登る。
    // その際にサブツリーが含むleaf nodeの数を記録する。
    if (gid0 < numPrimitives) {
        uint numLeaves = 1;
        uint selfIdx = gid0;
        uint pIdx = parentIdxs[gid0];
        parentIdxs += numPrimitives;
        
        // グローバルメモリへの書き込み・読み込みがキャッシュされないようにvolatile属性をつける。
        volatile global uint* numTotalLeavesUC = (volatile global uint*)numTotalLeaves;
        while (atomic_inc(counters + pIdx) == 3) {
            const global InternalNode* iNode = iNodes + pIdx;
            bool leftIsSelf = iNode->c[0] == selfIdx;
            uint otherIdx = iNode->c[leftIsSelf];
            numLeaves += iNode->isLeaf[leftIsSelf] ? 1 : numTotalLeavesUC[otherIdx];
            numTotalLeavesUC[pIdx] = numLeaves;
            
            if (numLeaves >= gamma/* || pIdx == 0*/) {
                treeletRoots[atomic_inc(&numRoots)] = pIdx;
                break;
            }
            
            selfIdx = pIdx;
            pIdx = parentIdxs[pIdx];
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    // END: Bottom-up Traversal
    //----------------------------------------------------------------
//    if (lid0 == 0) {
//        for (uint i = 0; i < numRoots; ++i) {
//            uint idx = treeletRoots[i];
//            printf("%5u: %5u(%3u)\n", get_group_id(0), idx, numTotalLeavesUC[idx]);
//        }
//    }
//    return;
    
    local uint* leafIndices = (local uint*)localMemPool;
    local float* surfaceAreas = (local float*)(leafIndices + 2 * n - 1);
    local uchar* localIndices = (local uchar*)(surfaceAreas + 2 * n - 1);
    local bool* isActualLeaf = (local bool*)(localIndices + 2 * n - 1);
    local uchar* parents = (local uchar*)(isActualLeaf + 2 * n - 1);
    local uchar* depths = (local uchar*)(parents + n - 1);
    local uchar* maxDepth = (local uchar*)(depths + n - 1);
    
    // 有効なサブツリーそれぞれに関してtreeletの形成と最適化処理を行う。
    for (uint i = 0; i < numRoots; ++i) {
        //----------------------------------------------------------------
        // Treelet Formation
        // 有効なサブツリーのルートとその子2つから始めて、AABBの表面積の大きいtreelet leafをその子2つで置き換えるという処理を繰り返す。
        // 5回繰り返すことによって7個のtreelet leafを持ったtreeletを形成する。
        if (lid0 == 0) {
            const global InternalNode* iNode = iNodes + treeletRoots[i];
            leafIndices[1] = iNode->c[0];
            leafIndices[2] = iNode->c[1];
            isActualLeaf[1] = iNode->isLeaf[0];
            isActualLeaf[2] = iNode->isLeaf[1];
            parents[0] = 0;
            depths[0] = 1;
            *maxDepth = 1;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        for (uint j = 0; j < 5; ++j) {
            if ((lid0 - 1) >> 1 == j) {
                AABB bbox;
                if (isActualLeaf[lid0])
                    bbox = (lNodes + leafIndices[lid0])->bbox;
                else
                    bbox = (iNodes + leafIndices[lid0])->bbox;
                vector3 edge = bbox.max - bbox.min;
                surfaceAreas[lid0] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
            }
            barrier(CLK_LOCAL_MEM_FENCE);
            
            uchar maxIndex = maxSurfaceAreaIndex(lid0, localIndices, surfaceAreas, depths, *maxDepth, parents, isActualLeaf, 3 + 2 * j);
            
            if (lid0 == 0) {
                uint idxBase = 3 + 2 * j;
                const global InternalNode* iNode = iNodes + leafIndices[maxIndex];
                leafIndices[idxBase + 0] = iNode->c[0];
                leafIndices[idxBase + 1] = iNode->c[1];
                isActualLeaf[idxBase + 0] = iNode->isLeaf[0];
                isActualLeaf[idxBase + 1] = iNode->isLeaf[1];
                parents[j + 1] = maxIndex;
                depths[j + 1] = depths[(maxIndex - 1) >> 1] + 1;
                *maxDepth = max(depths[j + 1], *maxDepth);
                
                if (treeletRoots[i] == 3998) {
                    printf("%5u, %u, %u\n", leafIndices[maxIndex], maxIndex, *maxDepth);
                }
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }
        // END: Treelet Formation
        //----------------------------------------------------------------
    }
}
