//
//  TRBVHBuilder.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/11/19.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_PathTracer__TRBVHBuilder__
#define __OpenCL_PathTracer__TRBVHBuilder__

#include "clUtility.hpp"
#include "LBVHBuilder.h"

namespace TRBVH {
    class Builder : public LBVH::Builder {
        cl::Kernel m_kernelCalcAABBs_SAHCosts;
        cl::Kernel m_kernelTreeletRestructuring;
        
        const uint32_t localSizeBottomUp;
        
        cl::Buffer m_bufNumTotalLeaves;
    public:
        Builder(cl::Context &context, cl::Device &device, uint32_t numFaces);
        
        void perform(cl::CommandQueue &queue,
                     const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                     cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling) override;
    };
}

#endif /* defined(__OpenCL_PathTracer__TRBVHBuilder__) */
