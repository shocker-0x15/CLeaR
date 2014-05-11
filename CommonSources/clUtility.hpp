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

void printErrorFromCode(cl_int err_code, char* err_str);

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

#endif
