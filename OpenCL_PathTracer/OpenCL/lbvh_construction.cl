//
//  bvh_construction.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#define printfF3(f3) printf(#f3": %f, %f, %f\n", (f3).x, (f3).y, (f3).z)
#define printSize(t) printf("sizeof("#t"): %u\n", sizeof(t))
#define convertPtrCG(dstT, ptr, offset) (const global dstT*)((const global uchar*)ptr + (offset))

typedef float3 point3;

// 56bytes
typedef struct __attribute__((aligned(4))) {
    uint p0, p1, p2;
    uint vn0, vn1, vn2;
    uint vt0, vt1, vt2;
    uint uv0, uv1, uv2;
    uint alphaTexPtr;
    ushort matPtr, lightPtr;
} Face;

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
                                    uint numBlocks, global uint* histograms, global ushort* localOffsets);

kernel void globalScatter(const global uchar* radixDigits, uint numPrimitives, const global uint* globalOffsets, const global ushort* localOffsets,
                          const global uint* indicesSrc, global uint* indicesDst);

char _numCommonBits(const global uint3* mortonCodes, const global uint* indices, uint numPrimitives, uint bitsPerDim,
                    const uint3* mc_i, int _idx0, int _idx1);
inline char signInt8(char val);
kernel void constructBinaryRadixTree(const global uint3* mortonCodes, uint bitsPerDim, const global uchar* AABBs, const global uint* indices, uint numPrimitives,
                                     global uchar* iNodes, global uchar* lNodes, global uint* parentIdxs, global uint* counters);

kernel void calcNodeAABBs(volatile global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs);

//----------------------------------------------------------------

// 各プリミティブのAABBとその中心を計算する。
// 1スレッドが1プリミティブに対応する。
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
    *((global AABB*)AABBs + gid0) = box;
}



#ifndef LOCAL_SIZE_UNIFY_AABBS
#define LOCAL_SIZE_UNIFY_AABBS 128
#endif

