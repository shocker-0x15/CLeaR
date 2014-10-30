//
//  LBVHBuilder.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/10/25.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "LBVHBuilder.h"
#include "StopWatch.hpp"

// 48bytes
struct AABB {
    cl_float3 min;
    cl_float3 max;
    cl_float3 center;
};

LBVHBuilder::LBVHBuilder(cl::Context &context, cl::Device &device, uint32_t numFaces) :
CLUtil::Technique(context, device), m_techGlobalScan{context, device},
localSizeCalcAABBs(128), localSizeUnifyAABBs(128), localSizeCalcMortonCodes(128),
localSizeBlockwiseSort(64), localSizeCalcBlockHistograms(128),
localSizeConstructBinaryRadixTree(64), localSizeCalcNodeAABBs(64) {
    std::string rawStrBuildAccel = CLUtil::stringFromFile("lbvh_construction.cl");
    cl::Program::Sources srcBuildAccel{1, std::make_pair(rawStrBuildAccel.c_str(), rawStrBuildAccel.length())};
    
    cl::Program programBuildAccel{context, srcBuildAccel};
    std::string buildLog;
    char extraArgs[256];
    sprintf(extraArgs, "-DLOCAL_SORT_SIZE=%u", localSizeBlockwiseSort);
    programBuildAccel.build(extraArgs);
    programBuildAccel.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
    printf("LBVH build program build log: \n");
    printf("%s\n", buildLog.c_str());
    
    m_kernelCalcAABBs = cl::Kernel(programBuildAccel, "calcAABBs");
    m_kernelUnifyAABBs = cl::Kernel(programBuildAccel, "unifyAABBs");
    m_kernelCalcMortonCodes = cl::Kernel(programBuildAccel, "calcMortonCodes");
    m_kernelBlockwiseSort = cl::Kernel(programBuildAccel, "blockwiseSort");
    m_kernelCalcBlockHistograms = cl::Kernel(programBuildAccel, "calcBlockwiseHistograms");
    m_kernelGlobalScatter = cl::Kernel(programBuildAccel, "globalScatter");
    m_kernelConstructBinaryRadixTree = cl::Kernel(programBuildAccel, "constructBinaryRadixTree");
    m_kernelCalcNodeAABBs = cl::Kernel(programBuildAccel, "calcNodeAABBs");
    
    m_currentCapacity = numFaces;
    setupWorkingBuffers();
    m_createdBuffers = true;
}

