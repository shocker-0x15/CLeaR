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
    // 48bytes
    struct InternalNode {
        cl_float3 min;
        cl_float3 max;
        cl_uchar isLeaf[2]; uint8_t dum0[2];
        cl_uint c[2]; uint8_t dum1[4];
    };
    
    // 48bytes
    struct LeafNode {
        cl_float3 min;
        cl_float3 max;
        cl_uint objIdx; uint8_t dum0[12];
    };
    
    class Builder : public CLUtil::Technique {
        uint32_t m_currentCapacity;
        cl::Buffer m_bufferGenericPool;
        bool m_createdBuffers;
        
        LBVH::Builder m_techLBVHBuilder;
        
        void setupWorkingBuffers();
    public:
        Builder(cl::Context &context, cl::Device &device, uint32_t numFaces);
        
        void perform(cl::CommandQueue &queue,
                     const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                     cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling);
    };
}

#endif /* defined(__OpenCL_PathTracer__TRBVHBuilder__) */