// ローカルな範囲でAABBの和を計算する。
kernel void unifyAABBs(const global uchar* AABBs, uint numAABBs, global uchar* unifiedAABBs) {
    const global AABB* gAABBs = (const global AABB*)AABBs;
    global AABB* gUnifiedAABBs = (global AABB*)unifiedAABBs;
    
    local point3 lmin[LOCAL_SIZE_UNIFY_AABBS];
    local point3 lmax[LOCAL_SIZE_UNIFY_AABBS];
    
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    // ローカル変数に各AABBの情報をコピー。
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
    for (uint i = LOCAL_SIZE_UNIFY_AABBS >> 1; i > 0; i >>= 1) {
        if (lid0 < i) {
            lmin[lid0] = fmin(lmin[lid0], lmin[lid0 + i]);
            lmax[lid0] = fmax(lmax[lid0], lmax[lid0 + i]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // AABBの和をグローバル変数に書き出す。
    if (lid0 == 0) {
        AABB unifiedAABB;
        unifiedAABB.min = lmin[0];
        unifiedAABB.max = lmax[0];
        gUnifiedAABBs[get_group_id(0)] = unifiedAABB;
    }
}



// 各プリミティブに関して、対応するAABBの中心点を基準にモートンコードを計算する。
// ソート前のインデックスも出力しておく。
// 1スレッドが1プリミティブに対応する。
kernel void calcMortonCodes(const global uchar* AABBs, uint numPrimitives,
                            point3 minEntireAABB, point3 sizeEntireAABB, uint numDivs,
                            global uint3* mortonCodes, global uint* indices) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
        
    const AABB box = *((const global AABB*)AABBs + gid0);
    
    point3 normPos = ((box.min + box.max) * 0.5f - minEntireAABB) / sizeEntireAABB;
    mortonCodes[gid0] = min(convert_uint3_rtz(normPos * numDivs), numDivs - 1);
    indices[gid0] = gid0;
}


#ifndef LOCAL_SORT_SIZE
#define LOCAL_SORT_SIZE 64
#endif

// ローカル変数のprefix sumを計算する。
ushort scan(local ushort* bitArray, const uint lid) {
    for (uint i = 2; i <= LOCAL_SORT_SIZE; i <<= 1) {
        uint idx = (lid + 1) * i - 1;
        if (idx < LOCAL_SORT_SIZE)
            bitArray[idx] += bitArray[idx - (i >> 1)];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0)
        bitArray[LOCAL_SORT_SIZE - 1] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint i = LOCAL_SORT_SIZE; i >= 2; i >>= 1) {
        uint idx1 = (lid + 1) * i - 1;
        if (idx1 < LOCAL_SORT_SIZE) {
            uint idx0 = idx1 - (i >> 1);
            ushort temp = bitArray[idx0];
            bitArray[idx0] = bitArray[idx1];
            bitArray[idx1] += temp;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    return bitArray[lid];
}

// radix bit配列のprefix sumを用いてソート後の位置を計算する。
uint split(local ushort* radixBits, local ushort* falseTotal, const uint lid, ushort pred) {
    ushort trueBefore = scan(radixBits, lid);
    
    if (lid == LOCAL_SORT_SIZE - 1)
        *falseTotal = LOCAL_SORT_SIZE - (trueBefore + pred);
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (pred)
        return trueBefore + *falseTotal;
    else
        return lid - trueBefore;
}

// モートンコードのXYZを1ビットずつ取り出して、ローカルな範囲でradixソートを行う。
// 後の計算を効率的に行うために、ソート済みのradix digit配列も出力する。
// 1スレッドが1つのインデックスの入れ替えを行う。
kernel void blockwiseSort(const global uint3* mortonCodes, uint numPrimitives, uchar bitID,
                          global uint* indices, global uchar* radixDigits) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    local uchar radixDigitsInBlock[LOCAL_SORT_SIZE];
    local uint indicesInBlock[LOCAL_SORT_SIZE];
    local ushort radixBits[LOCAL_SORT_SIZE];
    local ushort falseTotal;
    
    // ローカルメモリにインデックスとモートンコードから抽出したradix digitを格納する。
    if (gid0 < numPrimitives) {
        uint idx = indices[gid0];
        indicesInBlock[lid0] = idx;
        uchar3 extracted = convert_uchar3((mortonCodes[idx] >> bitID) & 0x01);
        radixDigitsInBlock[lid0] = extracted.x + (extracted.y << 1) + (extracted.z << 2);
    }
    else {
        radixDigitsInBlock[lid0] = 0x07;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (uint axis = 0; axis < 3; ++axis) {
        // predにコピーしてsplit()との間にバリアを張ることでsplit()がinline化されることによる同期ミスを防ぐ。
        uchar curDigit = radixDigitsInBlock[lid0];
        uint curIdx = indicesInBlock[lid0];
        uchar pred = (curDigit >> axis) & 0x01;
        radixBits[lid0] = pred;
        barrier(CLK_LOCAL_MEM_FENCE);
        
        uint dstIdx = split(radixBits, &falseTotal, lid0, pred);
        indicesInBlock[dstIdx] = curIdx;
        radixDigitsInBlock[dstIdx] = curDigit;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // グローバルメモリに書き戻す。
    if (gid0 < numPrimitives) {
        indices[gid0] = indicesInBlock[lid0];
        radixDigits[gid0] = radixDigitsInBlock[lid0];
    }
}



// 各ローカルソートグループに関してradix bucket値のヒストグラムを計算する。
// 各グループ中における各bucket値までのオフセットも出力する。
// 1スレッドが1グループに対応する。
kernel void calcBlockwiseHistograms(const global uchar* radixDigits, uint numPrimitives, 
                                    uint numBlocks, global uint* histograms, global ushort* localOffsets) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numBlocks)
        return;
    
    ushort localHistogram[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint idxMax = min(LOCAL_SORT_SIZE * (gid0 + 1), numPrimitives);
    for (uint i = LOCAL_SORT_SIZE * gid0; i < idxMax; ++i)
        ++localHistogram[radixDigits[i]];
    
    ushort localOffset = 0;
    ushort lastValue = 0;
    for (uint i = 0; i < 8; ++i) {
        localOffset += lastValue;
        localOffsets[8 * gid0 + i] = localOffset;
        lastValue = localHistogram[i];
        histograms[numBlocks * i + gid0] = lastValue;
    }
}



// 各radix bucket値のグローバルオフセットと、グループ中のオフセットを用いてradix sortの1パスを終了する。
// 1スレッドが1つのインデックスの入れ替えを行う。
kernel void globalScatter(const global uchar* radixDigits, uint numPrimitives, const global uint* globalBucketOffsets, const global ushort* localOffsets,
                          const global uint* indicesSrc, global uint* indicesDst) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
    
    uint idxSrc = indicesSrc[gid0];
    uchar digit = radixDigits[gid0];
    uint gOffset = globalBucketOffsets[get_num_groups(0) * digit + get_group_id(0)];
    uint lOffset = localOffsets[8 * get_group_id(0) + digit];
    indicesDst[gOffset + (lid0 - lOffset)] = idxSrc;
}



// 2つのモートンコードの上位共通ビット数を返す。
// 同じモートンコードの場合はインデックスを付加して拡張した数値に関して処理する。
char _numCommonBits(const global uint3* mortonCodes, const global uint* indices, uint numPrimitives, uint bitsPerDim,
                    const uint3* mc_i, int _idx0, int _idx1) {
    if (_idx1 < 0 || _idx1 >= (int)numPrimitives)
        return -1;
    uint idx1 = indices[_idx1];
    uint3 diffPosFromMSB = clz(*mc_i ^ mortonCodes[idx1]) - (32 - bitsPerDim);
    char ret = min(diffPosFromMSB.z * 3 + 0, min(diffPosFromMSB.y * 3 + 1, diffPosFromMSB.x * 3 + 2));
    if ((uint)ret != bitsPerDim * 3)
        return ret;
    return ret + clz(_idx0 ^ _idx1);
}
#define numCommonBits(i, j) _numCommonBits(mortonCodes, indices, numPrimitives, bitsPerDim, &mc_i, i, j)

inline char signInt8(char val) {
    return val > 0 ? 1 : (val < 0 ? -1 : 0);
}

// LBVHの木構造、主にinternal nodeの情報を並列に計算する。
// leaf nodeの情報や、後のステップで必要となる情報の初期化も同時に行っておく。
// 1スレッドが１つのinternal nodeを処理する。
kernel void constructBinaryRadixTree(const global uint3* mortonCodes, uint bitsPerDim, const global uchar* AABBs, const global uint* indices, uint numPrimitives,
                                     global uchar* iNodes, global uchar* lNodes, global uint* parentIdxs, global uint* counters) {
    const uint gid0 = get_global_id(0);
    if (gid0 >= numPrimitives)
        return;
    
    // leaf nodeの情報を記録しておく。
    global LeafNode* lNode = (global LeafNode*)lNodes + gid0;
    uint objIdx = indices[gid0];
    lNode->bbox = *((const global AABB*)AABBs + objIdx);
    lNode->objIdx = objIdx;
    
    // internal nodeの範囲を出るスレッドは終了させる。後のステップで使用するカウンターの初期化を行っておく。
    if (gid0 >= numPrimitives - 1)
        return;
    counters[gid0] = 0;
    
    uint3 mc_i = mortonCodes[objIdx];
    
    char commonR = numCommonBits(gid0, (int)gid0 + 1);
    char commonL = numCommonBits(gid0, (int)gid0 - 1);
    char d = signInt8(commonR - commonL);
    
    // ノードの幅の上限を求める。
    char minNumComBits = d > 0 ? commonL : commonR;
    uint lMax = 2;
    while (numCommonBits(gid0, (int)gid0 + lMax * d) > minNumComBits)
        lMax <<= 1;
    
    // ノードのもうひとつの端を求める。
    uint l = 0;
    for (uint t = lMax >> 1; t >= 1; t >>= 1)
        if (numCommonBits(gid0, (int)gid0 + (l + t) * d) > minNumComBits)
            l += t;
    uint otherEnd = gid0 + l * d;
    
    // 分割するインデックスを求める。
    char numComBitsNode = numCommonBits(gid0, otherEnd);
    uint splitPos = 0;
    float scale = 0.5f;
    while (true) {
        uint t = (uint)ceil(l * scale);
        if (numCommonBits(gid0, (int)gid0 + (splitPos + t) * d) > numComBitsNode)
            splitPos += t;
        if (t <= 1)
            break;
        scale *= 0.5f;
    }
    splitPos = gid0 + splitPos * d + min(d, (char)0);
    
    // ノードの子のインデックスを記録する。
    global InternalNode* iNode = (global InternalNode*)iNodes + gid0;
    bool leftIsLeaf = min(gid0, otherEnd) == splitPos;
    iNode->isLeaf[0] = leftIsLeaf;
    iNode->c[0] = splitPos;
    bool rightIsLeaf = max(gid0, otherEnd) == splitPos + 1;
    iNode->isLeaf[1] = rightIsLeaf;
    iNode->c[1] = splitPos + 1;
    
    // それぞれの子ノードから親ノードを辿れるようにインデックスを記録しておく。
    parentIdxs[(leftIsLeaf ? 0 : numPrimitives) + splitPos] = gid0;
    parentIdxs[(rightIsLeaf ? 0 : numPrimitives) + splitPos + 1] = gid0;
}

//#define CALC_NODE_AABB_GROUP_SIZE 64
// 各ノードのAABBを計算する。leaf nodeからルートへの経路単位で並列に計算される。
kernel void calcNodeAABBs(volatile global uchar* _iNodes, global uint* counters, const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs) {
    const uint gid0 = get_global_id(0);
//    const uint lid0 = get_local_id(0);
    volatile global InternalNode* iNodes = (volatile global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    if (gid0 >= numPrimitives)
        return;
    
//    uint localCounters[CALC_NODE_AABB_GROUP_SIZE];
//    localCounters[lid0] = 0;
    
    const global LeafNode* lNode = lNodes + gid0;
    point3 min = lNode->bbox.min;
    point3 max = lNode->bbox.max;
    
    uint selfIdx = gid0;
    uint tgtIdx = parentIdxs[gid0];
    parentIdxs += numPrimitives;
    
    // InternalNodeを繰り返しルートに向けて登る。
    // 2回目のアクセスを担当するスレッドがAABBの和をとることによって、そのノードの子全てが計算済みであることを保証する。
    while (atomic_inc(counters + tgtIdx) == 1) {
        volatile global InternalNode* tgtINode = iNodes + tgtIdx;
        bool leftIsSelf = tgtINode->c[0] == selfIdx;
        uint otherIdx = tgtINode->c[leftIsSelf];
        const AABB bbox = tgtINode->isLeaf[leftIsSelf] ? (lNodes + otherIdx)->bbox : (iNodes + otherIdx)->bbox;
        tgtINode->bbox.min = min = fmin(min, bbox.min);
        tgtINode->bbox.max = max = fmax(max, bbox.max);
        
        if (tgtIdx == 0)
            return;
        
        selfIdx = tgtIdx;
        tgtIdx = parentIdxs[tgtIdx];
    }
}
