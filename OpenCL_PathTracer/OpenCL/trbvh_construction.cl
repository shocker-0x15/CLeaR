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
    TLNodeType_Leaf = 1 << 1,
    TLNodeType_ActualLeaf = 1 << 2,
} TLNodeType;

#define TL_n 7
#define TL_size (TL_n * 2 - 1)
#define FullSet 0b1111111
#define GAMMA 7
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
inline uint __ballot(local void* mem, uint localID, bool pred);
inline float __shfl_f32(local float* mem, uint localID, float value, uint srcLane, uint width);
inline void shfl_2_f32(local float* mem, uint localID, float value,
                       uint srcLane0, uint srcLane1, float* v0, float* v1, uint width);
inline void shfl_2_ui32(local uint* mem, uint localID, uint value,
                        uint srcLane0, uint srcLane1, uint* v0, uint* v1, uint width);
inline void shfl_2_bool(local bool* mem, uint localID, bool value,
                        uint srcLane0, uint srcLane1, bool* v0, bool* v1, uint width);
inline uint work_group_broadcast_ui32(local uint* mem, uint localID, uint value, uint srcLane);

inline float calcSurfaceAreaAABB(const AABB bb);
inline AABB unionAABB(const AABB bb0, const AABB bb1);

kernel void calcNodeAABBs_SAHCosts(volatile global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                   const global uint* parentIdxs, volatile global uint* numTotalLeaves, volatile global float* SAHCosts);

inline uint largestOneBit32(uint value);
inline void setZeroAt32(uint* value, uint index);
inline AABB work_group_broadcast_AABB(local AABB* mem, const AABB value, bool pred);
uint maxSurfaceAreaIndex(uint lid, local float* TLSurfaceAreas, local uint* TLIndices, uint numElems);
kernel void treeletRestructuring(volatile global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                 volatile global uint* parentIdxs, volatile global uint* numTotalLeaves, volatile global float* SAHCosts,
                                 uint globalOptRound);

//----------------------------------------------------------------

// ローカルメモリのポインターをbytesにアラインする。
inline local uchar* alignPtrL(const local void* ptr, uintptr_t bytes) {
    return (local uchar*)(((uintptr_t)ptr + (bytes - 1)) & ~(bytes - 1));
}

// CUDAの__ballot風の関数。
inline uint __ballot(local void* mem, uint localID, bool pred) {
    if (localID == 0)
        *(local uint*)mem = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    if (pred)
        atomic_or((local uint*)mem, 1 << localID);
    barrier(CLK_LOCAL_MEM_FENCE);
    uint ret = *(local uint*)mem;
    barrier(CLK_LOCAL_MEM_FENCE);
    return ret;
}

// CUDAの__shfl風の関数。
inline float __shfl_f32(local float* mem, uint localID, float value, uint srcLane, uint width) {
    if (localID < width)
        mem[localID] = value;
    barrier(CLK_LOCAL_MEM_FENCE);
    float ret = mem[srcLane];
    barrier(CLK_LOCAL_MEM_FENCE);
    return ret;
}

// スレッドグループ中の2つのfloat値を取得する。
inline void shfl_2_f32(local float* mem, uint localID, float value,
                        uint srcLane0, uint srcLane1, float* v0, float* v1, uint width) {
    if (localID < width)
        mem[localID] = value;
    barrier(CLK_LOCAL_MEM_FENCE);
    float _v0 = mem[srcLane0];
    float _v1 = mem[srcLane1];
    barrier(CLK_LOCAL_MEM_FENCE);
    *v0 = _v0;
    *v1 = _v1;
}

// スレッドグループ中の2つのuint値を取得する。
inline void shfl_2_ui32(local uint* mem, uint localID, uint value,
                         uint srcLane0, uint srcLane1, uint* v0, uint* v1, uint width) {
    if (localID < width)
        mem[localID] = value;
    barrier(CLK_LOCAL_MEM_FENCE);
    uint _v0 = mem[srcLane0];
    uint _v1 = mem[srcLane1];
    barrier(CLK_LOCAL_MEM_FENCE);
    *v0 = _v0;
    *v1 = _v1;
}

// スレッドグループ中の2つのbool値を取得する。
inline void shfl_2_bool(local bool* mem, uint localID, bool value,
                         uint srcLane0, uint srcLane1, bool* v0, bool* v1, uint width) {
    if (localID < width)
        mem[localID] = value;
    barrier(CLK_LOCAL_MEM_FENCE);
    bool _v0 = mem[srcLane0];
    bool _v1 = mem[srcLane1];
    barrier(CLK_LOCAL_MEM_FENCE);
    *v0 = _v0;
    *v1 = _v1;
}

