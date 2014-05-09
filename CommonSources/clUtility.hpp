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

void printErrorFromCode(cl_int err_code, char* err_str) {
    switch (err_code) {
        case CL_SUCCESS:
            strcpy(err_str, "SUCCESS");
            break;
        case CL_DEVICE_NOT_FOUND:
            strcpy(err_str, "DEVICE_NOT_FOUND");
            break;
        case CL_DEVICE_NOT_AVAILABLE:
            strcpy(err_str, "DEVICE_NOT_AVAILABLE");
            break;
        case CL_COMPILER_NOT_AVAILABLE:
            strcpy(err_str, "COMPILER_NOT_AVAILABLE");
            break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            strcpy(err_str, "MEM_OBJECT_ALLOCATION_FAILURE");
            break;
        case CL_OUT_OF_RESOURCES:
            strcpy(err_str, "OUT_OF_RESOURCES");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            strcpy(err_str, "OUT_OF_HOST_MEMORY");
            break;
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            strcpy(err_str, "PROFILING_INFO_NOT_AVAILABLE");
            break;
        case CL_MEM_COPY_OVERLAP:
            strcpy(err_str, "MEM_COPY_OVERLAP");
            break;
        case CL_IMAGE_FORMAT_MISMATCH:
            strcpy(err_str, "IMAGE_FORMAT_MISMATCH");
            break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            strcpy(err_str, "IMAGE_FORMAT_NOT_SUPPORTED");
            break;
        case CL_BUILD_PROGRAM_FAILURE:
            strcpy(err_str, "BUILD_PROGRAM_FAILURE");
            break;
        case CL_MAP_FAILURE:
            strcpy(err_str, "MAP_FAILURE");
            break;
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            strcpy(err_str, "MISALIGNED_SUB_BUFFER_OFFSET");
            break;
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
            strcpy(err_str, "EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST");
            break;
        case CL_COMPILE_PROGRAM_FAILURE:
            strcpy(err_str, "COMPILE_PROGRAM_FAILURE");
            break;
        case CL_LINKER_NOT_AVAILABLE:
            strcpy(err_str, "LINKER_NOT_AVAILABLE");
            break;
        case CL_LINK_PROGRAM_FAILURE:
            strcpy(err_str, "LINK_PROGRAM_FAILURE");
            break;
        case CL_DEVICE_PARTITION_FAILED:
            strcpy(err_str, "DEVICE_PARTITION_FAILED");
            break;
        case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
            strcpy(err_str, "KERNEL_ARG_INFO_NOT_AVAILABLE");
            break;
        case CL_INVALID_VALUE:
            strcpy(err_str, "INVALID_VALUE");
            break;
        case CL_INVALID_DEVICE_TYPE:
            strcpy(err_str, "INVALID_DEVICE_TYPE");
            break;
        case CL_INVALID_PLATFORM:
            strcpy(err_str, "INVALID_PLATFORM");
            break;
        case CL_INVALID_DEVICE:
            strcpy(err_str, "INVALID_DEVICE");
            break;
        case CL_INVALID_CONTEXT:
            strcpy(err_str, "INVALID_CONTEXT");
            break;
        case CL_INVALID_QUEUE_PROPERTIES:
            strcpy(err_str, "INVALID_QUEUE_PROPERTIES");
            break;
        case CL_INVALID_COMMAND_QUEUE:
            strcpy(err_str, "INVALID_COMMAND_QUEUE");
            break;
        case CL_INVALID_HOST_PTR:
            strcpy(err_str, "INVALID_HOST_PTR");
            break;
        case CL_INVALID_MEM_OBJECT:
            strcpy(err_str, "INVALID_MEM_OBJECT");
            break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            strcpy(err_str, "INVALID_IMAGE_FORMAT_DESCRIPTOR");
            break;
        case CL_INVALID_IMAGE_SIZE:
            strcpy(err_str, "INVALID_IMAGE_SIZE");
            break;
        case CL_INVALID_SAMPLER:
            strcpy(err_str, "INVALID_SAMPLER");
            break;
        case CL_INVALID_BINARY:
            strcpy(err_str, "INVALID_BINARY");
            break;
        case CL_INVALID_BUILD_OPTIONS:
            strcpy(err_str, "INVALID_BUILD_OPTIONS");
            break;
        case CL_INVALID_PROGRAM:
            strcpy(err_str, "INVALID_PROGRAM");
            break;
        case CL_INVALID_PROGRAM_EXECUTABLE:
            strcpy(err_str, "INVALID_PROGRAM_EXECUTABLE");
            break;
        case CL_INVALID_KERNEL_NAME:
            strcpy(err_str, "INVALID_KERNEL_NAME");
            break;
        case CL_INVALID_KERNEL_DEFINITION:
            strcpy(err_str, "INVALID_KERNEL_DEFINITION");
            break;
        case CL_INVALID_KERNEL:
            strcpy(err_str, "INVALID_KERNEL");
            break;
        case CL_INVALID_ARG_INDEX:
            strcpy(err_str, "INVALID_ARG_INDEX");
            break;
        case CL_INVALID_ARG_VALUE:
            strcpy(err_str, "INVALID_ARG_VALUE");
            break;
        case CL_INVALID_ARG_SIZE:
            strcpy(err_str, "INVALID_ARG_SIZE");
            break;
        case CL_INVALID_KERNEL_ARGS:
            strcpy(err_str, "INVALID_KERNEL_ARGS");
            break;
        case CL_INVALID_WORK_DIMENSION:
            strcpy(err_str, "INVALID_WORK_DIMENSION");
            break;
        case CL_INVALID_WORK_GROUP_SIZE:
            strcpy(err_str, "INVALID_WORK_GROUP_SIZE");
            break;
        case CL_INVALID_WORK_ITEM_SIZE:
            strcpy(err_str, "INVALID_WORK_ITEM_SIZE");
            break;
        case CL_INVALID_GLOBAL_OFFSET:
            strcpy(err_str, "INVALID_GLOBAL_OFFSET");
            break;
        case CL_INVALID_EVENT_WAIT_LIST:
            strcpy(err_str, "INVALID_EVENT_WAIT_LIST");
            break;
        case CL_INVALID_EVENT:
            strcpy(err_str, "INVALID_EVENT");
            break;
        case CL_INVALID_OPERATION:
            strcpy(err_str, "INVALID_OPERATION");
            break;
        case CL_INVALID_GL_OBJECT:
            strcpy(err_str, "INVALID_GL_OBJECT");
            break;
        case CL_INVALID_BUFFER_SIZE:
            strcpy(err_str, "INVALID_BUFFER_SIZE");
            break;
        case CL_INVALID_MIP_LEVEL:
            strcpy(err_str, "INVALID_MIP_LEVEL");
            break;
        case CL_INVALID_GLOBAL_WORK_SIZE:
            strcpy(err_str, "INVALID_GLOBAL_WORK_SIZE");
            break;
        case CL_INVALID_PROPERTY:
            strcpy(err_str, "INVALID_PROPERTY");
            break;
        case CL_INVALID_IMAGE_DESCRIPTOR:
            strcpy(err_str, "INVALID_IMAGE_DESCRIPTOR");
            break;
        case CL_INVALID_COMPILER_OPTIONS:
            strcpy(err_str, "INVALID_COMPILER_OPTIONS");
            break;
        case CL_INVALID_LINKER_OPTIONS:
            strcpy(err_str, "INVALID_LINKER_OPTIONS");
            break;
        case CL_INVALID_DEVICE_PARTITION_COUNT:
            strcpy(err_str, "INVALID_DEVICE_PARTITION_COUNT");
            break;
        default:
            break;
    }
}

