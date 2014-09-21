//
//  generic_kernels.cl
//  Copyright (c) 2014 Shin Watanabe. All rights reserved.
//

#define localScanSize 128
kernel void localScan(global uint* elements, uint numElements, global uint* blockSums) {
    const uint lid0 = get_local_id(0);
    const uint gid0 = get_global_id(0);
    
    local uint blockPrefixSum[localScanSize];
    
    // ローカルメモリに要素をコピーする。範囲外のものは0とする。
    if (gid0 < numElements)
        blockPrefixSum[lid0] = elements[gid0];
    else
        blockPrefixSum[lid0] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // The Up-Sweep (Reduce) Phase
    for (uint i = 2; i <= localScanSize; i <<= 1) {
        uint idx = (lid0 + 1) * i - 1;
        if (idx < localScanSize)
            blockPrefixSum[idx] += blockPrefixSum[idx - (i >> 1)];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // The Down-Sweep Phase
    if (lid0 == 0) {
        blockSums[(get_global_offset(0) / localScanSize) + get_group_id(0)] = blockPrefixSum[localScanSize - 1];
        blockPrefixSum[localScanSize - 1] = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    for (uint i = localScanSize; i >= 2; i >>= 1) {
        uint idx1 = (lid0 + 1) * i - 1;
        if (idx1 < localScanSize) {
            uint idx0 = idx1 - (i >> 1);
            ushort temp = blockPrefixSum[idx0];
            blockPrefixSum[idx0] = blockPrefixSum[idx1];
            blockPrefixSum[idx1] += temp;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // グローバルメモリに書き戻す。
    if (gid0 < numElements)
        elements[gid0] = blockPrefixSum[lid0];
}

kernel void addOffset(const global uint* blockOffsets, global uint* elements, uint numElements) {
    const uint gid0 = get_global_id(0);
    
    if (gid0 < numElements)
        elements[gid0] += blockOffsets[(get_global_offset(0) / localScanSize) + get_group_id(0)];
}
