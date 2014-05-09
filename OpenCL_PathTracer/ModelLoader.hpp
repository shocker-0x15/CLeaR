//
//  ModelLoader.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_PathTracer__ModelLoader__
#define __OpenCL_PathTracer__ModelLoader__

#include <vector>
#include "cl12.hpp"
#include "Face.hpp"

bool loadModel(const char* fileName,
               std::vector<cl_float3>* vertices, std::vector<cl_float3>* normals, std::vector<cl_float2>* uvs, std::vector<Face>* faces);

#endif /* defined(__OpenCL_PathTracer__ModelLoader__) */
