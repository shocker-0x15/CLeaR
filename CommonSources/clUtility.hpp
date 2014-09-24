//
//  clUtility.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_clUtility_hpp
#define OpenCL_PathTracer_clUtility_hpp

#include "cl12.hpp"

void initCLUtility();

void printErrorFromCode(cl_int err_code, char* err_str);

void printDeviceInfo(const cl::Device &device, cl_device_info info);

void getProfilingInfo(const cl::Event &ev, cl_ulong* cmdStart, cl_ulong* cmdEnd, cl_ulong* cmdSubmit = nullptr);

namespace cl {
#define KERNEL_COMMON_ARGS const CommandQueue &queue, Kernel &kernel, const NDRange &offset, const NDRange &global, const NDRange &local, const VECTOR_CLASS<Event>* events, Event* event
    template <typename T0>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0) {
        kernel.setArg(0, t0);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3, typename T4>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        kernel.setArg(4, t4);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4,
                                       const T5 &t5) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        kernel.setArg(4, t4);
        kernel.setArg(5, t5);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4,
                                       const T5 &t5, const T6 &t6) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        kernel.setArg(4, t4);
        kernel.setArg(5, t5);
        kernel.setArg(6, t6);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4,
                                       const T5 &t5, const T6 &t6, const T7 &t7) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        kernel.setArg(4, t4);
        kernel.setArg(5, t5);
        kernel.setArg(6, t6);
        kernel.setArg(7, t7);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4,
                                       const T5 &t5, const T6 &t6, const T7 &t7, const T8 &t8) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        kernel.setArg(4, t4);
        kernel.setArg(5, t5);
        kernel.setArg(6, t6);
        kernel.setArg(7, t7);
        kernel.setArg(8, t8);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
    template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    inline cl_int enqueueNDRangeKernel(KERNEL_COMMON_ARGS,
                                       const T0 &t0, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4,
                                       const T5 &t5, const T6 &t6, const T7 &t7, const T8 &t8, const T9 &t9) {
        kernel.setArg(0, t0);
        kernel.setArg(1, t1);
        kernel.setArg(2, t2);
        kernel.setArg(3, t3);
        kernel.setArg(4, t4);
        kernel.setArg(5, t5);
        kernel.setArg(6, t6);
        kernel.setArg(7, t7);
        kernel.setArg(8, t8);
        kernel.setArg(9, t9);
        return queue.enqueueNDRangeKernel(kernel, offset, global, local, events, event);
    }
}

std::string stringFromFile(const char* filename);

cl_float2 makeCLfloat2(float x, float y);

cl_float3 makeCLfloat3(float x, float y, float z);

cl_float4 makeCLfloat4(float x, float y, float z, float w);

cl_float3 minCLfloat3(const cl_float3 &a, const cl_float3 &b);

cl_float3 maxCLfloat3(const cl_float3 &a, const cl_float3 &b);

uint64_t align(std::vector<uint8_t>* vec, uint32_t alignment);

uint64_t addDataAligned(std::vector<uint8_t>* dest, void* data, size_t bytes, uint32_t alignment);

template <typename T>
uint64_t addDataAligned(std::vector<uint8_t>* dest, const T &data, uint32_t alignment = sizeof(T)) {
    size_t numFill = ((dest->size() + (alignment - 1)) & ~(alignment - 1)) - dest->size();
    dest->insert(dest->end(), numFill + sizeof(T), 0);
    memcpy(&dest->back() + 1 - sizeof(T), &data, sizeof(T));
    return dest->size() - sizeof(T);
}

uint64_t fillZerosAligned(std::vector<uint8_t>* dest, uint32_t bytes, uint32_t alignment);

#endif