void LBVHBuilder::setupWorkingBuffers() {
    enum {
        BuildPass_calcAABBs,
        BuildPass_unifyAABBs,
        BuildPass_calcMortonCodes,
        BuildPass_blockwiseSort,
        BuildPass_calcBlockHistograms,
        BuildPass_globalScan,
        BuildPass_globalScatter,
        BuildPass_constructBinaryRadixTree,
        BuildPass_calcNodeAABBs,
        NumBuildPasses
    };
    
    struct Occupancy {
        uint64_t memStart;
        uint64_t memEnd;
        bool overlaps(uint64_t start, uint64_t end) {
            return !(end < memStart || start > memEnd);
        }
    };
    std::vector<Occupancy> occupancies[NumBuildPasses];
    
    // SubBufferの確保できる領域を検索して、確保領域に関する情報を記録する。
    auto reserveAlloc = [&occupancies](uint64_t size, uint32_t align, uint32_t lifeStart, uint32_t lifeEnd) {
        uint64_t offset = 0;
        while (true) {
            Occupancy overlapRange{UINT64_MAX, 0};
            for (uint32_t pass = lifeStart; pass <= lifeEnd; ++pass) {
                auto &occupanciesForPass = occupancies[pass];
                for (uint32_t i = 0; i < occupanciesForPass.size(); ++i) {
                    if (occupanciesForPass[i].overlaps(offset, offset + size - 1)) {
                        overlapRange.memStart = std::min(overlapRange.memStart, occupanciesForPass[i].memStart);
                        overlapRange.memEnd = std::max(overlapRange.memEnd, occupanciesForPass[i].memEnd);
                    }
                }
            }
            
            if (overlapRange.memStart == UINT64_MAX)
                break;
            
            offset = (overlapRange.memEnd + 1 + (align - 1)) & ~(align - 1);
        }
        
        Occupancy region{offset, offset + size - 1};
        for (uint32_t pass = lifeStart; pass <= lifeEnd; ++pass) {
            occupancies[pass].push_back(region);
        }
        
        return region;
    };
    
    const uint32_t numElementsHistograms = ((m_currentCapacity + (localSizeBlockwiseSort - 1)) / localSizeBlockwiseSort) * (1 << 3);
    const uint32_t sizeUnifyAABBWorkingBuffers = 2 * ((m_currentCapacity + (localSizeUnifyAABBs - 1)) / localSizeUnifyAABBs) * sizeof(AABB);
    
    // AABBs, unifyAABBsWorkingBuffers, MortonCodes,
    // indices, indices_shadow, radixDigits, histograms, offsets, globalScanWorkingBuffers
    // parentIdxs, counters
    std::vector<Occupancy> regions;
    regions.push_back(reserveAlloc(m_currentCapacity * sizeof(AABB), 16, BuildPass_calcAABBs, BuildPass_constructBinaryRadixTree));
    regions.push_back(reserveAlloc(sizeUnifyAABBWorkingBuffers, 16, BuildPass_unifyAABBs, BuildPass_unifyAABBs));
    regions.push_back(reserveAlloc(m_currentCapacity * sizeof(cl_uint3), 16, BuildPass_calcMortonCodes, BuildPass_constructBinaryRadixTree));
    regions.push_back(reserveAlloc(m_currentCapacity * sizeof(cl_uint), 4, BuildPass_calcMortonCodes, BuildPass_constructBinaryRadixTree));
    regions.push_back(reserveAlloc(m_currentCapacity * sizeof(cl_uint), 4, BuildPass_blockwiseSort, BuildPass_constructBinaryRadixTree));
    regions.push_back(reserveAlloc(m_currentCapacity * sizeof(cl_uchar), 1, BuildPass_blockwiseSort, BuildPass_globalScatter));
    regions.push_back(reserveAlloc(numElementsHistograms * sizeof(cl_uint), 4, BuildPass_calcBlockHistograms, BuildPass_globalScatter));
    regions.push_back(reserveAlloc(numElementsHistograms * sizeof(cl_uint), 4, BuildPass_calcBlockHistograms, BuildPass_globalScatter));
    uint64_t globalScanBufferSize;
    uint32_t globalScanBufferAlign;
    m_techGlobalScan.calcWorkingBuffersRequirement(numElementsHistograms, &globalScanBufferSize, &globalScanBufferAlign);
    regions.push_back(reserveAlloc(globalScanBufferSize, globalScanBufferAlign, BuildPass_globalScan, BuildPass_globalScan));
    regions.push_back(reserveAlloc((2 * m_currentCapacity - 1) * sizeof(cl_uint), 4, BuildPass_constructBinaryRadixTree, BuildPass_calcNodeAABBs));
    regions.push_back(reserveAlloc((m_currentCapacity - 1) * sizeof(cl_uint), 4, BuildPass_constructBinaryRadixTree, BuildPass_calcNodeAABBs));
    
    uint64_t requiredSize = 0;
    for (uint32_t i = 0; i < regions.size(); ++i) {
        requiredSize = std::max(requiredSize, regions[i].memEnd);
    }
    requiredSize += 1;
    
    m_bufferGenericPool = cl::Buffer(m_context, CL_MEM_READ_WRITE, requiredSize, nullptr, nullptr);
    
    m_bufAABBs = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                     regions[0].memStart, 16, regions[0].memEnd - regions[0].memStart + 1);
    m_bufs_unifyAABBs[0] = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                               regions[1].memStart, 16, (regions[1].memEnd - regions[1].memStart + 1) / 2);
    m_bufs_unifyAABBs[1] = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                               (regions[1].memEnd + regions[1].memStart + 1) / 2, 16, (regions[1].memEnd - regions[1].memStart + 1) / 2);
    
    m_bufMortonCodes = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                           regions[2].memStart, 16, regions[2].memEnd - regions[2].memStart + 1);
    m_bufIndices = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                       regions[3].memStart, 4, regions[3].memEnd - regions[3].memStart + 1);
    m_bufIndicesShadow = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                             regions[4].memStart, 4, regions[4].memEnd - regions[4].memStart + 1);
    m_bufRadixDigits = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                           regions[5].memStart, 1, regions[5].memEnd - regions[5].memStart + 1);
    m_bufHistograms = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                          regions[6].memStart, 4, regions[6].memEnd - regions[6].memStart + 1);
    m_bufLocalOffsets = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                            regions[7].memStart, 4, regions[7].memEnd - regions[7].memStart + 1);
    uint64_t temp;
    m_techGlobalScan.createWorkingBuffers(numElementsHistograms, m_bufferGenericPool, regions[8].memStart, &m_bufs_globalScan, &temp);
    
    m_bufParentIdxs = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                          regions[9].memStart, 4, regions[9].memEnd - regions[9].memStart + 1);
    m_bufCounters = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                        regions[10].memStart, 4, regions[10].memEnd - regions[10].memStart + 1);
}

