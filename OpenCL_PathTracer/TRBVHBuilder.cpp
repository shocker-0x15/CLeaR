//
//  TRBVHBuilder.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/11/19.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "TRBVHBuilder.h"
#include "StopWatch.hpp"

#include <iostream>
#include <fstream>

namespace TRBVH {
    Builder::Builder(cl::Context &context, cl::Device &device) :
    LBVH::Builder(context, device),
    localSizeRestructuring(32) {
        std::string rawStrBuildAccel = CLUtil::stringFromFile("OpenCL_src/trbvh_construction.cl");
        cl::Program::Sources srcBuildAccel{1, std::make_pair(rawStrBuildAccel.c_str(), rawStrBuildAccel.length())};
        
        cl::Program programTRBVH{context, srcBuildAccel};
        std::string buildLog;
        char extraArgs[256];
        sprintf(extraArgs, "-I \"OpenCL_src\"");
        programTRBVH.build(extraArgs);
        programTRBVH.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("TRBVH build program build log: \n");
        printf("%s\n", buildLog.c_str());
        
        m_kernelCalcAABBs_SAHCosts = cl::Kernel(programTRBVH, "calcNodeAABBs_SAHCosts");
        m_kernelTreeletRestructuring = cl::Kernel(programTRBVH, "treeletRestructuring");
        
        m_numBuildPass = (uint32_t)BuildPass::Num;
    }
    
    void Builder::setupWorkingBuffers() {
        const uint32_t numElementsHistograms = ((m_currentCapacity + (localSizeBlockwiseSort - 1)) / localSizeBlockwiseSort) * (1 << 3);
        const uint32_t sizeUnifyAABBWorkingBuffers = 2 * ((m_currentCapacity + (localSizeUnifyAABBs - 1)) / localSizeUnifyAABBs) * sizeof(AABB);
        
        enum BufferID {
            Buf_AABBs = 0,
            Buf_unifyAABBsWBs,
            Buf_MortonCodes,
            Buf_indices,
            Buf_indices_shadow,
            Buf_radixDigits,
            Buf_histograms,
            Buf_offsets,
            Buf_globalScanWBs,
            Buf_parentIdxs,
            Buf_counters,
            Buf_numTotalLeaves,
            Buf_SAHCosts, 
            NumBuffers
        };
        
        // SubBufferの確保できる領域を検索して、確保領域に関する情報を記録する。
        Occupancy regions[NumBuffers];
        regions[Buf_AABBs] = reserveAlloc(m_currentCapacity * sizeof(AABB), 16, (uint32_t)BuildPass::calcAABBs, (uint32_t)BuildPass::constructBinaryRadixTree);
        regions[Buf_unifyAABBsWBs] = reserveAlloc(sizeUnifyAABBWorkingBuffers, 16, (uint32_t)BuildPass::unifyAABBs, (uint32_t)BuildPass::unifyAABBs);
        regions[Buf_MortonCodes] = reserveAlloc(m_currentCapacity * sizeof(cl_uint3), 16, (uint32_t)BuildPass::calcMortonCodes, (uint32_t)BuildPass::constructBinaryRadixTree);
        regions[Buf_indices] = reserveAlloc(m_currentCapacity * sizeof(cl_uint), 4, (uint32_t)BuildPass::calcMortonCodes, (uint32_t)BuildPass::constructBinaryRadixTree);
        regions[Buf_indices_shadow] = reserveAlloc(m_currentCapacity * sizeof(cl_uint), 4, (uint32_t)BuildPass::blockwiseSort, (uint32_t)BuildPass::constructBinaryRadixTree);
        regions[Buf_radixDigits] = reserveAlloc(m_currentCapacity * sizeof(cl_uchar), 1, (uint32_t)BuildPass::blockwiseSort, (uint32_t)BuildPass::globalScatter);
        regions[Buf_histograms] = reserveAlloc(numElementsHistograms * sizeof(cl_uint), 4, (uint32_t)BuildPass::calcBlockHistograms, (uint32_t)BuildPass::globalScatter);
        regions[Buf_offsets] = reserveAlloc(numElementsHistograms * sizeof(cl_uint), 4, (uint32_t)BuildPass::calcBlockHistograms, (uint32_t)BuildPass::globalScatter);
        uint64_t globalScanBufferSize;
        uint32_t globalScanBufferAlign;
        m_techGlobalScan.calcWorkingBuffersRequirement(numElementsHistograms, &globalScanBufferSize, &globalScanBufferAlign);
        regions[Buf_globalScanWBs] = reserveAlloc(globalScanBufferSize, globalScanBufferAlign, (uint32_t)BuildPass::globalScan, (uint32_t)BuildPass::globalScan);
        regions[Buf_parentIdxs] = reserveAlloc((2 * m_currentCapacity - 1) * sizeof(cl_uint), 4, (uint32_t)BuildPass::constructBinaryRadixTree, (uint32_t)BuildPass::calcNodeAABBs);
        regions[Buf_counters] = reserveAlloc((m_currentCapacity - 1) * sizeof(cl_uint), 4, (uint32_t)BuildPass::constructBinaryRadixTree, (uint32_t)BuildPass::calcNodeAABBs);
        regions[Buf_numTotalLeaves] = reserveAlloc((m_currentCapacity - 1) * sizeof(cl_uint), 4, (uint32_t)BuildPass::calcNodeAABBs, (uint32_t)BuildPass::TreeletRestructuring);
        regions[Buf_SAHCosts] = reserveAlloc((2 * m_currentCapacity - 1) * sizeof(cl_float), 4, (uint32_t)BuildPass::calcNodeAABBs, (uint32_t)BuildPass::TreeletRestructuring);
        
        uint64_t requiredSize = 0;
        for (uint32_t i = 0; i < NumBuffers; ++i) {
            requiredSize = std::max(requiredSize, regions[i].memEnd);
        }
        requiredSize += 1;
        
        m_bufferGenericPool = cl::Buffer(m_context, CL_MEM_READ_WRITE, requiredSize, nullptr, nullptr);
        auto createSubBuffer = [this, &regions](BufferID buf, uint32_t align) {
            return cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION, regions[buf].memStart, align, regions[buf].size());
        };
        
