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

typedef enum {
    TreeletInternal = 0,
    ActualLeaf = 1 << 0,
    TreeletLeaf = 1 << 1,
} TreeletNodeType;

#define TL_n 7
#define C_i 1.2f
#define C_t 1.0f

// SIMD幅32のためのスケジュール。
#define SCHEDULE schedule2to5
#define NUM_OptRounds 5
const constant char schedule2to5[] = {
    // size: 2, active: 10(10)
    0b0000011, 0b0000101, 0b0000110, 0b0001001, 0b0001010, 0b0001100, 0b0010001, 0b0010010,
    0b0010100, 0b0011000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000,
    0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000,
    0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000,
    
    // size: 2-3, active: 20(11, 9)
    0b0100001, 0b0100010, 0b0100100, 0b0101000, 0b0110000, 0b1000001, 0b1000010, 0b1000100,
    0b1001000, 0b1010000, 0b1100000,
    0b0000111, 0b0001011, 0b0001101, 0b0001110, 0b0010011, // size 3
    0b0010101, 0b0010110, 0b0011001, 0b0011010, 0b0000000, 0b0000000, 0b0000000, 0b0000000,
    0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000,
    
    // size: 3-4, active: 29(26, 3)
    0b0011100, 0b0100011, 0b0100101, 0b0100110, 0b0101001, 0b0101010, 0b0101100, 0b0110001,
    0b0110010, 0b0110100, 0b0111000, 0b1000011, 0b1000101, 0b1000110, 0b1001001, 0b1001010,
    0b1001100, 0b1010001, 0b1010010, 0b1010100, 0b1011000, 0b1100001, 0b1100010, 0b1100100,
    0b1101000, 0b1110000,
    0b0001111, 0b0010111, 0b0011011, 0b0000000, 0b0000000, 0b0000000, // size: 4
    
    // size: 4, active: 32(32)
    0b0011101, 0b0011110, 0b0100111, 0b0101011, 0b0101101, 0b0101110, 0b0110011, 0b0110101,
    0b0110110, 0b0111001, 0b0111010, 0b0111100, 0b1000111, 0b1001011, 0b1001101, 0b1001110,
    0b1010011, 0b1010101, 0b1010110, 0b1011001, 0b1011010, 0b1011100, 0b1100011, 0b1100101,
    0b1100110, 0b1101001, 0b1101010, 0b1101100, 0b1110001, 0b1110010, 0b1110100, 0b1111000,
    
    // size: 5, active: 21(21)
    0b0011111, 0b0101111, 0b0110111, 0b0111011, 0b0111101, 0b0111110, 0b1001111, 0b1010111,
    0b1011011, 0b1011101, 0b1011110, 0b1100111, 0b1101011, 0b1101101, 0b1101110, 0b1110011,
    0b1110101, 0b1110110, 0b1111001, 0b1111010, 0b1111100, 0b0000000, 0b0000000, 0b0000000,
    0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000, 0b0000000
};

//----------------------------------------------------------------

inline local uchar* alignPtrL(const local void* ptr, uintptr_t bytes);

kernel void calcNodeAABBs_SAHCosts(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                   const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts);

uint maxSurfaceAreaIndex(uint lid0, uint depth, uint parent, local uchar* localIndices, local float* surfaceAreas,
                         uint maxDepth, uint numElems);
kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                 const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts,
                                 uint gamma);

//----------------------------------------------------------------

inline local uchar* alignPtrL(const local void* ptr, uintptr_t bytes) {
    return (local uchar*)(((uintptr_t)ptr + (bytes - 1)) & ~(bytes - 1));
}

