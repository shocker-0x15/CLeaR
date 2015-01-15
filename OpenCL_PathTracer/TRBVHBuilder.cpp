//
//  TRBVHBuilder.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/11/19.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "TRBVHBuilder.h"

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
        
        m_kernelBottomUp = cl::Kernel(programTRBVH, "treeletRestructuring");
        
        m_bufNumTotalLeaves = cl::Buffer{context, CL_MEM_READ_WRITE, (numFaces - 1) * sizeof(uint32_t), nullptr, nullptr};
    }
    
    void Builder::perform(cl::CommandQueue &queue,
                          const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                          cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling) {
        LBVH::Builder::perform(queue, buf_vertices, buf_faces, numFaces, numBitsPerDim, bufInternalNodes, bufLeafNodes, events, profiling);
//        queue.finish();
//        std::vector<uint32_t> counters;
//        counters.resize(numFaces - 1);
//        queue.enqueueReadBuffer(m_bufCounters, CL_TRUE, 0, counters.size() * sizeof(uint32_t), counters.data());
//        for (uint32_t i = 0; i < counters.size(); ++i) {
//            printf("%u: %u\n", i, counters[i]);
//        }
        
        events.emplace_back();
        const uint32_t workSizeBottomUp = CLUtil::largerMultiple(numFaces, localSizeBottomUp);
        cl::enqueueNDRangeKernel(queue, m_kernelBottomUp, cl::NullRange, cl::NDRange(workSizeBottomUp), cl::NDRange(localSizeBottomUp), nullptr, &events.back(),
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
