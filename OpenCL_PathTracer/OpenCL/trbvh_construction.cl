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

kernel void calcNodeAABBs_SAHCosts(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                   const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts);

uchar maxSurfaceAreaIndex(uint lid0, local uchar* localIndices, local float* surfaceAreas,
                          local uchar* depths, uchar maxDepth,
                          local uchar* parents, local bool* isActualLeaf, uint numElems);
kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                 const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts,
                                 uint gamma);

//----------------------------------------------------------------

kernel void calcNodeAABBs_SAHCosts(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                   const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts) {
    const uint gid0 = get_global_id(0);
//    const uint lid0 = get_local_id(0);
    if (gid0 >= numPrimitives)
        return;
    
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    const float Ci = 1.2f;
    const float Ct = 1.0f;
    
//    uint localCounters[CALC_NODE_AABB_GROUP_SIZE];
//    localCounters[lid0] = 0;
    
    const global LeafNode* lNode = lNodes + gid0;
    point3 min = lNode->bbox.min;
    point3 max = lNode->bbox.max;
    uint numLeaves = 1;
    vector3 edge = max - min;
    float SAHCost = Ct * (edge.x * edge.y + edge.y * edge.z + edge.z * edge.x);
    
    uint selfIdx = gid0;
    uint tgtIdx = parentIdxs[gid0];
    *(numTotalLeaves + gid0) = numLeaves;
    *(SAHCosts + gid0) = SAHCost;
    parentIdxs += numPrimitives;
    numTotalLeaves += numPrimitives;
    SAHCosts += numPrimitives;
    
    // InternalNodeを繰り返しルートに向けて登る。
    // 2回目のアクセスを担当するスレッドがAABBの和をとることによって、そのノードの子全てが計算済みであることを保証する。
    volatile global uint* numTotalLeavesUC = (volatile global uint*)numTotalLeaves;
    volatile global float* SAHCostsUC = (volatile global float*)SAHCosts;
    while (atomic_inc(counters + tgtIdx) == 1) {
        // グローバルメモリへの書き込みがキャッシュされてしまうとスレッド全体で一貫性が無くなってしまうのでvolatile属性をつける。
        volatile global InternalNode* tgtINode = (volatile global InternalNode*)iNodes + tgtIdx;
        
        bool leftIsSelf = tgtINode->c[0] == selfIdx;
        uint otherIdx = tgtINode->c[leftIsSelf];
        
        const AABB bbox = tgtINode->isLeaf[leftIsSelf] ? (lNodes + otherIdx)->bbox : (iNodes + otherIdx)->bbox;
        tgtINode->bbox.min = min = fmin(min, bbox.min);
        tgtINode->bbox.max = max = fmax(max, bbox.max);
        
        numLeaves += numTotalLeavesUC[otherIdx];
        numTotalLeavesUC[tgtIdx] = numLeaves;
        
        vector3 edge = max - min;
        float area = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
        SAHCost = fmin(Ci * area + (SAHCost + SAHCostsUC[otherIdx]), Ct * area * numLeaves);
        SAHCostsUC[tgtIdx] = SAHCost;
        
        if (tgtIdx == 0)
            return;
        
        selfIdx = tgtIdx;
        tgtIdx = parentIdxs[tgtIdx];
    }
}


#ifndef LOCAL_SIZE
#define LOCAL_SIZE 32
#endif

const constant uint n = 7;

typedef enum {
    ActualLeaf = 1 << 0,
    TreeletInternal = 1 << 1,
} TreeletNodeType;

