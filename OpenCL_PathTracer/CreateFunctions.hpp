//
//  CreateFunctions.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_CreateFunctions_hpp
#define OpenCL_PathTracer_CreateFunctions_hpp

#include "clUtility.hpp"
#include "ImageLoader.hpp"

uint64_t createFloat3ConstantTexture(std::vector<uint8_t>* storage, float s0, float s1, float s2) {
    cl_float3 f3Val;
    uint64_t texHead = addDataAligned<cl_uchar>(storage, 0);
    f3Val.s0 = s0; f3Val.s1 = s1; f3Val.s2 = s2;
    addDataAligned<cl_float3>(storage, f3Val);
    return texHead;
}

uint64_t createFloatConstantTexture(std::vector<uint8_t>* storage, float val) {
    uint64_t texHead = addDataAligned<cl_uchar>(storage, 10);
    addDataAligned<cl_float>(storage, val);
    return texHead;
}

uint64_t createImageTexture(std::vector<uint8_t>* storage, const char* filename) {
    uint32_t w, h;
    uint64_t texHead = addDataAligned<cl_uchar>(storage, 1);
    addDataAligned<cl_uint>(storage, 0);
    addDataAligned<cl_uint>(storage, 0);
    align(storage, sizeof(cl_uchar3));
    loadImage(filename, storage, &w, &h);
    *(cl_uint*)(storage->data() + texHead + sizeof(cl_uint) * 1) = w;
    *(cl_uint*)(storage->data() + texHead + sizeof(cl_uint) * 2) = h;
    return texHead;
}

uint64_t createCheckerBoardTexture(std::vector<uint8_t>* storage, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b) {
    uint64_t texHead = addDataAligned<cl_uchar>(storage, 2);
    addDataAligned<cl_uchar>(storage, 0);
    cl_float3 f3Val;
    f3Val.s0 = c0r; f3Val.s1 = c0g; f3Val.s2 = c0b;
    addDataAligned<cl_float3>(storage, f3Val);
    f3Val.s0 = c1r; f3Val.s1 = c1g; f3Val.s2 = c1b;
    addDataAligned<cl_float3>(storage, f3Val);
    return texHead;
}

#endif