cl_float2 makeCLfloat2(float x, float y) {
    cl_float2 f2 = {x, y};
    return f2;
}

cl_float3 makeCLfloat3(float x, float y, float z) {
    cl_float3 f3 = {x, y, z};
    return f3;
}

cl_float4 makeCLfloat4(float x, float y, float z, float w) {
    cl_float4 f4 = {x, y, z, w};
    return f4;
}

cl_float3 minCLfloat3(const cl_float3 &a, const cl_float3 &b) {
    cl_float3 f3 = {std::min(a.s0, b.s0), std::min(a.s1, b.s1), std::min(a.s2, b.s2)};
    return f3;
}

cl_float3 maxCLfloat3(const cl_float3 &a, const cl_float3 &b) {
    cl_float3 f3 = {std::max(a.s0, b.s0), std::max(a.s1, b.s1), std::max(a.s2, b.s2)};
    return f3;
}

uint64_t align(std::vector<uint8_t>* vec, uint32_t alignment) {
    uint32_t numFill = alignment - vec->size() % alignment;
    if (numFill != alignment)
        vec->insert(vec->end(), numFill, 0);
    return vec->size();
}

uint64_t addDataAligned(std::vector<uint8_t>* dest, void* data, uint32_t bytes, uint32_t alignment) {
    uint32_t numFill = alignment - dest->size() % alignment;
    if (numFill != alignment)
        dest->insert(dest->end(), numFill, 0);
    dest->insert(dest->end(), bytes, 0);
    memcpy(&dest->back() + 1 - bytes, data, bytes);
    return dest->size() - bytes;
}

template <typename T>
uint64_t addDataAligned(std::vector<uint8_t>* dest, const T &data, uint32_t alignment = sizeof(T)) {
    uint32_t numFill = alignment - dest->size() % alignment;
    if (numFill != alignment)
        dest->insert(dest->end(), numFill, 0);
    dest->insert(dest->end(), sizeof(T), 0);
    memcpy(&dest->back() + 1 - sizeof(T), &data, sizeof(T));
    return dest->size() - sizeof(T);
}

#endif