// 最も大きな表面積を持つノード(leaf nodeは除外)のローカルのインデックスを返す。
uchar maxSurfaceAreaIndex(uint lid0, local uchar* localIndices, local float* surfaceAreas,
                          local uchar* depths, uchar maxDepth,
                          local uchar* parents, local bool* isActualLeaf, uint numElems) {
    bool validThread = lid0 < numElems;
    if (validThread)
        localIndices[lid0] = lid0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uchar d = maxDepth; d > 0; --d) {
        if (validThread && depths[((int)lid0 - 1) >> 1] == d && (lid0 & 0x01) == 0x01) {
            uchar idx0 = localIndices[lid0];
            uchar idx1 = localIndices[lid0 + 1];
            float SA0 = isActualLeaf[idx0] ? 0 : surfaceAreas[idx0];
            float SA1 = isActualLeaf[idx1] ? 0 : surfaceAreas[idx1];
            uchar pIdx = parents[((int)lid0 - 1) >> 1];
            localIndices[pIdx] = SA0 > SA1 ? idx0 : idx1;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    return localIndices[0];
}

kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                 const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts,
                                 uint gamma) {
    const uint gid0 = get_global_id(0);
    const uint lid0 = get_local_id(0);
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    local uchar localMemPool[640] __attribute__((aligned(4)));
    
    local uint numRoots;
    local uint treeletRoots[8];
    if (lid0 == 0)
        numRoots = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    //----------------------------------------------------------------
    // Bottom-up Traversal
    // 各スレッドがleaf nodeからスタートしてtreeletの形成に十分なサイズのサブツリーを含むまで木を登り、ルートのインデックスを記録する。
    if (gid0 < numPrimitives) {
        uint selfIdx = gid0;
        uint pIdx = parentIdxs[gid0];
        parentIdxs += numPrimitives;
        numTotalLeaves += numPrimitives;
        
        // グローバルメモリへの書き込み・読み込みがキャッシュされないようにvolatile属性をつける。
        while (atomic_inc(counters + pIdx) == 3) {
            const global InternalNode* iNode = iNodes + pIdx;            
            if (numTotalLeaves[gid0] >= gamma/* || pIdx == 0*/) {
                treeletRoots[atomic_inc(&numRoots)] = pIdx;
                break;
            }
            
            selfIdx = pIdx;
            pIdx = parentIdxs[pIdx];
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
//    if (lid0 == 0) {
//        for (uint i = 0; i < numRoots; ++i) {
//            uint idx = treeletRoots[i];
//            printf("%5u(%u/%u): %5u(%3u)\n", get_group_id(0), i + 1, numRoots, idx, numTotalLeaves[idx]);
//        }
//    }
//    return;
    // END: Bottom-up Traversal
    //----------------------------------------------------------------
    
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
        // サブツリーのルートとその子2つから始めて、AABBの表面積の大きいtreelet leafをその子2つで置き換えるという処理を繰り返す。
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
            
            leafIndices[0] = UINT_MAX;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        for (int j = 0; j < (int)n - 2; ++j) {
            if (((int)lid0 - 1) >> 1 == j) {
                AABB bbox;
                if (isActualLeaf[lid0])
                    bbox = (lNodes + leafIndices[lid0])->bbox;
                else
                    bbox = (iNodes + leafIndices[lid0])->bbox;
                vector3 edge = bbox.max - bbox.min;
                surfaceAreas[lid0] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
            }
            barrier(CLK_LOCAL_MEM_FENCE);
            
            uint numElems = 3 + 2 * j;
            uchar maxIndex = maxSurfaceAreaIndex(lid0, localIndices, surfaceAreas, depths, *maxDepth, parents, isActualLeaf, numElems);
            
            if (lid0 == 0) {
                const global InternalNode* iNode = iNodes + leafIndices[maxIndex];
                leafIndices[numElems + 0] = iNode->c[0];
                leafIndices[numElems + 1] = iNode->c[1];
                isActualLeaf[numElems + 0] = iNode->isLeaf[0];
                isActualLeaf[numElems + 1] = iNode->isLeaf[1];
                parents[j + 1] = maxIndex;
                depths[j + 1] = depths[((char)maxIndex - 1) >> 1] + 1;
                *maxDepth = max(depths[j + 1], *maxDepth);
                
                leafIndices[maxIndex] = UINT_MAX;
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }
//        if (lid0 == 0 && get_group_id(0) == 1026) {
//            uint idx = 0;
//            for (uint j = 0; j < 13; ++j) {
//                if (leafIndices[j] != UINT_MAX)
//                    printf("G: %5u (%5u, %u) | %5u%c\n", get_group_id(0), treeletRoots[i], idx++, leafIndices[j], isActualLeaf[j] ? 'L' : ' ');
//            }
//        }
        // END: Treelet Formation
        //----------------------------------------------------------------
        
        uint leafIndex = UINT_MAX;
        if (lid0 < 2 * n - 1)
            leafIndex = leafIndices[lid0];
        barrier(CLK_LOCAL_MEM_FENCE);
        
        local uint* idxCounter = (local uint*)localIndices + n;
        if (lid0 == 0)
            *idxCounter = 0;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (leafIndex != UINT_MAX)
            leafIndices[atomic_inc(idxCounter)] = leafIndex;
        barrier(CLK_LOCAL_MEM_FENCE);
        
//        if (lid0 == 0) {
//            printf("%5u: %5u, %5u, %5u, %5u, %5u, %5u, %5u\n", treeletRoots[i],
//                   leafIndices[0], leafIndices[1], leafIndices[2], leafIndices[3], leafIndices[4], leafIndices[5], leafIndices[6]);
//        }
        
        point3 commonAABBMin, commonAABBMax;
    }
}