void LBVHBuilder::perform(cl::CommandQueue &queue,
                          const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                          cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling) {
    StopWatchHiRes stopwatchHiRes;
    uint32_t evStart;
    cl_ulong tpCmdStart, tpCmdEnd, tpCmdSubmit;
    
    if (numFaces > m_currentCapacity) {
        m_currentCapacity = numFaces;
        setupWorkingBuffers();
    }
    
    // 各三角形のAABBを並列に求める。
    if (profiling)
        stopwatchHiRes.start();
    {
        events.emplace_back();
        const uint32_t workSize = ((numFaces + (localSizeCalcAABBs - 1)) / localSizeCalcAABBs) * localSizeCalcAABBs;
        cl::enqueueNDRangeKernel(queue, m_kernelCalcAABBs, cl::NullRange, cl::NDRange(workSize), cl::NDRange(localSizeCalcAABBs), nullptr, &events.back(),
                                 buf_vertices, buf_faces, numFaces, m_bufAABBs);
        queue.enqueueBarrierWithWaitList();
    }
    if (profiling) {
        queue.finish();
        events.back().wait();
        CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
        printf("calculating each AABB done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
    }
    
    // 全体を囲むAABBを求める。
    evStart = (uint32_t)events.size();
    if (profiling)
        stopwatchHiRes.start();
    cl::Buffer buf_unifiedAABBs;
    {
        cl::Buffer buf_srcAABBs = m_bufAABBs;
        uint32_t numAABBs = numFaces;
        uint32_t numMerged = (numAABBs + (localSizeUnifyAABBs - 1)) / localSizeUnifyAABBs;
        
        uint32_t swapIdx = 0;
        while (true) {
            events.emplace_back();
            buf_unifiedAABBs = m_bufs_unifyAABBs[swapIdx++ % 2];
            
            uint32_t workSize = numMerged * localSizeUnifyAABBs;
            cl::enqueueNDRangeKernel(queue, m_kernelUnifyAABBs, cl::NullRange, cl::NDRange(workSize), cl::NDRange(localSizeUnifyAABBs), nullptr, &events.back(),
                                     buf_srcAABBs, numAABBs, buf_unifiedAABBs);
            queue.enqueueBarrierWithWaitList();
            
            if (numMerged == 1)
                break;
            
            buf_srcAABBs = buf_unifiedAABBs;
            numAABBs = numMerged;
            numMerged = (numAABBs + (localSizeUnifyAABBs - 1)) / localSizeUnifyAABBs;
        }
    }
    if (profiling) {
        queue.finish();
        cl_ulong sumTimeUnion = 0, sumTimeUnionFromSubmit = 0;
        for (uint32_t i = evStart; i < events.size(); ++i) {
            events[i].wait();
            CLUtil::getProfilingInfo(events[i], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            sumTimeUnion += tpCmdEnd - tpCmdStart;
            sumTimeUnionFromSubmit += tpCmdEnd - tpCmdSubmit;
        }
        printf("unifying AABBs done! ... time: %fusec (%fusec)\n", sumTimeUnion * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
    }
    AABB entireAABB;
    queue.enqueueReadBuffer(buf_unifiedAABBs, CL_TRUE, 0, sizeof(AABB), &entireAABB);
    
    // モートンコードを求める。
    if (profiling)
        stopwatchHiRes.start();
    {
        events.emplace_back();
        cl_float3 sizeEntireAABB = {
            entireAABB.max.s0 - entireAABB.min.s0,
            entireAABB.max.s1 - entireAABB.min.s1,
            entireAABB.max.s2 - entireAABB.min.s2
        };
        
        const uint32_t workSize = ((numFaces + (localSizeCalcMortonCodes - 1)) / localSizeCalcMortonCodes) * localSizeCalcMortonCodes;
        cl::enqueueNDRangeKernel(queue, m_kernelCalcMortonCodes, cl::NullRange, cl::NDRange(workSize), cl::NDRange(localSizeCalcMortonCodes), nullptr, &events.back(),
                                 m_bufAABBs, numFaces, entireAABB.min, sizeEntireAABB, numBitsPerDim, m_bufMortonCodes, m_bufIndices);
        queue.enqueueBarrierWithWaitList();
    }
    if (profiling) {
        queue.finish();
        events.back().wait();
        CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
        printf("calculating each Morton code done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
    }
    
    // モートンコードにしたがってradixソート。
    evStart = (uint32_t)events.size();
    if (profiling)
        stopwatchHiRes.start();
    {
        const uint32_t numPrimGroups = ((m_currentCapacity + (localSizeBlockwiseSort - 1)) / localSizeBlockwiseSort);
        const uint32_t workSizeBlockwiseSort = numPrimGroups * localSizeBlockwiseSort;
        const uint32_t workSizeHistograms = ((numPrimGroups + (localSizeCalcBlockHistograms - 1)) / localSizeCalcBlockHistograms) * localSizeCalcBlockHistograms;
        const uint32_t numElementsHistograms = numPrimGroups * (1 << 3);
        for (uint32_t i = 0; i < numBitsPerDim; ++i) {
            events.emplace_back();
            cl::enqueueNDRangeKernel(queue, m_kernelBlockwiseSort, cl::NullRange, cl::NDRange(workSizeBlockwiseSort), cl::NDRange(localSizeBlockwiseSort),
                                     nullptr, &events.back(),
                                     m_bufMortonCodes, numFaces, i, m_bufIndices, m_bufRadixDigits);
            queue.enqueueBarrierWithWaitList();
            
            events.emplace_back();
            cl::enqueueNDRangeKernel(queue, m_kernelCalcBlockHistograms, cl::NullRange, cl::NDRange(workSizeHistograms), cl::NDRange(localSizeCalcBlockHistograms),
                                     nullptr, &events.back(),
                                     m_bufRadixDigits, numFaces, numPrimGroups, m_bufHistograms, m_bufLocalOffsets);
            queue.enqueueBarrierWithWaitList();
            
            m_techGlobalScan.perform(queue, m_bufHistograms, numElementsHistograms, m_bufs_globalScan, events);
            
            events.emplace_back();
            cl::enqueueNDRangeKernel(queue, m_kernelGlobalScatter, cl::NullRange, cl::NDRange(workSizeBlockwiseSort), cl::NDRange(localSizeBlockwiseSort),
                                     nullptr, &events.back(),
                                     m_bufRadixDigits, numFaces, m_bufHistograms, m_bufLocalOffsets, m_bufIndices, m_bufIndicesShadow);
            queue.enqueueBarrierWithWaitList();
            
            cl::Buffer tempIndices = m_bufIndices;
            m_bufIndices = m_bufIndicesShadow;
            m_bufIndicesShadow = tempIndices;
        }
    }
    if (profiling) {
        queue.finish();
        cl_ulong sumTimeRadixSort = 0, sumTimeRadixSortFromSubmit = 0;
        for (uint32_t i = evStart; i < events.size(); ++i) {
            events[i].wait();
            CLUtil::getProfilingInfo(events[i], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            sumTimeRadixSort += tpCmdEnd - tpCmdStart;
            sumTimeRadixSortFromSubmit += tpCmdEnd - tpCmdSubmit;
        }
        printf("radix sorting done! ... time: %fusec (%fusec)\n", sumTimeRadixSort * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
    }
//    std::vector<cl_uint> indices;
//    indices.resize(m_currentCapacity);
//    std::vector<cl_uint3> MortonCodes;
//    MortonCodes.resize(m_currentCapacity);
//    queue.enqueueReadBuffer(m_bufIndices, CL_TRUE, 0, indices.size() * sizeof(cl_uint), indices.data());
//    queue.enqueueReadBuffer(m_bufMortonCodes, CL_TRUE, 0, MortonCodes.size() * sizeof(cl_uint3), MortonCodes.data());
//    for (uint32_t i = 0; i < indices.size(); ++i) {
//        cl_uint idx = indices[i];
//        cl_uint3 mc = MortonCodes[idx];
//        printf("%5u:", idx);
//        for (int32_t bit = numBitsPerDim - 1; bit >= 0; --bit) {
//            printf(" %u%u%u", (mc.s2 >> bit) & 0x01, (mc.s1 >> bit) & 0x01, (mc.s0 >> bit) & 0x01);
//        }
//        printf("\n");
//    }
    
    // BVHの木構造を計算する。
    if (profiling)
        stopwatchHiRes.start();
    {
        events.emplace_back();
        const uint32_t workSize = ((numFaces + (localSizeConstructBinaryRadixTree - 1)) / localSizeConstructBinaryRadixTree) * localSizeConstructBinaryRadixTree;
        cl::enqueueNDRangeKernel(queue, m_kernelConstructBinaryRadixTree, cl::NullRange, cl::NDRange(workSize), cl::NDRange(localSizeConstructBinaryRadixTree), nullptr, &events.back(),
                                 m_bufMortonCodes, numBitsPerDim, m_bufAABBs, m_bufIndices, numFaces,
                                 bufInternalNodes, bufLeafNodes, m_bufParentIdxs, m_bufCounters);
        queue.enqueueBarrierWithWaitList();
    }
    if (profiling) {
        queue.finish();
        events.back().wait();
        CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
        printf("constructing BVH tree done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
    }
    
    // 各ノードのAABBを計算する。
    if (profiling)
        stopwatchHiRes.start();
    {
        events.emplace_back();
        const uint32_t workSize = ((numFaces + (localSizeCalcNodeAABBs - 1)) / localSizeCalcNodeAABBs) * localSizeCalcNodeAABBs;
        cl::enqueueNDRangeKernel(queue, m_kernelCalcNodeAABBs, cl::NullRange, cl::NDRange(workSize), cl::NDRange(localSizeCalcNodeAABBs), nullptr, &events.back(),
                                 bufInternalNodes, m_bufCounters, bufLeafNodes, numFaces, m_bufParentIdxs);
        queue.enqueueBarrierWithWaitList();
    }
    if (profiling) {
        queue.finish();
        events.back().wait();
        CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
        printf("calculating node-AABBs done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
    }
//    std::vector<LeafNode> leafNodes;
//    leafNodes.resize(numFaces);
//    std::vector<InternalNode> internalNodes;
//    internalNodes.resize(numFaces - 1);
//    queue.enqueueReadBuffer(bufInternalNodes, CL_TRUE, 0, internalNodes.size() * sizeof(InternalNode), internalNodes.data());
//    queue.enqueueReadBuffer(bufLeafNodes, CL_TRUE, 0, leafNodes.size() * sizeof(LeafNode), leafNodes.data());
//    printf("--------------------------------\n");
//    printf("Internal Nodes\n");
//    for (uint32_t i = 0; i < internalNodes.size(); ++i) {
//        InternalNode &iNode = internalNodes[i];
//        printf("%5u: min(%11.3e, %11.3e, %11.3e) | max(%11.3e, %11.3e, %11.3e) %5u_%u %5u_%u\n", i,
//               iNode.min.s0, iNode.min.s1, iNode.min.s2,
//               iNode.max.s0, iNode.max.s1, iNode.max.s2,
//               iNode.c[0], iNode.isChild[0], iNode.c[1], iNode.isChild[1]);
//    }
//    printf("--------------------------------\n");
//    printf("Leaf Nodes\n");
//    for (uint32_t i = 0; i <leafNodes.size(); ++i) {
//        LeafNode &lNode = leafNodes[i];
//        printf("%5u: min(%11.3e, %11.3e, %11.3e) | max(%11.3e, %11.3e, %11.3e) %5u\n", i,
//               lNode.min.s0, lNode.min.s1, lNode.min.s2,
//               lNode.max.s0, lNode.max.s1, lNode.max.s2,
//               lNode.objIdx);
//    }
//    printf("--------------------------------\n");
}
