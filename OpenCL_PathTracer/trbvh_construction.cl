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

kernel void bottomUp(global uchar* _iNodes, global uint* counters, global volatile uint* numTotalLeaves,
                     const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs,
                     uint gamma);

//----------------------------------------------------------------

kernel void bottomUp(global uchar* _iNodes, global uint* counters, global volatile uint* numTotalLeaves,
                     const global uchar* _lNodes, uint numPrimitives, const global uint* parentIdxs,
                     uint gamma) {
    const uint gid0 = get_global_id(0);
    global InternalNode* iNodes = (global InternalNode*)_iNodes;
    const global LeafNode* lNodes = (const global LeafNode*)_lNodes;
    if (gid0 >= numPrimitives)
        return;
    
    uint numLeaves = 1;
    uint selfIdx = gid0;
    uint pIdx = parentIdxs[gid0];
    parentIdxs += numPrimitives;
    
    while (atomic_inc(counters + pIdx) == 3) {
        const global InternalNode* iNode = iNodes + pIdx;
        bool leftIsSelf = iNode->c[0] == selfIdx;
        uint otherIdx = iNode->c[leftIsSelf];
        numLeaves += iNode->isLeaf[leftIsSelf] ? 1 : numTotalLeaves[otherIdx];
        numTotalLeaves[pIdx] = numLeaves;
        
        if (pIdx == 0)
            break;
        
        selfIdx = pIdx;
        pIdx = parentIdxs[pIdx];
    }
}