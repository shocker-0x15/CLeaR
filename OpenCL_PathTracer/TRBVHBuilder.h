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
    typedef LBVH::AABB AABB;
    typedef LBVH::InternalNode InternalNode;
    typedef LBVH::LeafNode LeafNode;
    
    class Builder : public LBVH::Builder {
        enum class BuildPass : uint32_t {
            calcAABBs,
            unifyAABBs,
            calcMortonCodes,
            blockwiseSort,
            calcBlockHistograms,
            globalScan,
            globalScatter,
            constructBinaryRadixTree,
            calcNodeAABBs,
            TreeletRestructuring, 
            Num
        };
        
        cl::Kernel m_kernelCalcAABBs_SAHCosts;
        cl::Kernel m_kernelTreeletRestructuring;
        
        const uint32_t localSizeRestructuring;
        
        cl::Buffer m_bufNumTotalLeaves;
        cl::Buffer m_bufSAHCosts;
        void setupWorkingBuffers() override;
        
        // for debugging
        bool validate(cl::CommandQueue &queue, uint32_t numFaces,
                      cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, bool quiet, bool dump = false) const;
        
        void calcNodeAABBs_SAHCosts(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                                    cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, uint32_t numFaces);
        void treeletRestructuring(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                                  cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, uint32_t numFaces);
    public:
        Builder(cl::Context &context, cl::Device &device);
        
        void perform(cl::CommandQueue &queue,
                     const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                     cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling) override;
        
        // for debugging
        void loadFromBinary(cl::CommandQueue &queue, const std::string &filepath, cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes);
    };
}

#endif /* defined(__OpenCL_PathTracer__TRBVHBuilder__) */
