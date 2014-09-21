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
    class Technique {
    protected:
        cl::Context m_context;
        cl::Device m_device;
        
    public:
        Technique(cl::Context &context, cl::Device &device) : m_context(context), m_device(device) {};
        
        cl::Context& getContext() {
            return m_context;
        }
        
        cl::Device& getDevice() {
            return m_device;
        }
    };
    
    class GlobalScan : public Technique {
        cl::Kernel m_kernelLocalScan;
        cl::Kernel m_kernelAddOffset;
    
    public:
        GlobalScan(cl::Context &context, cl::Device &device);
        
        void perform(cl::CommandQueue &queue,
                     cl::Buffer &bufElements, uint32_t numElements, std::vector<cl::Event> &events);
    };
}

#endif /* defined(__OpenCL_PathTracer__CLGenericKernels__) */
