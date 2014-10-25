//
//  CLGenericKernels.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/09/21.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "CLGenericKernels.h"
#include <string>
#include <fstream>

namespace CLGeneric {
    GlobalScan::GlobalScan(cl::Context &context, cl::Device &device) : Technique(context, device) {
        std::string buildLog;
        std::string rawStrBuildAccel = CLUtil::stringFromFile("generic_kernels.cl");
        cl::Program::Sources srcBuildAccel{1, std::make_pair(rawStrBuildAccel.c_str(), rawStrBuildAccel.length())};
        
        cl::Program programBuildAccel{context, srcBuildAccel};
        programBuildAccel.build("");
        programBuildAccel.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("generic kernels build log: \n");
        printf("%s\n", buildLog.c_str());
        
        m_kernelLocalScan = cl::Kernel(programBuildAccel, "localScan");
        m_kernelAddOffset = cl::Kernel(programBuildAccel, "addOffset");
    }
    
    void GlobalScan::perform(cl::CommandQueue &queue,
                             cl::Buffer &bufElements, uint32_t numElements, std::vector<cl::Event> &events) {
        const uint32_t blockSize = 128;
        
        cl::Buffer bufBlockSums;
        
        cl::Buffer bufElementsStack[5];
        uint32_t numElemsStack[5];
        uint32_t depth = 0;
        bufElementsStack[depth] = bufElements;
        numElemsStack[depth] = numElements;
        ++depth;
        while (numElements > 1) {
            events.emplace_back();
            
            uint32_t numBlocks = ((numElements + (blockSize - 1))) / blockSize;
            uint32_t workSize = numBlocks * blockSize;
            cl::Buffer bufBlockSums{m_context, CL_MEM_READ_WRITE, numBlocks * sizeof(uint32_t), nullptr, nullptr};
            
            m_kernelLocalScan.setArg(0, bufElementsStack[depth - 1]);
            m_kernelLocalScan.setArg(1, numElements);
            m_kernelLocalScan.setArg(2, bufBlockSums);
            queue.enqueueNDRangeKernel(m_kernelLocalScan, cl::NullRange, cl::NDRange(workSize),
                                       cl::NDRange(blockSize), nullptr, &events.back());
            queue.enqueueBarrierWithWaitList();
            
            numElements = numBlocks;
            numBlocks = ((numElements + (blockSize - 1))) / blockSize;
            bufElementsStack[depth] = bufBlockSums;
            numElemsStack[depth] = numElements;
            ++depth;
        }
        
        --depth;
        while (depth > 1) {
            events.emplace_back();
            
            --depth;
            numElements = numElemsStack[depth - 1];
            uint32_t numBlocks = ((numElements + (blockSize - 1))) / blockSize;
            uint32_t workSize = numBlocks * blockSize;
            
            m_kernelAddOffset.setArg(0, bufElementsStack[depth]);
            m_kernelAddOffset.setArg(1, bufElementsStack[depth - 1]);
            m_kernelAddOffset.setArg(2, numElements);
            queue.enqueueNDRangeKernel(m_kernelAddOffset, cl::NullRange, cl::NDRange(workSize),
                                       cl::NDRange(blockSize), nullptr, &events.back());
            queue.enqueueBarrierWithWaitList();
        }
    }
    
    void GlobalScan::calcWorkingBuffersRequirement(uint32_t numElements, uint64_t* size, uint32_t* align) {
        const uint32_t blockSize = 128;
        uint32_t numElems = numElements;
        *size = 0;
        *align = sizeof(uint32_t);
        while (numElems > 1) {
            numElems = ((numElems + (blockSize - 1))) / blockSize;
            *size += numElems * sizeof(uint32_t);
        }
    }
    
    void GlobalScan::createWorkingBuffers(uint32_t numElements, cl::Buffer &pool, uint64_t offset, std::vector<cl::Buffer>* buffers, uint64_t* nextAddress) {
        const uint32_t blockSize = 128;
        
        uint32_t numElems = numElements;
        cl_buffer_region region = {offset, 0};
        while (numElems > 1) {
            numElems = ((numElems + (blockSize - 1))) / blockSize;
            region.origin += region.size;
            region.size = numElems * sizeof(uint32_t);
            buffers->push_back(pool.createSubBuffer(CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION, &region));
        }
        *nextAddress = region.origin + region.size;
    }
    
    void GlobalScan::perform(cl::CommandQueue &queue,
                             cl::Buffer &bufElements, uint32_t numElements, const std::vector<cl::Buffer> &workingBuffers,
                             std::vector<cl::Event> &events) {
        const uint32_t blockSize = 128;
        
        cl::Buffer bufElementsStack[5];
        uint32_t numElemsStack[5];
        uint32_t depth = 0;
        bufElementsStack[depth] = bufElements;
        numElemsStack[depth] = numElements;
        ++depth;
        while (numElements > 1) {
            events.emplace_back();
            
            uint32_t numBlocks = ((numElements + (blockSize - 1))) / blockSize;
            uint32_t workSize = numBlocks * blockSize;
            const cl::Buffer &bufBlockSums = workingBuffers[depth - 1];
            
            cl::enqueueNDRangeKernel(queue, m_kernelLocalScan, cl::NullRange, cl::NDRange(workSize),
                                     cl::NDRange(blockSize), nullptr, &events.back(),
                                     bufElementsStack[depth - 1], numElements, bufBlockSums);
            queue.enqueueBarrierWithWaitList();
            
            numElements = numBlocks;
            numBlocks = ((numElements + (blockSize - 1))) / blockSize;
            bufElementsStack[depth] = bufBlockSums;
            numElemsStack[depth] = numElements;
            ++depth;
        }
        
        --depth;
        while (depth > 1) {
            events.emplace_back();
            
            --depth;
            numElements = numElemsStack[depth - 1];
            uint32_t numBlocks = ((numElements + (blockSize - 1))) / blockSize;
            uint32_t workSize = numBlocks * blockSize;
            
            cl::enqueueNDRangeKernel(queue, m_kernelAddOffset, cl::NullRange, cl::NDRange(workSize),
                                     cl::NDRange(blockSize), nullptr, &events.back(),
                                     bufElementsStack[depth], bufElementsStack[depth - 1], numElements);
            queue.enqueueBarrierWithWaitList();
        }
    }
}
