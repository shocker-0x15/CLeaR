//
//  LBVHBuilder.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/10/25.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_PathTracer__LBVHBuilder__
#define __OpenCL_PathTracer__LBVHBuilder__

#include "clUtility.hpp"
#include "CLGenericKernels.h"

namespace LBVH {
    // 32bytes
    struct AABB {
        cl_float3 min;
        cl_float3 max;
    };
    
    // 48bytes
    struct InternalNode {
        AABB bbox;
        cl_uchar isLeaf[2]; uint8_t dum0[2];
        cl_uint c[2]; uint8_t dum1[4];
    };
    
    // 48bytes
    struct LeafNode {
        AABB bbox;
        cl_uint objIdx; uint8_t dum0[12];
    };

    class Builder : public CLUtil::Technique {
    protected:
        cl::Kernel m_kernelCalcAABBs;
        cl::Kernel m_kernelUnifyAABBs;
        cl::Kernel m_kernelCalcMortonCodes;
        cl::Kernel m_kernelBlockwiseSort;
        cl::Kernel m_kernelCalcBlockHistograms;
        cl::Kernel m_kernelGlobalScatter;
        cl::Kernel m_kernelConstructBinaryRadixTree;
        cl::Kernel m_kernelCalcNodeAABBs;
        
        const uint32_t localSizeCalcAABBs;
        const uint32_t localSizeUnifyAABBs;
        const uint32_t localSizeCalcMortonCodes;
        const uint32_t localSizeBlockwiseSort;
        const uint32_t localSizeCalcBlockHistograms;
        const uint32_t localSizeConstructBinaryRadixTree;
        const uint32_t localSizeCalcNodeAABBs;
        
        cl::Buffer m_bufAABBs;
        cl::Buffer m_bufs_unifyAABBs[2];
        cl::Buffer m_bufMortonCodes;
        cl::Buffer m_bufIndices;
        cl::Buffer m_bufIndicesShadow;
        cl::Buffer m_bufRadixDigits;
        cl::Buffer m_bufHistograms;
        cl::Buffer m_bufLocalOffsets;
        std::vector<cl::Buffer> m_bufs_globalScan;
        cl::Buffer m_bufParentIdxs;
        cl::Buffer m_bufCounters;
        
        uint32_t m_currentCapacity;
        cl::Buffer m_bufferGenericPool;
        bool m_createdBuffers;
        
        CLGeneric::GlobalScan m_techGlobalScan;
        
        void setupWorkingBuffers();
        
        void calcAABBs(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                       const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces);
        void unifyAABBs(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                        cl::Buffer &buf_unifiedAABBs, uint32_t numFaces);
        void calcMortonCodes(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                             uint32_t numFaces, const AABB &entireAABB, uint32_t numBitsPerDim);
        void radixSort(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                       uint32_t numFaces, uint32_t numBitsPerDim);
        void constructBinaryRadixTree(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                                      cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, uint32_t numFaces, uint32_t numBitsPerDim);
        void calcNodeAABBs(cl::CommandQueue &queue, std::vector<cl::Event> &events,
                           cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, uint32_t numFaces);
    public:
        Builder(cl::Context &context, cl::Device &device, uint32_t numFaces);
        
        virtual void perform(cl::CommandQueue &queue,
                             const cl::Buffer &buf_vertices, const cl::Buffer &buf_faces, uint32_t numFaces, uint32_t numBitsPerDim,
                             cl::Buffer &bufInternalNodes, cl::Buffer &bufLeafNodes, std::vector<cl::Event> &events, bool profiling);
    };
}

#endif /* defined(__OpenCL_PathTracer__LBVHBuilder__) */
