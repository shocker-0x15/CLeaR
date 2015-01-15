typedef float3 point3;

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

kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, global uint* numTotalLeaves,
                                 const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs,
                                 uint gamma);

//----------------------------------------------------------------

#ifndef LOCAL_SIZE
#define LOCAL_SIZE 32
#endif
kernel void treeletRestructuring(global uchar* _iNodes, global uint* counters, global uint* numTotalLeaves,
                                 const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs,
                                 uint gamma) {
    const uint gid0 = get_global_id(0);
    const uint lid0 = get_local_id(0);
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    if (gid0 >= numPrimitives)
        return;
    
    local uchar localMemPool[640];
    
    local uint numRoots;
    local uint treeletRoots[8];
    if (lid0 == 0)
        numRoots = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
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
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (uint i = 0; i < numRoots; ++i) {
        
    }
}