        m_bufAABBs = createSubBuffer(Buf_AABBs, 16);
        m_bufs_unifyAABBs[0] = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                                   regions[Buf_unifyAABBsWBs].memStart, 16, regions[Buf_unifyAABBsWBs].size() / 2);
        m_bufs_unifyAABBs[1] = cl::createSubBuffer(m_bufferGenericPool, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                                                   regions[Buf_unifyAABBsWBs].memStart + regions[Buf_unifyAABBsWBs].size() / 2, 16,
                                                   regions[Buf_unifyAABBsWBs].size() / 2);
        
        m_bufMortonCodes = createSubBuffer(Buf_MortonCodes, 16);
        m_bufIndices = createSubBuffer(Buf_indices, 4);
        m_bufIndicesShadow = createSubBuffer(Buf_indices_shadow, 4);
        m_bufRadixDigits = createSubBuffer(Buf_radixDigits, 1);
        m_bufHistograms = createSubBuffer(Buf_histograms, 4);
        m_bufLocalOffsets = createSubBuffer(Buf_offsets, 4);
        uint64_t temp;
        m_techGlobalScan.createWorkingBuffers(numElementsHistograms, m_bufferGenericPool, regions[Buf_globalScanWBs].memStart, &m_bufs_globalScan, &temp);
        
        m_bufParentIdxs = createSubBuffer(Buf_parentIdxs, 4);
        m_bufCounters = createSubBuffer(Buf_counters, 4);
        m_bufNumTotalLeaves = createSubBuffer(Buf_numTotalLeaves, 4);
        m_bufSAHCosts = createSubBuffer(Buf_SAHCosts, 4);
    }
    
    
    bool Builder::validate(cl::CommandQueue &queue, uint32_t numFaces,
                           cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, bool quiet, bool dump) const {
        queue.finish();
        
        std::vector<InternalNode> internalNodes;
        std::vector<LeafNode> leafNodes;
        std::vector<uint32_t> numTotalLeaves;
        std::vector<float> SAHCosts;
        std::vector<uint32_t> parentIdxs;
        std::vector<uint32_t> counters;
        std::vector<uint8_t> refCounts;
        
        internalNodes.resize(numFaces - 1);
        leafNodes.resize(numFaces);
        numTotalLeaves.resize(numFaces - 1);
        SAHCosts.resize(2 * numFaces - 1);
        parentIdxs.resize(2 * numFaces - 1);
        counters.resize(numFaces - 1);
        refCounts.resize(2 * numFaces - 1, 0);
        
        queue.enqueueReadBuffer(bufInternalNodes, CL_TRUE, 0, internalNodes.size() * sizeof(InternalNode), internalNodes.data());
        queue.enqueueReadBuffer(bufLeafNodes, CL_TRUE, 0, leafNodes.size() * sizeof(LeafNode), leafNodes.data());
        queue.enqueueReadBuffer(m_bufNumTotalLeaves, CL_TRUE, 0, numTotalLeaves.size() * sizeof(uint32_t), numTotalLeaves.data());
        queue.enqueueReadBuffer(m_bufSAHCosts, CL_TRUE, 0, SAHCosts.size() * sizeof(float), SAHCosts.data());
        queue.enqueueReadBuffer(m_bufParentIdxs, CL_TRUE, 0, parentIdxs.size() * sizeof(uint32_t), parentIdxs.data());
        queue.enqueueReadBuffer(m_bufCounters, CL_TRUE, 0, counters.size() * sizeof(uint32_t), counters.data());
        
        auto calcSurfaceArea = [](const AABB &bbox) {
            cl_float3 edge{bbox.max.s0 - bbox.min.s0, bbox.max.s1 - bbox.min.s1, bbox.max.s2 - bbox.min.s2};
            return 2 * (edge.s0 * edge.s1 + edge.s1 * edge.s2 + edge.s2 * edge.s0);
        };
        
        if (quiet) {
            InternalNode &iNode = internalNodes[0];
            bool lIsL = iNode.isLeaf[0];
            bool rIsL = iNode.isLeaf[1];
            uint32_t c0 = iNode.c[0];
            uint32_t c1 = iNode.c[1];
            float surfaceArea = calcSurfaceArea(iNode.bbox);
            
            uint32_t cIdx0 = c0 + (lIsL ? 0 : numFaces);
            uint32_t cIdx1 = c1 + (rIsL ? 0 : numFaces);
            uint32_t numCPrims0 = lIsL ? 1 : numTotalLeaves[c0];
            uint32_t numCPrims1 = rIsL ? 1 : numTotalLeaves[c1];
            bool error = (0 != parentIdxs[cIdx0] || 0 != parentIdxs[cIdx1] ||
                          numTotalLeaves[0] != numCPrims0 + numCPrims1);
            
            printf("%5u |%5u%c, %5u%c|(%10.6f, %10.6f, %10.6f), (%10.6f, %10.6f, %10.6f)|A: %10.6f, #P: %5u, $: %10.6f, #V: %u, #R: %u%s\n", 0,
                   iNode.c[0], iNode.isLeaf[0] ? 'L' : ' ',
                   iNode.c[1], iNode.isLeaf[1] ? 'L' : ' ',
                   iNode.bbox.min.x, iNode.bbox.min.y, iNode.bbox.min.z, iNode.bbox.max.x, iNode.bbox.max.y, iNode.bbox.max.z,
                   surfaceArea, numTotalLeaves[0], SAHCosts[numFaces + 0], counters[0], refCounts[numFaces + 0], error ? " error" : "");
        }
        
        for (uint32_t i = 0; i < internalNodes.size(); ++i) {
            InternalNode &iNode = internalNodes[i];
            uint32_t cIdx0 = iNode.c[0] + (iNode.isLeaf[0] ? 0 : numFaces);
            uint32_t cIdx1 = iNode.c[1] + (iNode.isLeaf[1] ? 0 : numFaces);
            ++refCounts[cIdx0];
            ++refCounts[cIdx1];
        }
        
        bool valid = true;
        
        if (!quiet) {
            printf("Internal Nodes\n");
        }
        for (uint32_t i = 0; i < internalNodes.size(); ++i) {
            InternalNode &iNode = internalNodes[i];
            bool lIsL = iNode.isLeaf[0];
            bool rIsL = iNode.isLeaf[1];
            uint32_t c0 = iNode.c[0];
            uint32_t c1 = iNode.c[1];
            float surfaceArea = calcSurfaceArea(iNode.bbox);
            
            InternalNode &pINode = internalNodes[parentIdxs[numFaces + i]];
            float pSurfaceArea = calcSurfaceArea(pINode.bbox);
            
            uint32_t cIdx0 = c0 + (lIsL ? 0 : numFaces);
            uint32_t cIdx1 = c1 + (rIsL ? 0 : numFaces);
            uint32_t numCPrims0 = lIsL ? 1 : numTotalLeaves[c0];
            uint32_t numCPrims1 = rIsL ? 1 : numTotalLeaves[c1];
            bool error = (i != parentIdxs[cIdx0] || i != parentIdxs[cIdx1] ||
                          pSurfaceArea < surfaceArea ||
                          numTotalLeaves[i] != numCPrims0 + numCPrims1);
            valid &= ~error;
            
            if (!quiet) {
                printf("%5u |%5u%c, %5u%c, %5u|(%10.6f, %10.6f, %10.6f), (%10.6f, %10.6f, %10.6f)|A: %10.6f, #P: %5u, $: %10.6f, #V: %u, #R: %u%s\n", i,
                       iNode.c[0], iNode.isLeaf[0] ? 'L' : ' ',
                       iNode.c[1], iNode.isLeaf[1] ? 'L' : ' ', parentIdxs[numFaces + i],
                       iNode.bbox.min.x, iNode.bbox.min.y, iNode.bbox.min.z, iNode.bbox.max.x, iNode.bbox.max.y, iNode.bbox.max.z,
                       surfaceArea, numTotalLeaves[i], SAHCosts[numFaces + i], counters[i], refCounts[numFaces + i], error ? " error" : "");
            }
        }
        if (!quiet) {
            printf("--------------------------------\n");
            printf("Leaf Nodes\n");
        }
        for (uint32_t i = 0; i < leafNodes.size(); ++i) {
            LeafNode &lNode = leafNodes[i];
            float surfaceArea = calcSurfaceArea(lNode.bbox);
            
            InternalNode &pINode = internalNodes[parentIdxs[i]];
            float pSurfaceArea = calcSurfaceArea(pINode.bbox);
            
            bool error = pSurfaceArea < surfaceArea;
            valid &= ~error;
            
            if (!quiet) {
                printf("%5uL|%5u, %5u|(%10.6f, %10.6f, %10.6f), (%10.6f, %10.6f, %10.6f)|A: %10.6f, $: %10.6f, #R: %u%s\n", i,
                       lNode.objIdx, parentIdxs[i],
                       lNode.bbox.min.x, lNode.bbox.min.y, lNode.bbox.min.z, lNode.bbox.max.x, lNode.bbox.max.y, lNode.bbox.max.z,
                       surfaceArea, SAHCosts[i], refCounts[i], error ? " error" : "");
            }
        }
        
        if (dump) {
            std::ofstream ofs;
            ofs.open("node_data", std::ios::out | std::ios::binary | std::ios::trunc);
            
            ofs.write("INODES", 7);
            size_t numINodes = internalNodes.size();
            ofs.write((const char*)&numINodes, sizeof(size_t));
            for (uint32_t i = 0; i < internalNodes.size(); ++i)
                ofs.write((const char*)&internalNodes[i], sizeof(InternalNode));
            
            ofs.write("LNODES", 7);
            size_t numLNodes = leafNodes.size();
            ofs.write((const char*)&numLNodes, sizeof(size_t));
            for (uint32_t i = 0; i < leafNodes.size(); ++i)
                ofs.write((const char*)&leafNodes[i], sizeof(LeafNode));
        }
        
        return valid;
    }
    
    void Builder::loadFromBinary(cl::CommandQueue &queue, const std::string &filepath, cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes) {
        std::ifstream ifs;
        ifs.open(filepath, std::ios::in | std::ios::binary);
        char strBuffer[256];
        size_t size_t_store;
        std::vector<InternalNode> internalNodes;
        std::vector<LeafNode> leafNodes;
        
        ifs.read(strBuffer, 7);
        ifs.read((char*)&size_t_store, sizeof(size_t));
        internalNodes.resize(size_t_store);
        for (uint32_t i = 0; i < internalNodes.size(); ++i)
            ifs.read((char*)&internalNodes[i], sizeof(InternalNode));
        
        ifs.read(strBuffer, 7);
        ifs.read((char*)&size_t_store, sizeof(size_t));
        leafNodes.resize(size_t_store);
        for (uint32_t i = 0; i < leafNodes.size(); ++i)
            ifs.read((char*)&leafNodes[i], sizeof(LeafNode));
        
        queue.enqueueWriteBuffer(bufInternalNodes, CL_TRUE, 0, internalNodes.size() * sizeof(InternalNode), internalNodes.data());
        queue.enqueueWriteBuffer(bufLeafNodes, CL_TRUE, 0, leafNodes.size() * sizeof(LeafNode), leafNodes.data());
    }
    
    
    // 各ノードのAABBを計算する。
    void Builder::calcNodeAABBs_SAHCosts(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                                         cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, uint32_t numFaces) {
        events.emplace_back();
        const uint32_t workSize = ((numFaces + (localSizeCalcNodeAABBs - 1)) / localSizeCalcNodeAABBs) * localSizeCalcNodeAABBs;
        cl::enqueueNDRangeKernel(queue, m_kernelCalcAABBs_SAHCosts, cl::NullRange, cl::NDRange(workSize), cl::NDRange(localSizeCalcNodeAABBs), nullptr, &events.back(),
                                 bufInternalNodes, m_bufCounters, bufLeafNodes, numFaces, m_bufParentIdxs, m_bufNumTotalLeaves, m_bufSAHCosts);
        queue.enqueueBarrierWithWaitList();
    }
    
    // BVHの最適化を行う。
    void Builder::treeletRestructuring(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                                       cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, uint32_t numFaces) {
        const uint32_t workSizeRestructuring = CLUtil::largerMultiple(numFaces, localSizeRestructuring);
        for (int i = 0; i < 3; ++i) {
            events.emplace_back();
            cl::enqueueNDRangeKernel(queue, m_kernelTreeletRestructuring, cl::NullRange, cl::NDRange(workSizeRestructuring), cl::NDRange(localSizeRestructuring), nullptr, &events.back(),
                                     bufInternalNodes, m_bufCounters, bufLeafNodes, numFaces, m_bufParentIdxs, m_bufNumTotalLeaves, m_bufSAHCosts, i);
            queue.enqueueBarrierWithWaitList();// 途中には不要？
        }
    }
    
    
    void Builder::perform(cl::CommandQueue &queue,
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
        calcNodeAABBs_SAHCosts(queue, events, bufInternalNodes, bufLeafNodes, numFaces);
        if (profiling) {
            queue.finish();
            events.back().wait();
            CLUtil::getProfilingInfo(events.back(), &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            printf("calculating node-AABBs done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
        }
        validate(queue, numFaces, bufInternalNodes, bufLeafNodes, true);
        
        // Treelet単位で木構造の最適化を行う。
        evStart = (uint32_t)events.size();
        if (profiling)
            stopwatchHiRes.start();
        treeletRestructuring(queue, events, bufInternalNodes, bufLeafNodes, numFaces);
        if (profiling) {
            queue.finish();
            cl_ulong sumTimeRestructuring = 0, sumTimeRestructuringFromSubmit = 0;
            for (uint32_t i = evStart; i < events.size(); ++i) {
                events[i].wait();
                CLUtil::getProfilingInfo(events[i], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
                sumTimeRestructuring += tpCmdEnd - tpCmdStart;
                sumTimeRestructuringFromSubmit += tpCmdEnd - tpCmdSubmit;
            }
            printf("treelet restructuring done! ... time: %fusec (%fusec)\n", sumTimeRestructuring * 0.001f, stopwatchHiRes.stop(StopWatchHiRes::Nanoseconds) * 0.001f);
        }
        bool valid = validate(queue, numFaces, bufInternalNodes, bufLeafNodes, false);
        if (!valid)
            printf("invalid!!\n");
        printf("");
    };
}
