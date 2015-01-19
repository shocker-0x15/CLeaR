//
//  TRBVHBuilder.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/11/19.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "TRBVHBuilder.h"
#include "StopWatch.hpp"

namespace TRBVH {
    Builder::Builder(cl::Context &context, cl::Device &device, uint32_t numFaces) :
    LBVH::Builder(context, device, numFaces),
    localSizeBottomUp(32) {
        std::string rawStrBuildAccel = CLUtil::stringFromFile("OpenCL_src/trbvh_construction.cl");
        cl::Program::Sources srcBuildAccel{1, std::make_pair(rawStrBuildAccel.c_str(), rawStrBuildAccel.length())};
        
        cl::Program programTRBVH{context, srcBuildAccel};
        std::string buildLog;
        char extraArgs[256];
        sprintf(extraArgs, "-I\"OpenCL_src\"");
        programTRBVH.build(extraArgs);
        programTRBVH.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("TRBVH build program build log: \n");
        printf("%s\n", buildLog.c_str());
        
        m_kernelCalcAABBs_SAHCosts = cl::Kernel(programTRBVH, "calcNodeAABBs_SAHCosts");
        m_kernelTreeletRestructuring = cl::Kernel(programTRBVH, "treeletRestructuring");
        
        m_bufNumTotalLeaves = cl::Buffer{context, CL_MEM_READ_WRITE, (numFaces - 1) * sizeof(uint32_t), nullptr, nullptr};
    }
    
    void Builder::perform(cl::CommandQueue &queue,
                          const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                          cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling) {
        using LBVH::AABB;
        using LBVH::InternalNode;
        using LBVH::LeafNode;
        
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
        calcAABBs(queue, events, buf_vertices, buf_faces, numFaces);
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
        unifyAABBs(queue, events, buf_unifiedAABBs, numFaces);
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
        calcMortonCodes(queue, events, numFaces, entireAABB, numBitsPerDim);
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
        radixSort(queue, events, numFaces, numBitsPerDim);
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
        
        // BVHの木構造を計算する。
        if (profiling)
            stopwatchHiRes.start();
        constructBinaryRadixTree(queue, events, bufInternalNodes, bufLeafNodes, numFaces, numBitsPerDim);
        if (profiling) {
            queue.finish();
            events.back().wait();
            CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            printf("constructing BVH tree done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
        }
        
        // 各ノードのAABBを計算する。
        if (profiling)
            stopwatchHiRes.start();
        calcNodeAABBs(queue, events, bufInternalNodes, bufLeafNodes, numFaces);
        if (profiling) {
            queue.finish();
            events.back().wait();
            CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            printf("calculating node-AABBs done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
        }
//        queue.finish();
//        std::vector<InternalNode> internalNodes;
//        std::vector<LeafNode> leafNodes;
//        internalNodes.resize(numFaces - 1);
//        leafNodes.resize(numFaces);
//        queue.enqueueReadBuffer(bufInternalNodes, CL_TRUE, 0, internalNodes.size() * sizeof(InternalNode), internalNodes.data());
//        queue.enqueueReadBuffer(bufLeafNodes, CL_TRUE, 0, leafNodes.size() * sizeof(LeafNode), leafNodes.data());
//        printf("Internal Nodes\n");
//        for (uint32_t i = 0; i < internalNodes.size(); ++i) {
//            InternalNode &iNode = internalNodes[i];
//            cl_float3 edge{iNode.max.s0 - iNode.min.s0, iNode.max.s1 - iNode.min.s1, iNode.max.s2 - iNode.min.s2};
//            float surfaceArea = 2 * (edge.s0 * edge.s1 + edge.s1 * edge.s2 + edge.s2 * edge.s0);
//            printf("%5u : %5u%c, %5u%c, %f\n", i,
//                   iNode.c[0], iNode.isLeaf[0] ? 'L' : ' ',
//                   iNode.c[1], iNode.isLeaf[1] ? 'L' : ' ',
//                   surfaceArea);
//        }
//        printf("--------------------------------\n");
//        printf("Leaf Nodes\n");
//        for (uint32_t i = 0; i < leafNodes.size(); ++i) {
//            LeafNode &lNode = leafNodes[i];
//            cl_float3 edge{lNode.max.s0 - lNode.min.s0, lNode.max.s1 - lNode.min.s1, lNode.max.s2 - lNode.min.s2};
//            float surfaceArea = 2 * (edge.s0 * edge.s1 + edge.s1 * edge.s2 + edge.s2 * edge.s0);
//            printf("%5uL: %5u , %f\n", i, lNode.objIdx, surfaceArea);
//        }
        
        events.emplace_back();
        const uint32_t workSizeRestructuring = CLUtil::largerMultiple(numFaces, localSizeBottomUp);
        cl::enqueueNDRangeKernel(queue, m_kernelTreeletRestructuring, cl::NullRange, cl::NDRange(workSizeRestructuring), cl::NDRange(localSizeBottomUp), nullptr, &events.back(),
                                 bufInternalNodes, m_bufCounters, m_bufNumTotalLeaves, bufLeafNodes, numFaces, m_bufParentIdxs, 7);
        queue.enqueueBarrierWithWaitList();
        
        queue.finish();
//        std::vector<uint32_t> numTotalLeaves;
//        numTotalLeaves.resize(numFaces - 1);
//        queue.enqueueReadBuffer(m_bufNumTotalLeaves, CL_TRUE, 0, numTotalLeaves.size() * sizeof(uint32_t), numTotalLeaves.data());
//        if (numTotalLeaves[0] == 35018)
//            for (uint32_t i = 0; i < numTotalLeaves.size(); ++i)
//                printf("%u: %u\n", i, numTotalLeaves[i]);
        printf("");
    };
}
