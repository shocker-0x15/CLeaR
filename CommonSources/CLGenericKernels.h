//
//  CLGenericKernels.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/09/21.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_PathTracer__CLGenericKernels__
#define __OpenCL_PathTracer__CLGenericKernels__

#include "clUtility.hpp"

namespace CLGeneric {    
    class GlobalScan : public CLUtil::Technique {
        cl::Kernel m_kernelLocalScan;
        cl::Kernel m_kernelAddOffset;
    
    public:
        GlobalScan(cl::Context &context, cl::Device &device);
        
        void perform(cl::CommandQueue &queue,
                     cl::Buffer &bufElements, uint32_t numElements, std::vector<cl::Event> &events);
        
        void calcWorkingBuffersRequirement(uint32_t numElements, uint64_t* size, uint32_t* align);
        void createWorkingBuffers(uint32_t numElements, cl::Buffer &pool, uint64_t offset, std::vector<cl::Buffer>* buffers, uint64_t* nextAddress);
        void perform(cl::CommandQueue &queue,
                     cl::Buffer &bufElements, uint32_t numElements, const std::vector<cl::Buffer> &workingBuffers,
                     std::vector<cl::Event> &events);
    };
}

#endif /* defined(__OpenCL_PathTracer__CLGenericKernels__) */