// 各ノードのAABB、leaf nodeの数、SAHコストを計算する。leaf nodeからルートへの経路単位で並列に計算される。
// グローバルメモリへの書き込みがキャッシュされてしまうとスレッド全体で一貫性が無くなってしまうのでvolatile属性をつける。
kernel void calcNodeAABBs_SAHCosts(global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                   const global uint* parentIdxs, global uint* numTotalLeaves, global float* SAHCosts) {
    const uint gid0 = get_global_id(0);
//    const uint lid0 = get_local_id(0);
    if (gid0 >= numPrimitives)
        return;
    
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    volatile global uint* numTotalLeavesUC = (volatile global uint*)numTotalLeaves;
    volatile global float* SAHCostsUC = (volatile global float*)SAHCosts;
    
//    uint localCounters[CALC_NODE_AABB_GROUP_SIZE];
//    localCounters[lid0] = 0;
    
    const global LeafNode* lNode = lNodes + gid0;
    point3 min = lNode->bbox.min;
    point3 max = lNode->bbox.max;
    uint numLeaves = 1;
    vector3 edge = max - min;
    float SAHCost = C_t * (edge.x * edge.y + edge.y * edge.z + edge.z * edge.x);
    
    uint selfIdx = gid0;
    uint tgtIdx = parentIdxs[gid0];
    *(numTotalLeavesUC + gid0) = numLeaves;
    *(SAHCostsUC + gid0) = SAHCost;
    parentIdxs += numPrimitives;
    numTotalLeavesUC += numPrimitives;
    SAHCostsUC += numPrimitives;
    
    // InternalNodeを繰り返しルートに向けて登る。
    // 2回目のアクセスを担当するスレッドがAABBの和をとることによって、そのノードの子全てが計算済みであることを保証する。
    while (atomic_inc(counters + tgtIdx) == 1) {
        volatile global InternalNode* tgtINode = (volatile global InternalNode*)iNodes + tgtIdx;
        
        bool leftIsSelf = tgtINode->c[0] == selfIdx;
        uint otherIdx = tgtINode->c[leftIsSelf];
        bool otherIsLeaf = tgtINode->isLeaf[leftIsSelf];
        
        const AABB bbox = otherIsLeaf ? (lNodes + otherIdx)->bbox : (iNodes + otherIdx)->bbox;
        tgtINode->bbox.min = min = fmin(min, bbox.min);
        tgtINode->bbox.max = max = fmax(max, bbox.max);
        
        otherIdx = otherIsLeaf ? otherIdx : (numPrimitives + otherIdx);
        numLeaves += numTotalLeaves[otherIdx];
        numTotalLeavesUC[tgtIdx] = numLeaves;
        
        vector3 edge = max - min;
        float area = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
        SAHCost = fmin(C_i * area + (SAHCost + SAHCosts[otherIdx]), C_t * area * numLeaves);
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

// 最も大きな表面積を持つノード(leaf nodeは除外)のローカルのインデックスを返す。
uint maxSurfaceAreaIndex(uint lid0, uint depth, uint parent, local uchar* localIndices, local float* surfaceAreas,
                         uint maxDepth, uint numElems) {
    bool validThread = lid0 < numElems;
    if (validThread)
        localIndices[lid0] = lid0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint d = maxDepth; d > 0; --d) {
        if (validThread && depth == d && (lid0 & 0x01) == 0x01) {
            uchar idx0 = localIndices[lid0];
            uchar idx1 = localIndices[lid0 + 1];
            localIndices[parent] = surfaceAreas[idx0] > surfaceAreas[idx1] ? idx0 : idx1;
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
    
     // Keplerアーキテクチャーだとローカルメモリ使用量は768B以下が望ましい。
    local float SA_SAHCosts[128];
    local float* subsetSurfaceAreas = SA_SAHCosts;
    local float* subsetSAHCosts = SA_SAHCosts;
    local char partitions[128];
    
    local uint numRoots;
    local uint treeletRoots[8];
    local uchar extraLMemPool[164] __attribute__((aligned(4)));
    
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
        
        while (atomic_inc(counters + pIdx) == 3) {
            if (numTotalLeaves[pIdx] >= gamma/* || pIdx == 0*/) {
                // この部分もCUDAのballot()のようなものがあればローカルメモリやatomicを使わず効率化可能。SIMD幅はハード依存性が高いためか、OpenCL 2.0にもballot()に相当するものは無い。
                // work_group_broadcast(rootIdx, work_group_reduce_max(lid0 * !done))でループすれば比較的シンプルに書くことは可能？
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
    
    // extraLMem: 0 - 131bytes
    local uint* leafIndices = (local uint*)alignPtrL(extraLMemPool, sizeof(uint));
    local float* surfaceAreas = (local float*)alignPtrL(leafIndices + 2 * TL_n - 1, sizeof(float));
    local uchar* localIndices = (local uchar*)alignPtrL(surfaceAreas + 2 * TL_n - 1, sizeof(uchar));
    local uchar* depths = (local uchar*)alignPtrL(localIndices + 2 * TL_n - 1, sizeof(uchar));
    local uchar* maxDepth = (local uchar*)alignPtrL(depths + 2 * TL_n - 1, sizeof(uchar));
    uint parent;
    TreeletNodeType nodeType;
    AABB bbox;
    
    // 有効なサブツリーそれぞれに関してtreeletの形成と最適化処理を行う。
    for (uint i = 0; i < numRoots; ++i) {
        //----------------------------------------------------------------
        // Treelet Formation
        // サブツリーのルートとその子2つから始めて、AABBの表面積の大きいtreelet leafをその子2つで置き換えるという処理を繰り返す。
        // 5回繰り返すことによって7個のtreelet leafを持ったtreeletを形成する。
        nodeType = TreeletLeaf;
        if (lid0 == 1 || lid0 == 2) {
            const global InternalNode* iNode = iNodes + treeletRoots[i];
            uint lr = lid0 & 0x01;
            leafIndices[lid0] = iNode->c[lr];
            nodeType |= (iNode->isLeaf[lr] ? ActualLeaf : 0);
            if ((nodeType & ActualLeaf) != ActualLeaf) {
                bbox = (iNodes + leafIndices[lid0])->bbox;
                vector3 edge = bbox.max - bbox.min;
                surfaceAreas[lid0] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
            }
            else {
                bbox = (lNodes + leafIndices[lid0])->bbox;
                surfaceAreas[lid0] = 0;
            }
            parent = 0;
            depths[lid0] = 1;
        }
        else if (lid0 == 0) {
            *maxDepth = 1;
            nodeType = TreeletInternal;
            
            depths[0] = 0;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        for (int j = 0; j < TL_n - 2; ++j) {
            uint maxIndex = maxSurfaceAreaIndex(lid0, depths[lid0], parent, localIndices, surfaceAreas, *maxDepth, 3 + 2 * j);
            
            if ((int)lid0 - 2 * j ==  3 || (int)lid0 - 2 * j == 4) {
                const global InternalNode* iNode = iNodes + leafIndices[maxIndex];
                uint lr = lid0 & 0x01;
                leafIndices[lid0] = iNode->c[lr];
                nodeType |= (iNode->isLeaf[lr] ? ActualLeaf : 0);
                if ((nodeType & ActualLeaf) != ActualLeaf) {
                    bbox = (iNodes + leafIndices[lid0])->bbox;
                    vector3 edge = bbox.max - bbox.min;
                    surfaceAreas[lid0] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
                }
                else {
                    bbox = (lNodes + leafIndices[lid0])->bbox;
                    surfaceAreas[lid0] = 0;
                }
                parent = maxIndex;
                depths[lid0] = depths[parent] + 1;
            }
            else if (lid0 == maxIndex) {
                *maxDepth = max((uchar)(depths[lid0] + 1), *maxDepth);
                nodeType = TreeletInternal;
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }
//        if (get_group_id(0) == 1026) {
//            if (lid0 < 13 && (nodeType & TreeletLeaf) == TreeletLeaf)
//                printf("G: %5u (%5u, %2u) | %5u%c\n", get_group_id(0), treeletRoots[i], lid0, leafIndices[lid0], (nodeType & ActualLeaf) == ActualLeaf ? 'L' : ' ');
//        }
        // END: Treelet Formation
        //----------------------------------------------------------------
        
        //----------------------------------------------------------------
        // Local Memory Compaction
        // 後の処理で扱いやすくなるように、treelet leafのインデックスなどをローカルメモリ中で詰めておく。
        // extraLMem: 28 - 80
        local bool* isActualLeaf = (local bool*)alignPtrL(leafIndices + TL_n, sizeof(bool));
        local uchar* leafRefs = (local uchar*)alignPtrL(isActualLeaf + TL_n, sizeof(uchar));
        local uint* idxCounter = (local uint*)alignPtrL(leafRefs + TL_n, sizeof(uint));
        // CUDAにおけるshuffle()がOpenCL 1.2では使えないのでこの後のAABBの組み合わせ計算ステップではこのローカルメモリ領域を経由させる。
        local AABB* leafAABB = (local AABB*)alignPtrL(idxCounter + 1, sizeof(point3));
        
        uint leafIndex = UINT_MAX;
        if (lid0 < 2 * TL_n - 1 && (nodeType & TreeletLeaf) == TreeletLeaf)
            leafIndex = leafIndices[lid0];
        if (lid0 == 0)
            *idxCounter = 0;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (leafIndex != UINT_MAX) {
            uint idx = atomic_inc(idxCounter);
            leafIndices[idx] = leafIndex;
            isActualLeaf[idx] = (nodeType & ActualLeaf) == ActualLeaf;
            leafRefs[idx] = lid0;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
//        if (lid0 == 0) {
//            printf("%5u: %5u, %5u, %5u, %5u, %5u, %5u, %5u\n", treeletRoots[i],
//                   leafIndices[0], leafIndices[1], leafIndices[2], leafIndices[3], leafIndices[4], leafIndices[5], leafIndices[6]);
//        }
        // END: Local Memory Compaction
        //----------------------------------------------------------------

        //----------------------------------------------------------------
        // AABB Calculation
        // 各スレッドが4パターンの組み合わせ、32のグループサイズで計128パターンのAABBの和の表面積を計算する。
        // スレッド内の4パターンでは、7つのAABBのうち5つ(インデックス2-6)の有無は共通となるので先に計算を済ませる。
        point3 commonAABBMin, commonAABBMax;
        commonAABBMin = (point3)(INFINITY, INFINITY, INFINITY);
        commonAABBMax = (point3)(-INFINITY, -INFINITY, -INFINITY);
        uchar commonFlags = 4 * (lid0 + 1) - 1;
        for (int j = TL_n - 1; j > 1; --j) {
            if (leafRefs[j] == lid0)// OpenCL 2.0なら *** = work_group_broadcast(bbox.min, leafRefs[j])と言った感じで書ける？
                *leafAABB = bbox;
            barrier(CLK_LOCAL_MEM_FENCE);
            if ((commonFlags >> j) & 0x01) {
                commonAABBMin = fmin(commonAABBMin, leafAABB->min);
                commonAABBMax = fmax(commonAABBMax, leafAABB->max);
            }
        }
        AABB unifiedAABB;
        // 共通AABBのみ。
        vector3 edge = commonAABBMax - commonAABBMin;
        subsetSurfaceAreas[4 * lid0 + 0] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
        // インデックス0のAABBと共通AABBの和。
        if (leafRefs[0] == lid0)
            *leafAABB = bbox;
        barrier(CLK_LOCAL_MEM_FENCE);
        unifiedAABB.min = fmin(commonAABBMin, leafAABB->min);
        unifiedAABB.max = fmax(commonAABBMax, leafAABB->max);
        edge = unifiedAABB.max - unifiedAABB.min;
        subsetSurfaceAreas[4 * lid0 + 1] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
        // インデックス1のAABBと共通AABBの和。
        if (leafRefs[1] == lid0)
            *leafAABB = bbox;
        barrier(CLK_LOCAL_MEM_FENCE);
        unifiedAABB.min = fmin(commonAABBMin, leafAABB->min);
        unifiedAABB.max = fmax(commonAABBMax, leafAABB->max);
        edge = unifiedAABB.max - unifiedAABB.min;
        subsetSurfaceAreas[4 * lid0 + 2] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
        // インデックス0, 1のAABBと共通AABBの和。
        if (leafRefs[0] == lid0)
            *leafAABB = bbox;
        barrier(CLK_LOCAL_MEM_FENCE);
        unifiedAABB.min = fmin(unifiedAABB.min, leafAABB->min);
        unifiedAABB.max = fmax(unifiedAABB.max, leafAABB->max);
        edge = unifiedAABB.max - unifiedAABB.min;
        subsetSurfaceAreas[4 * lid0 + 3] = edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
//        if (lid0 == 0)
//            printf("%5u: %f\n", treeletRoots[i], subsetSurfaceAreas[127]);
//        return;
        // END: AABB Calculation
        //----------------------------------------------------------------
        
        // Initialize costs of individual leaves
        // 単独のノードだけが分割される場合のSAHコストを初期化しておく。
        if (lid0 < TL_n)
            subsetSAHCosts[1 << lid0] = SAHCosts[leafIndices[lid0] + (isActualLeaf[lid0] ? 0 : numPrimitives)];
        barrier(CLK_LOCAL_MEM_FENCE);
        
        //----------------------------------------------------------------
        // Optimize every subset of leaves
        // 7つのtreelet leafの部分集合それぞれに対してSAHコストが最小となる分割を求める。
        // 基本的に部分集合のサイズの小さいものから順番に求めるが、SIMD利用率を向上させるため、サイズ2~5は予め作成済みのスケジュールに従って異なるサイズも一部並列に処理する。
        for (int round = 0; round < NUM_OptRounds; ++round) {
            char subset = SCHEDULE[32 * round + lid0];
            if (subset != 0) {
                float c_s = INFINITY;
                char p_s = 0;
                
                // Try each way of partitionings the leaves
                // ある部分集合に対する最適な分割を求める。
                char delta = (subset - 1) & subset;
                char p = (-delta) & subset;
//                uchar pCount = 0;
                do {
                    float c = subsetSAHCosts[(uchar)p] + subsetSAHCosts[(uchar)(subset ^ p)];
                    if (c < c_s) {
                        c_s = c;
                        p_s = p;
                    }
                    p = (p - delta) & subset;
//                    ++pCount;
//                    if (get_group_id(0) == 0 && i == 0)
//                        printf("round: %u, size: %u, subset: %#08u(%2u), partition: %2u\n", round + 1, popcount(subset), (uchar)subset, lid0, pCount);
                } while (p != 0);
                
                // ある部分集合に対する最終的なSAHコストと分割を記録する。
                uint numLeaves_s = 0;
                for (int j = 0; j < TL_n; ++j) {// この処理もローカルメモリやOpenCL 2.0のwork_group_broadcast()によって最適化できるかもしれない。
                    if ((subset >> j) & 0x01)
                        numLeaves_s += isActualLeaf[j] ? 1 : numTotalLeaves[leafIndices[j] + numPrimitives];
                }
                float surfaceArea = subsetSurfaceAreas[(uchar)subset];
                subsetSAHCosts[(uchar)subset] = fmin(C_i * surfaceArea + c_s, C_t * surfaceArea * numLeaves_s);
                partitions[(uchar)subset] = p_s;
            }
            
            barrier(CLK_LOCAL_MEM_FENCE);
        }
        
        // extraLMem: 35 - 164
        local uint* lNumLeaves_s = (local uint*)alignPtrL(isActualLeaf + TL_n, sizeof(uint));
        local float* reductionBuffer = (local float*)alignPtrL(isActualLeaf + TL_n, sizeof(float));
        
        // size: 6, subset: 7, active: 28
        // 1つの部分集合(31通りの分割)を4つのスレッドで処理する。1スレッドあたり8種類の分割コストを計算する。
        // 4番目のスレッドの最後1つは余分な(重複した)計算となるが、そのためだけにif文を追加するのも微妙？
        float c_s = INFINITY;
        char p_s = 0;
        char bit = lid0 >> 2;// 0 ~ 6
        char subset = 0b1111111 ^ (1 << bit);// 1111110, 1111101, 1111011, ... 0111111
        // 部分集合毎のLeafノードの数を求めて、同じ部分集合を処理するスレッド間で共有しておく。
        uint numLeaves_s = 0;
        if (lid0 < 28 && (lid0 & 0b11) == 0) {
            for (int j = 0; j < TL_n; ++j) {// この処理もローカルメモリやOpenCL 2.0のwork_group_broadcast()によって最適化できるかもしれない。
                if ((subset >> j) & 0x01)
                    numLeaves_s += isActualLeaf[j] ? 1 : numTotalLeaves[leafIndices[j] + numPrimitives];
            }
            lNumLeaves_s[(uchar)bit] = numLeaves_s;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        numLeaves_s = lNumLeaves_s[(uchar)bit];
        if (lid0 < 28) {
            char lowMask = (1 << bit) - 1;// 000000, 000001, 000011, ... 111111
            for (char j = 0; j < 8; ++j) {
                // ex) bit: 2 => 1111011
                char p = 8 * (lid0 & 0b11) + j + 1;// 1 ~ 32
                p = ((p & ~lowMask) << 1) | (p & lowMask);
                float c = subsetSAHCosts[(uchar)p] + subsetSAHCosts[(uchar)(subset ^ p)];
                if (c < c_s) {
                    c_s = c;
                    p_s = p;
                }
            }
            reductionBuffer[lid0] = c_s;
        }
        else {
            reductionBuffer[lid0] = INFINITY;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        // 4スレッド間でreductionを行い、最小コストを求める。最小のコストに一致するスレッドが保持する分割を記録する。
        if ((lid0 & 0b01) == 0)
            reductionBuffer[lid0] = fmin(reductionBuffer[lid0], reductionBuffer[lid0 + 1]);
        barrier(CLK_LOCAL_MEM_FENCE);
        if ((lid0 & 0b11) == 0)
            reductionBuffer[lid0] = fmin(reductionBuffer[lid0], reductionBuffer[lid0 + 2]);
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lid0 < 28 && c_s == reductionBuffer[lid0 & ~0b11]) {
            float surfaceArea = subsetSurfaceAreas[(uchar)subset];
            subsetSAHCosts[(uchar)subset] = fmin(C_i * surfaceArea + c_s, C_t * surfaceArea * numLeaves_s);
            partitions[(uchar)subset] = p_s;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        
        // size: 7, subset: 1, active: 32
        // 部分集合はそもそも1つしか存在しない。63通り(0000001 ~ 0111111)の分割が考えられるため、各スレッドが2つの分割コストを計算する。
        if (lid0 == 0) {
            for (int j = 0; j < 7; ++j) {
                numLeaves_s += isActualLeaf[j] ? 1 : numTotalLeaves[leafIndices[j] + numPrimitives];
            }
            lNumLeaves_s[0] = numLeaves_s;
            subsetSAHCosts[0] = subsetSAHCosts[0b1111111] = INFINITY;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        numLeaves_s = lNumLeaves_s[0];
        char p = 2 * lid0 + 0;
        float c_s0 = subsetSAHCosts[(uchar)p] + subsetSAHCosts[(uchar)(0b1111111 ^ p)];
        p += 1;
        float c_s1 = subsetSAHCosts[(uchar)p] + subsetSAHCosts[(uchar)(0b1111111 ^ p)];
        c_s = c_s0 < c_s1 ? c_s0 : c_s1;
        p_s = c_s0 < c_s1 ? (p - 1) : p;
        reductionBuffer[lid0] = c_s;
        barrier(CLK_LOCAL_MEM_FENCE);
        // 2スレッド毎の最小コストを求めた段階でreductionを実行、全ての分割中で最小のコストを求める。最小のコストに一致するスレッドが保持する分割を記録する。
        for (uint j = 16; j > 0; j >>= 1) {
            if (lid0 < j)
                reductionBuffer[lid0] = fmin(reductionBuffer[lid0], reductionBuffer[lid0 + j]);
            barrier(CLK_LOCAL_MEM_FENCE);
        }
        if (lid0 < 32 && c_s == reductionBuffer[0]) {
            float surfaceArea = subsetSurfaceAreas[(uchar)0b1111111];
            subsetSAHCosts[(uchar)0b1111111] = fmin(C_i * surfaceArea + c_s, C_t * surfaceArea * numLeaves_s);
            partitions[(uchar)0b1111111] = p_s;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        // END: Optimize every subset of leaves
        //----------------------------------------------------------------
        
//        if (lid0 == 0 && get_group_id(0) == 0)
//            printf("subsetSurfaceAreas: %#08x\n"
//                   "subsetSAHCosts: %#08x\n"
//                   "partitions: %#08x\n"
//                   "extraLMemPool: %#08x\n"
//                   "numRoots: %#08x\n"
//                   "treeletRoots: %#08x\n"
//                   "leafIndices: %#08x\n"
//                   "surfaceAreas: %#08x\n"
//                   "localIndices: %#08x\n"
//                   "depths: %#08x\n"
//                   "maxDepth: %#08x\n"
//                   "isActualLeaf: %#08x\n"
//                   "leafRefs: %#08x\n"
//                   "idxCounter: %#08x\n"
//                   "leafAABB: %#08x\n"
//                   "lNumLeaves_s: %#08x\n"
//                   "reductionBuffer: %#08x\n",
//                   subsetSurfaceAreas, subsetSAHCosts, partitions, extraLMemPool,
//                   &numRoots, treeletRoots, leafIndices, surfaceAreas,
//                   localIndices, depths, maxDepth, isActualLeaf,
//                   leafRefs, idxCounter, leafAABB, lNumLeaves_s,
//                   reductionBuffer);
//        return;
    }
}