// OpenCL 2.0のwork_group_broadcast()風の関数。
inline uint work_group_broadcast_ui32(local uint* mem, uint localID, uint value, uint srcLane) {
    if (localID == srcLane)
        *mem = value;
    barrier(CLK_LOCAL_MEM_FENCE);
    uint ret = *mem;
    barrier(CLK_LOCAL_MEM_FENCE);
    return ret;
}


// AABBの表面積(の半分)を計算する。
inline float calcSurfaceAreaAABB(const AABB bb) {
    vector3 edge = bb.max - bb.min;
    return edge.x * edge.y + edge.y * edge.z + edge.z * edge.x;
}

// AABBの和を計算する。
inline AABB unionAABB(const AABB bb0, const AABB bb1) {
    AABB ret;
    ret.min = fmin(bb0.min, bb1.min);
    ret.max = fmax(bb0.max, bb1.max);
    return ret;
}


// 各ノードのAABB、プリミティブの数、SAHコストを計算する。leaf nodeからルートへの経路単位で並列に計算される。
// グローバルメモリへの書き込みがキャッシュされてしまうとスレッド全体で一貫性が無くなってしまうのでvolatile属性をつける。
kernel void calcNodeAABBs_SAHCosts(volatile global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                   const global uint* parentIdxs, volatile global uint* numTotalLeaves, volatile global float* SAHCosts) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
    
    volatile global InternalNode* iNodes = (volatile global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    
    const global LeafNode* lNode = lNodes + gid0;
    AABB bbox = lNode->bbox;
    uint numLeaves = 1;
    float SAHCost = C_t * calcSurfaceAreaAABB(bbox);
    
    uint selfIdx = gid0;
    uint pIdx = parentIdxs[selfIdx];
    SAHCosts[gid0] = SAHCost;
    parentIdxs += numPrimitives;
    
    // InternalNodeを繰り返しルートに向けて登る。
    // 2回目のアクセスを担当するスレッドがAABBの和をとることによって、そのノードの子全てが計算済みであることを保証する。
    while (atomic_inc(counters + pIdx) == 1) {
        volatile global InternalNode* iNode = iNodes + pIdx;
        
        bool leftIsSelf = iNode->c[0] == selfIdx;// LBVH構築後は2つのインデックスが同じ値(ただしどちらかはLeaf)という状態は無い？
        uint otherIdx = iNode->c[leftIsSelf];
        bool otherIsLeaf = iNode->isLeaf[leftIsSelf];
        
        const AABB otherBBox = otherIsLeaf ? (lNodes + otherIdx)->bbox : (iNodes + otherIdx)->bbox;
        iNode->bbox = bbox = unionAABB(bbox, otherBBox);
        
        numLeaves += otherIsLeaf ? 1 : numTotalLeaves[otherIdx];
        numTotalLeaves[pIdx] = numLeaves;
        
        float area = calcSurfaceAreaAABB(bbox);
        SAHCost = fmin(C_i * area + (SAHCost + SAHCosts[otherIdx + (otherIsLeaf ? 0 : numPrimitives)]), C_t * area * numLeaves);
        SAHCosts[pIdx + numPrimitives] = SAHCost;
        
        if (pIdx == 0)
            return;
        
        selfIdx = pIdx;
        pIdx = parentIdxs[selfIdx];
    }
}


// 1になっているビット中で最大のインデックスを返す。
inline uint largestOneBit32(uint value) {
    return 31 - clz(value);
}

// indexで指定したビットをゼロにセットする。
inline void setZeroAt32(uint* value, uint index) {
    *value &= ~(1 << index);
}

inline AABB work_group_broadcast_AABB(local AABB* mem, const AABB value, bool pred) {
    if (pred)
        *mem = value;
    barrier(CLK_LOCAL_MEM_FENCE);
    AABB ret = *mem;
    barrier(CLK_LOCAL_MEM_FENCE);
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
    uint ret = TLIndices[0];
    barrier(CLK_LOCAL_MEM_FENCE);
    return ret;
}

