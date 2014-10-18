//
//  clUtility.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/10.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "clUtility.hpp"
#include <map>
#include <fstream>

bool initialized = false;

enum CLDeviceInfoType {
    CLDeviceInfoType_uint,
    CLDeviceInfoType_ulong,
    CLDeviceInfoType_size_t,
    CLDeviceInfoType_size_t_vec,
    CLDeviceInfoType_bool,
    CLDeviceInfoType_string,
    CLDeviceInfoType_platform_id,
    CLDeviceInfoType_device_id,
    CLDeviceInfoType_command_queue_properties,
    CLDeviceInfoType_device_fp_config,
    CLDeviceInfoType_device_exec_capabilities,
    CLDeviceInfoType_device_type,
    CLDeviceInfoType_device_mem_cache_type,
    CLDeviceInfoType_device_local_mem_type,
    CLDeviceInfoType_device_partition_property_vec,
    CLDeviceInfoType_device_affinity_domain
};

struct CLDeviceInfoMetaInfo {
    CLDeviceInfoType infoType;
    const char* name;
};

std::map<cl_device_info, CLDeviceInfoMetaInfo> clDeviceInfoMetaInfos;

void initCLUtility() {
    if (initialized)
        return;
    
    clDeviceInfoMetaInfos[CL_DEVICE_ADDRESS_BITS] = {CLDeviceInfoType_uint, "CL_DEVICE_ADDRESS_BITS"};
    clDeviceInfoMetaInfos[CL_DEVICE_AVAILABLE] = {CLDeviceInfoType_bool, "CL_DEVICE_AVAILABLE"};
    clDeviceInfoMetaInfos[CL_DEVICE_BUILT_IN_KERNELS] = {CLDeviceInfoType_string, "CL_DEVICE_BUILT_IN_KERNELS"};
    clDeviceInfoMetaInfos[CL_DEVICE_COMPILER_AVAILABLE] = {CLDeviceInfoType_bool, "CL_DEVICE_COMPILER_AVAILABLE"};
    clDeviceInfoMetaInfos[CL_DEVICE_DOUBLE_FP_CONFIG] = {CLDeviceInfoType_device_fp_config, "CL_DEVICE_DOUBLE_FP_CONFIG"};
    clDeviceInfoMetaInfos[CL_DEVICE_ENDIAN_LITTLE] = {CLDeviceInfoType_bool, "CL_DEVICE_ENDIAN_LITTLE"};
    clDeviceInfoMetaInfos[CL_DEVICE_ERROR_CORRECTION_SUPPORT] = {CLDeviceInfoType_bool, "CL_DEVICE_ERROR_CORRECTION_SUPPORT"};
    clDeviceInfoMetaInfos[CL_DEVICE_EXECUTION_CAPABILITIES] = {CLDeviceInfoType_device_exec_capabilities, "CL_DEVICE_EXECUTION_CAPABILITIES"};
    clDeviceInfoMetaInfos[CL_DEVICE_EXTENSIONS] = {CLDeviceInfoType_string, "CL_DEVICE_EXTENSIONS"};
    clDeviceInfoMetaInfos[CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE] = {CLDeviceInfoType_uint, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_GLOBAL_MEM_CACHE_SIZE] = {CLDeviceInfoType_ulong, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_GLOBAL_MEM_CACHE_TYPE] = {CLDeviceInfoType_device_mem_cache_type, "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE"};
    clDeviceInfoMetaInfos[CL_DEVICE_GLOBAL_MEM_SIZE] = {CLDeviceInfoType_ulong, "CL_DEVICE_GLOBAL_MEM_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_HALF_FP_CONFIG] = {CLDeviceInfoType_device_fp_config, "CL_DEVICE_HALF_FP_CONFIG"};
    clDeviceInfoMetaInfos[CL_DEVICE_HOST_UNIFIED_MEMORY] = {CLDeviceInfoType_bool, "CL_DEVICE_HOST_UNIFIED_MEMORY"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE2D_MAX_HEIGHT] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE2D_MAX_HEIGHT"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE2D_MAX_WIDTH] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE2D_MAX_WIDTH"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE3D_MAX_DEPTH] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE3D_MAX_DEPTH"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE3D_MAX_HEIGHT] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE3D_MAX_HEIGHT"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE3D_MAX_WIDTH] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE3D_MAX_WIDTH"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT] = {CLDeviceInfoType_uint, "CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE_MAX_ARRAY_SIZE] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE_MAX_ARRAY_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE_MAX_BUFFER_SIZE] = {CLDeviceInfoType_size_t, "CL_DEVICE_IMAGE_MAX_BUFFER_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE_PITCH_ALIGNMENT] = {CLDeviceInfoType_uint, "CL_DEVICE_IMAGE_PITCH_ALIGNMENT"};
    clDeviceInfoMetaInfos[CL_DEVICE_IMAGE_SUPPORT] = {CLDeviceInfoType_bool, "CL_DEVICE_IMAGE_SUPPORT"};
    clDeviceInfoMetaInfos[CL_DEVICE_LINKER_AVAILABLE] = {CLDeviceInfoType_bool, "CL_DEVICE_LINKER_AVAILABLE"};
    clDeviceInfoMetaInfos[CL_DEVICE_LOCAL_MEM_SIZE] = {CLDeviceInfoType_ulong, "CL_DEVICE_LOCAL_MEM_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_LOCAL_MEM_TYPE] = {CLDeviceInfoType_device_local_mem_type, "CL_DEVICE_LOCAL_MEM_TYPE"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_CLOCK_FREQUENCY] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_CLOCK_FREQUENCY"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_COMPUTE_UNITS] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_COMPUTE_UNITS"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_CONSTANT_ARGS] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_CONSTANT_ARGS"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE] = {CLDeviceInfoType_ulong, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_MEM_ALLOC_SIZE] = {CLDeviceInfoType_ulong, "CL_DEVICE_MAX_MEM_ALLOC_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_PARAMETER_SIZE] = {CLDeviceInfoType_size_t, "CL_DEVICE_MAX_PARAMETER_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_READ_IMAGE_ARGS] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_READ_IMAGE_ARGS"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_SAMPLERS] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_SAMPLERS"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_WORK_GROUP_SIZE] = {CLDeviceInfoType_size_t, "CL_DEVICE_MAX_WORK_GROUP_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_WORK_ITEM_SIZES] = {CLDeviceInfoType_size_t_vec, "CL_DEVICE_MAX_WORK_ITEM_SIZES"};
    clDeviceInfoMetaInfos[CL_DEVICE_MAX_WRITE_IMAGE_ARGS] = {CLDeviceInfoType_uint, "CL_DEVICE_MAX_WRITE_IMAGE_ARGS"};
    clDeviceInfoMetaInfos[CL_DEVICE_MEM_BASE_ADDR_ALIGN] = {CLDeviceInfoType_uint, "CL_DEVICE_MEM_BASE_ADDR_ALIGN"};
    clDeviceInfoMetaInfos[CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE] = {CLDeviceInfoType_uint, "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_NAME] = {CLDeviceInfoType_string, "CL_DEVICE_NAME"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_INT] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG"};
    clDeviceInfoMetaInfos[CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT] = {CLDeviceInfoType_uint, "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT"};
    clDeviceInfoMetaInfos[CL_DEVICE_OPENCL_C_VERSION] = {CLDeviceInfoType_string, "CL_DEVICE_OPENCL_C_VERSION"};
    clDeviceInfoMetaInfos[CL_DEVICE_PARENT_DEVICE] = {CLDeviceInfoType_device_id, "CL_DEVICE_PARENT_DEVICE"};
    clDeviceInfoMetaInfos[CL_DEVICE_PARTITION_AFFINITY_DOMAIN] = {CLDeviceInfoType_device_affinity_domain, "CL_DEVICE_PARTITION_AFFINITY_DOMAIN"};
    clDeviceInfoMetaInfos[CL_DEVICE_PARTITION_MAX_SUB_DEVICES] = {CLDeviceInfoType_uint, "CL_DEVICE_PARTITION_MAX_SUB_DEVICES"};
    clDeviceInfoMetaInfos[CL_DEVICE_PARTITION_PROPERTIES] = {CLDeviceInfoType_device_partition_property_vec, "CL_DEVICE_PARTITION_PROPERTIES"};
    clDeviceInfoMetaInfos[CL_DEVICE_PARTITION_TYPE] = {CLDeviceInfoType_device_partition_property_vec, "CL_DEVICE_PARTITION_TYPE"};
    clDeviceInfoMetaInfos[CL_DEVICE_PLATFORM] = {CLDeviceInfoType_platform_id, "CL_DEVICE_PLATFORM"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_INTEROP_USER_SYNC] = {CLDeviceInfoType_bool, "CL_DEVICE_PREFERRED_INTEROP_USER_SYNC"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG"};
    clDeviceInfoMetaInfos[CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT] = {CLDeviceInfoType_uint, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT"};
    clDeviceInfoMetaInfos[CL_DEVICE_PRINTF_BUFFER_SIZE] = {CLDeviceInfoType_size_t, "CL_DEVICE_PRINTF_BUFFER_SIZE"};
    clDeviceInfoMetaInfos[CL_DEVICE_PROFILE] = {CLDeviceInfoType_string, "CL_DEVICE_PROFILE"};
    clDeviceInfoMetaInfos[CL_DEVICE_PROFILING_TIMER_RESOLUTION] = {CLDeviceInfoType_size_t, "CL_DEVICE_PROFILING_TIMER_RESOLUTION"};
    clDeviceInfoMetaInfos[CL_DEVICE_QUEUE_PROPERTIES] = {CLDeviceInfoType_command_queue_properties, "CL_DEVICE_QUEUE_PROPERTIES"};
    clDeviceInfoMetaInfos[CL_DEVICE_REFERENCE_COUNT] = {CLDeviceInfoType_uint, "CL_DEVICE_REFERENCE_COUNT"};
    clDeviceInfoMetaInfos[CL_DEVICE_SINGLE_FP_CONFIG] = {CLDeviceInfoType_device_fp_config, "CL_DEVICE_SINGLE_FP_CONFIG"};
    clDeviceInfoMetaInfos[CL_DEVICE_TYPE] = {CLDeviceInfoType_device_type, "CL_DEVICE_TYPE"};
    clDeviceInfoMetaInfos[CL_DEVICE_VENDOR] = {CLDeviceInfoType_string, "CL_DEVICE_VENDOR"};
    clDeviceInfoMetaInfos[CL_DEVICE_VENDOR_ID] = {CLDeviceInfoType_uint, "CL_DEVICE_VENDOR_ID"};
    clDeviceInfoMetaInfos[CL_DEVICE_VERSION] = {CLDeviceInfoType_string, "CL_DEVICE_VERSION"};
    clDeviceInfoMetaInfos[CL_DRIVER_VERSION] = {CLDeviceInfoType_string, "CL_DRIVER_VERSION"};
    
    initialized = true;
}

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

void printDeviceInfo(const cl::Device &device, cl_device_info info) {
    if (!initialized) {
        printf("not initialized. call initCLUtility().");
    }
    
    printf("%s: ", clDeviceInfoMetaInfos[info].name);
    
    switch (clDeviceInfoMetaInfos[info].infoType) {
        case CLDeviceInfoType_bool: {
            cl_bool val;
            device.getInfo(info, &val);
            printf(val != 0 ? "YES" : "NO");
            break;
        }
        case CLDeviceInfoType_uint: {
            cl_uint val;
            device.getInfo(info, &val);
            printf("%u", val);
            break;
        }
        case CLDeviceInfoType_ulong: {
            cl_ulong val;
            device.getInfo(info, &val);
            printf("%llu", val);
            break;
        }
        case CLDeviceInfoType_size_t: {
            size_t val;
            device.getInfo(info, &val);
            printf("%lu", val);
            break;
        }
        case CLDeviceInfoType_string: {
            std::string val;
            device.getInfo(info, &val);
            printf("%s", val.c_str());
            break;
        }
        case CLDeviceInfoType_size_t_vec: {
            VECTOR_CLASS<size_t> val;
            device.getInfo(info, &val);
            for (uint32_t i = 0; i < val.size() - 1; ++i) {
                printf("%lu, ", val[i]);
            }
            printf("%lu", val.back());
            break;
        }
        case CLDeviceInfoType_device_id: {
            cl_device_id val;
            device.getInfo(info, &val);
            printf("%#018llx", (uint64_t)val);
            break;
        }
        case CLDeviceInfoType_platform_id: {
            cl_platform_id val;
            device.getInfo(info, &val);
            printf("%#018llx", (uint64_t)val);
            break;
        }
        case CLDeviceInfoType_device_type: {
            cl_device_type val;
            device.getInfo(info, &val);
            switch (val) {
                case CL_DEVICE_TYPE_CPU:
                    printf("CPU");
                    break;
                case CL_DEVICE_TYPE_GPU:
                    printf("GPU");
                    break;
                case CL_DEVICE_TYPE_ACCELERATOR:
                    printf("Accelerator");
                    break;
                case CL_DEVICE_TYPE_CUSTOM:
                    printf("Custom");
                    break;
                default:
                    break;
            }
            break;
        }
        case CLDeviceInfoType_device_fp_config: {
            cl_device_fp_config val;
            device.getInfo(info, &val);
            if ((val & CL_FP_DENORM) != 0)
                printf("CL_FP_DENORM ");
            if ((val & CL_FP_INF_NAN) != 0)
                printf("CL_FP_INF_NAN ");
            if ((val & CL_FP_ROUND_TO_NEAREST) != 0)
                printf("CL_FP_ROUND_TO_NEAREST ");
            if ((val & CL_FP_ROUND_TO_ZERO) != 0)
                printf("CL_FP_ROUND_TO_ZERO ");
            if ((val & CL_FP_ROUND_TO_INF) != 0)
                printf("CL_FP_ROUND_TO_INF ");
            if ((val & CL_FP_FMA) != 0)
                printf("CL_FP_FMA ");
            if ((val & CL_FP_SOFT_FLOAT) != 0)
                printf("CL_FP_SOFT_FLOAT ");
            if ((val & CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT) != 0)
                printf("CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT ");
            break;
        }
        case CLDeviceInfoType_device_local_mem_type: {
            cl_device_local_mem_type val;
            device.getInfo(info, &val);
            switch (val) {
                case CL_LOCAL:
                    printf("Local");
                    break;
                case CL_GLOBAL:
                    printf("Global");
                default:
                    break;
            }
            break;
        }
        case CLDeviceInfoType_device_mem_cache_type: {
            cl_device_mem_cache_type val;
            device.getInfo(info, &val);
            switch (val) {
                case CL_NONE:
                    printf("None");
                    break;
                case CL_READ_ONLY_CACHE:
                    printf("Read Only Cache");
                    break;
                case CL_READ_WRITE_CACHE:
                    printf("Read Write Cache");
                    break;
                default:
                    break;
            }
            break;
        }
        case CLDeviceInfoType_device_exec_capabilities: {
            cl_device_exec_capabilities val;
            device.getInfo(info, &val);
            if ((val & CL_EXEC_KERNEL) != 0)
                printf("CL_EXEC_KERNEL ");
            if ((val & CL_EXEC_NATIVE_KERNEL) != 0)
                printf("CL_EXEC_NATIVE_KERNEL ");
            break;
        }
        case CLDeviceInfoType_command_queue_properties: {
            cl_command_queue_properties val;
            device.getInfo(info, &val);
            if ((val & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) != 0)
                printf("CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ");
            if ((val & CL_QUEUE_PROFILING_ENABLE) != 0)
                printf("CL_QUEUE_PROFILING_ENABLE ");
            break;
        }
        case CLDeviceInfoType_device_affinity_domain: {
            cl_device_affinity_domain val;
            device.getInfo(info, &val);
            if ((val & CL_DEVICE_AFFINITY_DOMAIN_NUMA) != 0)
                printf("CL_DEVICE_AFFINITY_DOMAIN_NUMA ");
            if ((val & CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE) != 0)
                printf("CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE ");
            if ((val & CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE) != 0)
                printf("CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE ");
            if ((val & CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE) != 0)
                printf("CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE ");
            if ((val & CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE) != 0)
                printf("CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE ");
            if ((val & CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE) != 0)
                printf("CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE ");
            break;
        }
        case CLDeviceInfoType_device_partition_property_vec: {
            VECTOR_CLASS<cl_device_partition_property> val;
            device.getInfo(info, &val);
            for (uint32_t i = 0; i < val.size(); ++i) {
                switch (val[i]) {
                    case CL_DEVICE_PARTITION_EQUALLY:
                        printf("CL_DEVICE_PARTITION_EQUALLY");
                        break;
                    case CL_DEVICE_PARTITION_BY_COUNTS:
                        printf("CL_DEVICE_PARTITION_BY_COUNTS");
                        break;
                    case CL_DEVICE_PARTITION_BY_COUNTS_LIST_END:
                        printf("CL_DEVICE_PARTITION_BY_COUNTS_LIST_END");
                        break;
                    case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN: 
                        printf("CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN");
                        break;
                    default:
                        break;
                }
                if (i < val.size() - 1)
                    printf(", ");
            }
            break;
        }
        default:
            break;
    }
    printf("\n");
}

void getProfilingInfo(const cl::Event &ev, cl_ulong* cmdStart, cl_ulong* cmdEnd, cl_ulong* cmdSubmit) {
    ev.getProfilingInfo(CL_PROFILING_COMMAND_START, cmdStart);
    ev.getProfilingInfo(CL_PROFILING_COMMAND_END, cmdEnd);
    if (cmdSubmit)
        ev.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, cmdSubmit);
}

namespace cl {
    Buffer createSubBuffer(Buffer &buffer,
                           cl_mem_flags flags, cl_buffer_create_type buffer_create_type,
                           uint64_t origin, uint64_t align, uint64_t size,
                           uint64_t* next, cl_int* err) {
        cl_buffer_region region;
        region.origin = (origin + (align - 1)) & ~(align - 1);
        region.size = size;
        if (next)
        *next = region.origin + region.size;
        return buffer.createSubBuffer(flags, buffer_create_type, &region, err);
    }
}

std::string stringFromFile(const char* filename) {
    std::ifstream ifs;
    ifs.open(filename);
    ifs.clear();
    ifs.seekg(0, std::ios::beg);
    return std::string{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
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
    size_t numFill = ((vec->size() + (alignment - 1)) & ~(alignment - 1)) - vec->size();
    vec->insert(vec->end(), numFill, 0);
    return vec->size();
}

uint64_t addDataAligned(std::vector<uint8_t>* dest, void* data, size_t bytes, uint32_t alignment) {
    size_t numFill = ((dest->size() + (alignment - 1)) & ~(alignment - 1)) - dest->size();
    dest->insert(dest->end(), numFill + bytes, 0);
    memcpy(&dest->back() + 1 - bytes, data, bytes);
    return dest->size() - bytes;
}

uint64_t fillZerosAligned(std::vector<uint8_t>* dest, uint32_t bytes, uint32_t alignment) {
    size_t numFill = ((dest->size() + (alignment - 1)) & ~(alignment - 1)) - dest->size();
    dest->insert(dest->end(), numFill + bytes, 0);
    return dest->size() - bytes;
}
