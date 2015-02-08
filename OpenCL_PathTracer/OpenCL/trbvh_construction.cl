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
    TLNodeType_Invalid = 0,
    TLNodeType_Internal = 1 << 0,
    TLNodeType_ActualLeaf = 1 << 1,
    TLNodeType_Leaf = 1 << 2,
} TLNodeType;

#define TL_n 7
#define FullSet 0b1111111
#define C_i 1.2f
#define C_t 1.0f

// SIMD幅32のためのスケジュール。
#define SIMD_Width 32
#define NUM_OptRounds 5
const constant char schedule[] = {
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

inline uint __ballot(local uint* mem, uint localID, bool pred);
inline uint largestOneBit32(uint value);
inline void setZeroAt32(uint* value, uint index);
inline float calcSurfaceAreaAABB(const AABB bb);
inline AABB unionAABB(const AABB bb0, const local AABB* bb1);
uint maxSurfaceAreaIndex(uint lid, local float* TLSurfaceAreas, local uint* TLIndices, uint numElems);
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
    if (gid0 >= numPrimitives)
        return;
    
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    volatile global uint* numTotalLeavesUC = (volatile global uint*)numTotalLeaves;
    volatile global float* SAHCostsUC = (volatile global float*)SAHCosts;
    
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

// CUDAのballotを模倣する関数。
inline uint __ballot(local uint* mem, uint localID, bool pred) {
    if (localID == 0)
        *mem = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    if (pred)
        atomic_or(mem, 1 << localID);
    barrier(CLK_LOCAL_MEM_FENCE);
    return *mem;
}
#define ballot(pred) __ballot(&ballotField, lid0, pred)

// 1になっているビット中で最大のインデックスを返す。
inline uint largestOneBit32(uint value) {
    return 31 - clz(value);
}

// indexで指定したビットをゼロにセットする。
inline void setZeroAt32(uint* value, uint index) {
    *value &= ~(1 << index);
}

// AABBの表面積(の半分)を計算する。
inline float calcSurfaceAreaAABB(const AABB bb) {
    vector3 edge = bb.max - bb.min;
    return edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
}

// AABBの和を計算する。
inline AABB unionAABB(const AABB bb0, const local AABB* bb1) {
    AABB ret;
    ret.min = fmin(bb0.min, bb1->min);
    ret.max = fmax(bb0.max, bb1->max);
    return ret;
}

// 最も大きな表面積を持つノード(leaf nodeは除外)のインデックスを返す。
uint maxSurfaceAreaIndex(uint lid, local float* TLSurfaceAreas, local uint* TLIndices, uint numElems) {
    uint nextPow2 = 1 << (32 - clz(numElems - 1));
    for (uint j = nextPow2 >> 1; j > 0; j >>= 1) {
        if (lid < j) {
            float SA0 = TLSurfaceAreas[lid];
            float SA1 = (lid + j) < numElems ? TLSurfaceAreas[lid + j] : 0;
            if (SA1 > SA0) {
                TLSurfaceAreas[lid] = TLSurfaceAreas[lid + j];
                TLIndices[lid] = TLIndices[lid + j];
            }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    return TLIndices[0];
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
    
    // OpenCL 1.2以下では(少なくとも明示的に)ローカルメモリを使わないと、
    // reductionなどが出来ないので元論文より余分な領域の確保が必要。
    local uint ballotField;
    local uchar extraLMemPool[128] __attribute__((aligned(16)));
    
    uint rootFlags = 0;
    uint treeletRoot = UINT_MAX;
    
    //----------------------------------------------------------------
    // Bottom-up Traversal
    // 各スレッドがleaf nodeからスタートしてtreeletの形成に十分なサイズのサブツリーを含むまで木を登り、ルートのインデックスを記録する。
    if (gid0 < numPrimitives) {
        uint selfIdx = gid0;
        uint pIdx = parentIdxs[gid0];
        
        while (atomic_inc(counters + pIdx) == 3) {
            if (numTotalLeaves[numPrimitives +  pIdx] >= gamma/* || pIdx == 0*/) {
                treeletRoot = pIdx;
                break;
            }
            
            selfIdx = pIdx;
            pIdx = parentIdxs[numPrimitives +  pIdx];
        }
    }
    rootFlags = ballot(treeletRoot != UINT_MAX);
    // END: Bottom-up Traversal
    //----------------------------------------------------------------
    
    // 有効なサブツリーそれぞれに関してtreeletの形成と最適化処理を行う。
    while (popcount(rootFlags) > 0) {
        uint rootBitIdx = largestOneBit32(rootFlags);
        bool rootThread = rootBitIdx == lid0;
        setZeroAt32(&rootFlags, rootBitIdx);// ルートを持つスレッドフラグを消しておく。
        
        local uint* curRoot = (local uint*)alignPtrL(extraLMemPool, sizeof(uint));
        if (rootThread)
            *curRoot = treeletRoot;
        barrier(CLK_LOCAL_MEM_FENCE);
        
        //----------------------------------------------------------------
        // Treelet Formation
        // サブツリーのルートから始めて、AABBの表面積の大きいtreelet leafをその子2つで置き換えるという処理を繰り返す。
        // 6回繰り返すことによって7個のtreelet leafを持ったtreeletを形成する。
        local float* TLSurfaceAreas = (local float*)alignPtrL(extraLMemPool, sizeof(float));
        local uint* TLIndices = (local uint*)alignPtrL(TLSurfaceAreas + 2 * TL_n - 1, sizeof(uint));
        
        uint nodeIdx = UINT_MAX;
        TLNodeType nodeType = TLNodeType_Invalid;
        float surfaceArea = 0;
        uint numLeaves = 0;
        AABB bbox;
        if (lid0 == 0) {
            nodeIdx = *curRoot;
            nodeType = TLNodeType_Leaf;
            surfaceArea = 1;
            numLeaves = numTotalLeaves[numPrimitives + nodeIdx];
        }
        
        for (int j = 0; j < TL_n - 1; ++j) {
            if (lid0 < 1 + 2 * j) {
                // 内部ノードになっている場合や、本当の葉ノードだった場合は拡張できないため、面積0として弾かせる。
                TLSurfaceAreas[lid0] = nodeType == TLNodeType_Leaf ? surfaceArea : 0;
                TLIndices[lid0] = nodeIdx;
            }
            barrier(CLK_LOCAL_MEM_FENCE);
            uint maxIndex = maxSurfaceAreaIndex(lid0, TLSurfaceAreas, TLIndices, 1 + 2 * j);
            
            if (nodeIdx == maxIndex && (nodeType & TLNodeType_ActualLeaf) != TLNodeType_ActualLeaf)
                nodeType = TLNodeType_Internal;
            if ((int)lid0 - 2 * j ==  1 || (int)lid0 - 2 * j == 2) {
                const global InternalNode* iNode = iNodes + maxIndex;
                uint lr = lid0 & 0x01;
                nodeIdx = iNode->c[lr];
                bool isActualLeaf = iNode->isLeaf[lr];
                nodeType = TLNodeType_Leaf | (isActualLeaf ? TLNodeType_ActualLeaf : 0);
                bbox = isActualLeaf ? (lNodes + nodeIdx)->bbox : (iNodes + nodeIdx)->bbox;
                surfaceArea = calcSurfaceAreaAABB(bbox);
                numLeaves = isActualLeaf ? 1 : numTotalLeaves[numPrimitives + nodeIdx];
            }
        }
        // END: Treelet Formation
        //----------------------------------------------------------------
    
        // 各スレッドが自身が何番目のTreelet Leafなのかを計算する。Treelet Leafじゃない場合はUCHAR_MAX。
        // ex) TLLeafFlags: 0001 1110 1010 0010, localID: 5
        //              AND 0000 0000 0001 1111
        //                  0000 0000 0000 0010 => popcount => 1
        bool TLLeafThread = (nodeType & TLNodeType_Leaf) == TLNodeType_Leaf;
        uint TLLeafFlags = ballot(TLLeafThread);
        uchar TLLeafIdx = TLLeafThread ? popcount(TLLeafFlags & ((1 << lid0) - 1)) : UCHAR_MAX;
        
        //----------------------------------------------------------------
        // AABB Calculation
        // 各スレッドが4パターンの組み合わせ、32のグループサイズで計128パターンのAABBの和の表面積を計算する。
        // スレッド内の4パターンでは、7つのAABBのうち5つ(インデックス2-6)の有無は共通となるので先に計算を済ませる。
        local AABB* TLLeafAABB = (local AABB*)alignPtrL(extraLMemPool, sizeof(point3));
        AABB commonAABB;
        commonAABB.min = (point3)(INFINITY, INFINITY, INFINITY);
        commonAABB.max = (point3)(-INFINITY, -INFINITY, -INFINITY);
        for (int j = TL_n - 1; j > 1; --j) {
            if (TLLeafIdx == j)
                *TLLeafAABB = bbox;
            barrier(CLK_LOCAL_MEM_FENCE);
            if ((lid0 >> (j - 2)) & 0x01)
                commonAABB = unionAABB(commonAABB, TLLeafAABB);
        }
        AABB unifiedAABB;
        // 共通AABBのみ。
        subsetSurfaceAreas[4 * lid0 + 0] = calcSurfaceAreaAABB(commonAABB);
        // インデックス0のAABBと共通AABBの和。
        if (TLLeafIdx == 0)
            *TLLeafAABB = bbox;
        barrier(CLK_LOCAL_MEM_FENCE);
        unifiedAABB = unionAABB(commonAABB, TLLeafAABB);
        subsetSurfaceAreas[4 * lid0 + 1] = calcSurfaceAreaAABB(unifiedAABB);
        // インデックス1のAABBと共通AABBの和。
        if (TLLeafIdx == 1)
            *TLLeafAABB = bbox;
        barrier(CLK_LOCAL_MEM_FENCE);
        unifiedAABB = unionAABB(commonAABB, TLLeafAABB);
        subsetSurfaceAreas[4 * lid0 + 2] = calcSurfaceAreaAABB(unifiedAABB);
        // インデックス0, 1のAABBと共通AABBの和。
        if (TLLeafIdx == 0)
            *TLLeafAABB = bbox;
        barrier(CLK_LOCAL_MEM_FENCE);
        unifiedAABB = unionAABB(unifiedAABB, TLLeafAABB);
        subsetSurfaceAreas[4 * lid0 + 3] = calcSurfaceAreaAABB(unifiedAABB);
        // END: AABB Calculation
        //----------------------------------------------------------------
        
        // Initialize costs of individual leaves
        // 単独のノードだけが分割される場合のSAHコストを初期化しておく。
        if (TLLeafIdx != UCHAR_MAX) {
            subsetSAHCosts[1 << TLLeafIdx] = SAHCosts[nodeIdx + ((nodeType & TLNodeType_ActualLeaf) == TLNodeType_ActualLeaf ? 0 : numPrimitives)];
            partitions[1 << TLLeafIdx] = 0;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        //----------------------------------------------------------------
        // Optimize every subset of leaves
        // 7つのtreelet leafの部分集合それぞれに対してSAHコストが最小となる分割を求める。
        // 基本的に部分集合のサイズの小さいものから順番に求めるが、SIMD利用率を向上させるため、サイズ2~5は予め作成済みのスケジュールに従って異なるサイズも一部並列に処理する。
        local uint* TLLeafNumLeaves = (local uint*)alignPtrL(extraLMemPool, sizeof(uint));
        if (TLLeafIdx != UCHAR_MAX)
            TLLeafNumLeaves[TLLeafIdx] = numLeaves;
        barrier(CLK_LOCAL_MEM_FENCE);
        for (int round = 0; round < NUM_OptRounds; ++round) {
            char subset = schedule[SIMD_Width * round + lid0];
            if (subset != 0) {
                float c_s = INFINITY;
                char p_s = 0;
                
                // Try each way of partitionings the leaves
                // ある部分集合に対する最適な分割を求める。
                char delta = (subset - 1) & subset;
                char p = (-delta) & subset;
                do {
                    float c = subsetSAHCosts[(uchar)p] + subsetSAHCosts[(uchar)(subset ^ p)];
                    if (c < c_s) {
                        c_s = c;
                        p_s = p;
                    }
                    p = (p - delta) & subset;
                } while (p != 0);
                
                // ある部分集合に対する最終的なSAHコストと分割を記録する。
                uint numLeaves_s = 0;
                for (int j = 0; j < TL_n; ++j)
                    numLeaves_s += (subset >> j) & 0x01 ? TLLeafNumLeaves[j] : 0;
                float surfaceArea = subsetSurfaceAreas[(uchar)subset];
                subsetSAHCosts[(uchar)subset] = fmin(C_i * surfaceArea + c_s, C_t * surfaceArea * numLeaves_s);
                partitions[(uchar)subset] = p_s;
            }
            
            barrier(CLK_LOCAL_MEM_FENCE);
        }
        
        local float* reductionBuffer = (local float*)alignPtrL(extraLMemPool, sizeof(float));
        
        // size: 6, subset: 7, active: 28
        // 1つの部分集合(31通りの分割)を4つのスレッドで処理する。1スレッドあたり8種類の分割コストを計算する。
        // 4番目のスレッドの最後1つは余分な(重複した)計算となるが、そのためだけにif文を追加するのも微妙？
        float c_s = INFINITY;
        char p_s = 0;
        char bit = lid0 >> 2;// 0 ~ 6
        char subset = FullSet ^ (1 << bit);// 1111110, 1111101, 1111011, ... 0111111
        uint numLeaves_s = 0;
        if (lid0 < 28) {
            // 部分集合毎のLeafノードの数を求めて、同じ部分集合を処理するスレッド間で共有しておく。
            for (int j = 0; j < TL_n; ++j)
                numLeaves_s += (subset >> j) & 0x01 ? TLLeafNumLeaves[j] : 0;
            
            char lowMask = (1 << bit) - 1;// 000000, 000001, 000011, ... 111111
            for (char j = 0; j < 8; ++j) {
                // ex) bit: 2 => 1111011
                char p = 8 * (lid0 & 0x03) + j + 1;// 1 ~ 32
                p = ((p & ~lowMask) << 1) | (p & lowMask);
                float c = subsetSAHCosts[(uchar)p] + subsetSAHCosts[(uchar)(subset ^ p)];
                if (c < c_s) {
                    c_s = c;
                    p_s = p;
                }
            }
        }
        reductionBuffer[lid0] = c_s;
        barrier(CLK_LOCAL_MEM_FENCE);
        // 4スレッド間でreductionを行い、最小コストを求める。最小のコストに一致するスレッドが保持する分割を記録する。
        if ((lid0 & 0x01) == 0)
            reductionBuffer[lid0] = fmin(reductionBuffer[lid0], reductionBuffer[lid0 + 1]);
        barrier(CLK_LOCAL_MEM_FENCE);
        if ((lid0 & 0x03) == 0)
            reductionBuffer[lid0] = fmin(reductionBuffer[lid0], reductionBuffer[lid0 + 2]);
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lid0 < 28 && c_s == reductionBuffer[lid0 & ~0x03]) {
            float surfaceArea = subsetSurfaceAreas[(uchar)subset];
            subsetSAHCosts[(uchar)subset] = fmin(C_i * surfaceArea + c_s, C_t * surfaceArea * numLeaves_s);
            partitions[(uchar)subset] = p_s;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        
        // size: 7, subset: 1, active: 32
        // 部分集合はそもそも1つしか存在しない。63通り(0000001 ~ 0111111)の分割が考えられるため、各スレッドが2つの分割コストを計算する。
        local uint* numLeavesTreelet = (local uint*)alignPtrL(extraLMemPool, sizeof(uint));
        if (lid0 == 0) {
            *numLeavesTreelet = numLeaves;
            subsetSAHCosts[0] = INFINITY;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        numLeaves_s = *numLeavesTreelet;
        char p = 2 * lid0 + 0;
        float c_s0 = subsetSAHCosts[p] + subsetSAHCosts[FullSet ^ p];
        p += 1;
        float c_s1 = subsetSAHCosts[p] + subsetSAHCosts[FullSet ^ p];
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
            float surfaceArea = subsetSurfaceAreas[FullSet];
            subsetSAHCosts[FullSet] = fmin(C_i * surfaceArea + c_s, C_t * surfaceArea * numLeaves_s);
            partitions[FullSet] = p_s;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        // END: Optimize every subset of leaves
        //----------------------------------------------------------------
        
        // 各スレッドが自身が何番目のTreelet Internalなのかを計算する。Treelet Internalじゃない場合はUCHAR_MAX。
        bool TLIntThread = (nodeType & TLNodeType_Internal) == TLNodeType_Internal;
        uint TLIntFlags = ballot(TLIntThread);
        uchar TLIntIdx = TLIntThread ? popcount(TLIntFlags & ((1 << lid0) - 1)) : UCHAR_MAX;
        
        //----------------------------------------------------------------
        // Reconstruction
        // Optimizationステップで記録された分割方法を基に木構造を再構築する。
        // Treelet Leaf Nodeには一切触れず、Treelet Internal Nodeも接続情報を変えるだけ。
        local char* partitionPair = (local char*)alignPtrL(extraLMemPool, sizeof(char));
        local bool* childIsLeaf = (local bool*)alignPtrL(partitionPair + 2, sizeof(bool));
        local uint* childIdx = (local uint*)alignPtrL(childIsLeaf, sizeof(uint));
        subset = FullSet;
        uint c[2] = {0, 0};
        bool isLeaf[2] = {false, false};
        uchar nextIntIdx = 1;
        for (int i = 0; i < TL_n - 1; ++i) {
            if (TLIntIdx == i) {
                partitionPair[0] = partitions[subset];
                partitionPair[1] = subset ^ partitionPair[0];
            }
            barrier(CLK_LOCAL_MEM_FENCE);
            
            for (int j = 0; j < 2; ++j) {
                // 分割後の要素数に応じてTreelet Internal/Leaf Nodeのインデックスを取得する。
                if (popcount(partitionPair[j]) > 1) {
                    // 分割後の部分集合を担当するスレッドに部分集合の内容を伝えておく。
                    if (TLIntIdx == nextIntIdx) {
                        *childIdx = nodeIdx;
                        subset = partitionPair[j];
                    }
                    *childIsLeaf = false;
                    ++nextIntIdx;
                }
                else {
                    if (TLLeafIdx == popcount(partitionPair[j] - 1)) {
                        *childIdx = nodeIdx;
                        *childIsLeaf = (nodeType & TLNodeType_ActualLeaf) == TLNodeType_ActualLeaf;
                    }
                }
                barrier(CLK_LOCAL_MEM_FENCE);
                
                if (TLIntIdx == i) {
                    c[j] = *childIdx;
                    isLeaf[j] = *childIsLeaf;
                }
            }
        }
        
        local bool* temp = (local bool*)alignPtrL(extraLMemPool, sizeof(bool));
        if (rootThread)
            *temp = false;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (rootThread && treeletRoot == 3998)
            *temp = true;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (*temp) {
            printf("Node: %5u%c, c0: %5u%c, c1: %5u%c\n",
                   nodeIdx, (nodeType & TLNodeType_ActualLeaf) == TLNodeType_ActualLeaf ? 'L' : ' ',
                   c[0], isLeaf[0] ? 'L' : ' ',
                   c[1], isLeaf[1] ? 'L' : ' ');
        }
        // END: Reconstruction
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