kernel void treeletRestructuring(volatile global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives,
                                 volatile global uint* parentIdxs, volatile global uint* numTotalLeaves, volatile global float* SAHCosts,
                                 uint globalOptRound) {
    const uint lid0 = get_local_id(0);
    volatile global InternalNode* iNodes = (volatile global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    
    // Keplerアーキテクチャーだとローカルメモリ使用量は768B以下が望ましい。
    local float SA_SAHCosts[128];
    local float* subsetSurfaceAreas = SA_SAHCosts;
    local float* subsetSAHCosts = SA_SAHCosts;
    local char partitions[128];
    
    // OpenCL 1.2以下では(少なくとも明示的に)ローカルメモリを使わないと、reductionなどが出来ないので元論文より余分な領域の確保が必要。
    // ballotなどを考えると2.0/2.1でも必要かも。
    local uchar extraLMemPool[128] __attribute__((aligned(16)));
    
    
    uint selfIdx = get_global_id(0);
    uint pIdx = parentIdxs[selfIdx];
    bool survive = selfIdx < numPrimitives;

    uint dbgStep = UINT_MAX;
    while (true) {
        ++dbgStep;
        uint treeletRoot = UINT_MAX;
        
        //----------------------------------------------------------------
        // Bottom-up Traversal
        // 各スレッドがleaf nodeからスタートしてtreeletの形成に十分なサイズのサブツリーを含むまで木を登り、ルートのインデックスを記録する。
        if (survive) {
            while ((survive = atomic_inc(counters + pIdx) == (3u + 2u * globalOptRound)) == true) {
                selfIdx = pIdx;
                pIdx = parentIdxs[numPrimitives + selfIdx];
                
                if (numTotalLeaves[selfIdx] >= ((uint)GAMMA << globalOptRound)) {
                    treeletRoot = selfIdx;
                    break;
                }
            }
        }
        if (__ballot(extraLMemPool, lid0, survive) == 0)
            return;
        uint rootFlags = __ballot(extraLMemPool, lid0, treeletRoot != UINT_MAX);
        // END: Bottom-up Traversal
        //----------------------------------------------------------------
        
        // 有効なサブツリーそれぞれに関してtreeletの形成と最適化処理を行う。
        while (popcount(rootFlags) > 0) {
            uint rootThreadLID = largestOneBit32(rootFlags);
            setZeroAt32(&rootFlags, rootThreadLID);// ルートを持つスレッドのフラグを消しておく。
            
            uint curRoot = work_group_broadcast_ui32((local uint*)extraLMemPool, lid0, treeletRoot, rootThreadLID);
            
            // Per Node Values
            uint nodeIdx = UINT_MAX;
            TLNodeType nodeType = TLNodeType_Invalid;
            uint numLeaves = 0;
            AABB bbox;
            
            //----------------------------------------------------------------
            // Treelet Formation
            // サブツリーのルートから始めて、AABBの表面積の大きいtreelet leafをその子2つで置き換えるという処理を繰り返す。
            // 6回繰り返すことによって7個のtreelet leafを持ったtreeletを形成する。
            local float* TLSurfaceAreas = (local float*)alignPtrL(extraLMemPool, sizeof(float));
            local uint* TLIndices = (local uint*)alignPtrL(TLSurfaceAreas + TL_size, sizeof(uint));
            
            float surfaceArea = 0;
            if (lid0 == 0) {
                nodeIdx = curRoot;
                nodeType = TLNodeType_Leaf;
                surfaceArea = 1;
                numLeaves = numTotalLeaves[nodeIdx];
            }
            
            for (int j = 0; j < TL_n - 1; ++j) {
                if ((int)lid0 < 1 + 2 * j) {
                    // 内部ノードになっている場合や、本当の葉ノードだった場合は拡張できないため、面積0として弾かせる。
                    TLSurfaceAreas[lid0] = (nodeType == TLNodeType_Leaf) ? surfaceArea : 0;
                    TLIndices[lid0] = nodeIdx;
                }
                barrier(CLK_LOCAL_MEM_FENCE);
                uint maxIndex = maxSurfaceAreaIndex(lid0, TLSurfaceAreas, TLIndices, 1 + 2 * j);
                
                if (nodeIdx == maxIndex && (nodeType & TLNodeType_ActualLeaf) != TLNodeType_ActualLeaf)
                    nodeType = TLNodeType_Internal;
                if ((int)lid0 - 2 * j ==  1 || (int)lid0 - 2 * j == 2) {
                    volatile const global InternalNode* iNode = (volatile const global InternalNode*)iNodes + maxIndex;
                    uint lr = lid0 & 0x01;
                    nodeIdx = iNode->c[lr];
                    bool isActualLeaf = iNode->isLeaf[lr];
                    nodeType = TLNodeType_Leaf | (isActualLeaf ? TLNodeType_ActualLeaf : 0);
                    numLeaves = isActualLeaf ? 1 : numTotalLeaves[nodeIdx];
                    bbox = isActualLeaf ? (lNodes + nodeIdx)->bbox : (iNodes + nodeIdx)->bbox;
                    surfaceArea = calcSurfaceAreaAABB(bbox);
                }
            }
            // END: Treelet Formation
            //----------------------------------------------------------------
            
//            if (__ballot(extraLMemPool, lid0, nodeIdx == 1354) != 0) {
//                printf("TF %u-%u-%2u-%4u: lid: %2u, node: %5u %u|(%10.6f, %10.6f, %10.6f), (%10.6f, %10.6f, %10.6f)\n", globalOptRound, dbgStep, get_group_id(0), curRoot, lid0,
//                       nodeIdx, nodeType,
//                       bbox.min.x, bbox.min.y, bbox.min.z, bbox.max.x, bbox.max.y, bbox.max.z);
//            }
            
            // 各スレッドが自身が何番目のTreelet Leafなのかを計算する。Treelet Leafじゃない場合はUCHAR_MAX。
            // ex) TLLeafFlags: 0001 1110 1010 0010, localID: 5
            //              AND 0000 0000 0001 1111
            //                  0000 0000 0000 0010 => popcount => 1
            bool TLLeafThread = (nodeType & TLNodeType_Leaf) == TLNodeType_Leaf;
            uint TLLeafFlags = __ballot(extraLMemPool, lid0, TLLeafThread);
            uchar TLLeafIdx = TLLeafThread ? popcount(TLLeafFlags & ((1 << lid0) - 1)) : UCHAR_MAX;
            
            //----------------------------------------------------------------
            // AABB Calculation
            // 各スレッドが4パターンの組み合わせ、32のグループサイズで計128パターンのAABBの和の表面積を計算する。
            // スレッド内の4パターンでは、7つのAABBのうち5つ(インデックス2-6)の有無は共通となるので先に計算を済ませる。
            AABB commonAABB, TLLeafAABB;
            commonAABB.min = (point3)(INFINITY, INFINITY, INFINITY);
            commonAABB.max = (point3)(-INFINITY, -INFINITY, -INFINITY);
            for (int j = TL_n - 1; j > 1; --j) {
                TLLeafAABB = work_group_broadcast_AABB((local AABB*)extraLMemPool, bbox, TLLeafIdx == j);
                if ((lid0 >> (j - 2)) & 0x01)
                    commonAABB = unionAABB(commonAABB, TLLeafAABB);
            }
            AABB unifiedAABB = commonAABB;
            // 共通AABBのみ。
            subsetSurfaceAreas[4 * lid0 + 0] = calcSurfaceAreaAABB(unifiedAABB);
            // インデックス0のAABBと共通AABBの和。
            TLLeafAABB = work_group_broadcast_AABB((local AABB*)extraLMemPool, bbox, TLLeafIdx == 0);
            unifiedAABB = unionAABB(commonAABB, TLLeafAABB);
            subsetSurfaceAreas[4 * lid0 + 1] = calcSurfaceAreaAABB(unifiedAABB);
            // インデックス1のAABBと共通AABBの和。
            TLLeafAABB = work_group_broadcast_AABB((local AABB*)extraLMemPool, bbox, TLLeafIdx == 1);
            unifiedAABB = unionAABB(commonAABB, TLLeafAABB);
            subsetSurfaceAreas[4 * lid0 + 2] = calcSurfaceAreaAABB(unifiedAABB);
            // インデックス0, 1のAABBと共通AABBの和。
            TLLeafAABB = work_group_broadcast_AABB((local AABB*)extraLMemPool, bbox, TLLeafIdx == 0);
            unifiedAABB = unionAABB(unifiedAABB, TLLeafAABB);
            subsetSurfaceAreas[4 * lid0 + 3] = calcSurfaceAreaAABB(unifiedAABB);
            // END: AABB Calculation
            //----------------------------------------------------------------
            
            // Initialize costs of individual leaves
            // 単独のノードだけが分割される場合のSAHコストを初期化しておく。
            if (TLLeafIdx != UCHAR_MAX) {
                bool isActualLeaf = (nodeType & TLNodeType_ActualLeaf) == TLNodeType_ActualLeaf;
                subsetSAHCosts[1 << TLLeafIdx] = SAHCosts[nodeIdx + (isActualLeaf ? 0 : numPrimitives)];
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
            char zeroBit = lid0 >> 2;// 0 ~ 6
            char subset = FullSet ^ (1 << zeroBit);// 1111110, 1111101, 1111011, ... 0111111
            uint numLeaves_s = 0;
            if (lid0 < 28) {
                // 部分集合毎のプリミティブの数を求める。
                for (int j = 0; j < TL_n; ++j)
                    numLeaves_s += TLLeafNumLeaves[j];
                numLeaves_s -= TLLeafNumLeaves[zeroBit];
                
                char lowMask = (1 << zeroBit) - 1;// 000000, 000001, 000011, ... 111111
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
            barrier(CLK_LOCAL_MEM_FENCE);
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
            barrier(CLK_LOCAL_MEM_FENCE);
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
            uint TLIntFlags = __ballot(extraLMemPool, lid0, TLIntThread);
            uchar TLIntIdx = TLIntThread ? popcount(TLIntFlags & ((1 << lid0) - 1)) : UCHAR_MAX;
            
            //----------------------------------------------------------------
            // Reconstruction
            // Optimizationステップで記録された分割方法を基に木構造を再構築する。
            // Treelet Leaf Nodeには一切触れず、Treelet Internal Nodeの接続情報・AABBを変更する。
            // SAHコストに改善が無い場合はスキップする。
            if ((subsetSAHCosts[FullSet] / SAHCosts[curRoot + numPrimitives]) < 1.0f) {
                local char* partitionPair = (local char*)alignPtrL(extraLMemPool, sizeof(char));
                local uchar* childLocalID = (local uchar*)alignPtrL(partitionPair + 2, sizeof(uchar));
                local uint* parentIdx = (local uint*)alignPtrL(childLocalID + 1, sizeof(uint));
                char subset = FullSet;
                uchar cLID[2] = {0, 0};
                uchar nextIntIdx = 1;
                for (int i = 0; i < TL_n - 1; ++i) {
                    if (TLIntIdx == i) {
                        partitionPair[0] = partitions[subset];
                        partitionPair[1] = subset ^ partitionPair[0];
                        *parentIdx = nodeIdx;
                    }
                    barrier(CLK_LOCAL_MEM_FENCE);
                    
                    for (int j = 0; j < 2; ++j) {
                        // 分割後の要素数に応じてTreelet Internal/Leaf Nodeのインデックスを取得する。
                        if (popcount(partitionPair[j]) > 1) {
                            if (TLIntIdx == nextIntIdx) {
                                // 空いているInternal NodeスレッドのローカルIDを教えてもらう。
                                *childLocalID = (uchar)lid0;
                                // 分割後の部分集合を担当するスレッドに部分集合の内容を伝えておく。
                                subset = partitionPair[j];
                                parentIdxs[nodeIdx + numPrimitives] = *parentIdx;
                            }
                            ++nextIntIdx;
                        }
                        else {
                            // 分割フラグのビットに合うLeaf NodeスレッドのローカルIDを教えてもらう。
                            if (TLLeafIdx == popcount(partitionPair[j] - 1)) {
                                *childLocalID = (uchar)lid0;
                                bool isActualLeaf = (nodeType & TLNodeType_ActualLeaf) == TLNodeType_ActualLeaf;
                                parentIdxs[nodeIdx + (isActualLeaf ? 0 : numPrimitives)] = *parentIdx;
                            }
                        }
                        barrier(CLK_LOCAL_MEM_FENCE);
                        
                        if (TLIntIdx == i)
                            cLID[j] = *childLocalID;
                        barrier(CLK_LOCAL_MEM_FENCE);
                    }
                }
                if (TLIntIdx != UCHAR_MAX)
                    SAHCosts[nodeIdx + numPrimitives] = subsetSAHCosts[subset];
                
                local bool* validAABBs = (local bool*)alignPtrL(extraLMemPool, sizeof(bool));
                local float* AABBCoords = (local float*)alignPtrL(validAABBs + TL_size, sizeof(float));
                local uint* TLNumLeaves = (local uint*)alignPtrL(validAABBs + TL_size, sizeof(uint));
                
                // 子ノードが全て計算済みであればAABBとプリミティブの総数を計算する。ローカルメモリが潤沢には使えないためAABBは次元毎にmin/maxを計算する。
                if (lid0 < TL_size)
                    validAABBs[lid0] = (nodeType & TLNodeType_Leaf) == TLNodeType_Leaf;
                barrier(CLK_LOCAL_MEM_FENCE);
                while (true) {
                    AABB newAABB;
                    float v0, v1;
                    
                    shfl_2_f32(AABBCoords, lid0, bbox.min.x, cLID[0], cLID[1], &v0, &v1, TL_size);
                    newAABB.min.x = fmin(v0, v1);
                    shfl_2_f32(AABBCoords, lid0, bbox.min.y, cLID[0], cLID[1], &v0, &v1, TL_size);
                    newAABB.min.y = fmin(v0, v1);
                    shfl_2_f32(AABBCoords, lid0, bbox.min.z, cLID[0], cLID[1], &v0, &v1, TL_size);
                    newAABB.min.z = fmin(v0, v1);
                    
                    shfl_2_f32(AABBCoords, lid0, bbox.max.x, cLID[0], cLID[1], &v0, &v1, TL_size);
                    newAABB.max.x = fmax(v0, v1);
                    shfl_2_f32(AABBCoords, lid0, bbox.max.y, cLID[0], cLID[1], &v0, &v1, TL_size);
                    newAABB.max.y = fmax(v0, v1);
                    shfl_2_f32(AABBCoords, lid0, bbox.max.z, cLID[0], cLID[1], &v0, &v1, TL_size);
                    newAABB.max.z = fmax(v0, v1);
                    
                    uint n0, n1;
                    shfl_2_ui32(TLNumLeaves, lid0, numLeaves, cLID[0], cLID[1], &n0, &n1, TL_size);
                    
                    bool valid = validAABBs[cLID[0]] & validAABBs[cLID[1]];
                    barrier(CLK_LOCAL_MEM_FENCE);
                    if (valid && TLIntIdx != UCHAR_MAX) {
                        numLeaves = n0 + n1;
                        numTotalLeaves[nodeIdx] = numLeaves;
                        bbox = newAABB;
                        
                        validAABBs[lid0] = true;
                    }
                    barrier(CLK_LOCAL_MEM_FENCE);
                    if (validAABBs[0])
                        break;
                }
                
                InternalNode newINode;
                newINode.bbox = bbox;
                shfl_2_ui32((local uint*)extraLMemPool, lid0, nodeIdx, cLID[0], cLID[1], &newINode.c[0], &newINode.c[1], TL_size);
                shfl_2_bool((local bool*)extraLMemPool, lid0, (nodeType & TLNodeType_ActualLeaf) == TLNodeType_ActualLeaf,
                            cLID[0], cLID[1], &newINode.isLeaf[0], &newINode.isLeaf[1], TL_size);
                if (TLIntIdx != UCHAR_MAX) {
                    iNodes[nodeIdx] = newINode;
//                    if (nodeIdx == 1354) {
//                        printf("R %u-%u-%2u-%4u: lid: %2u, node: %5u %u, %2u, %2u|(%10.6f, %10.6f, %10.6f), (%10.6f, %10.6f, %10.6f)\n", globalOptRound, dbgStep, get_group_id(0), curRoot, lid0,
//                               nodeIdx, nodeType, cLID[0], cLID[1],
//                               bbox.min.x, bbox.min.y, bbox.min.z, bbox.max.x, bbox.max.y, bbox.max.z);
//                    }
                }
                
//                if (__ballot(extraLMemPool, lid0, nodeIdx == 1354) != 0) {
//                    printf("R %u-%u-%2u-%4u: lid: %2u, node: %5u %u, %2u, %2u|(%10.6f, %10.6f, %10.6f), (%10.6f, %10.6f, %10.6f)\n", globalOptRound, dbgStep, get_group_id(0), curRoot, lid0,
//                           nodeIdx, nodeType, cLID[0], cLID[1],
//                           bbox.min.x, bbox.min.y, bbox.min.z, bbox.max.x, bbox.max.y, bbox.max.z);
//                }
            }
            // END: Reconstruction
            //----------------------------------------------------------------
            
            if (curRoot == 0)
                return;
        }// End of Valid Treelet Roots Loop
    }// End of Bottom-up traversal Loop
}
