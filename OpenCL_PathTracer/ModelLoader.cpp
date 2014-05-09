//
//  ModelLoader.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "ModelLoader.hpp"
#include <string>

bool loadObj(const char* fileName, std::vector<cl_float3>* vertices, std::vector<cl_float3>* normals, std::vector<cl_float2>* uvs) {
    return true;
}

bool loadModel(const char* fileName, std::vector<cl_float3>* vertices, std::vector<cl_float3>* normals, std::vector<cl_float2>* uvs) {
    std::string sFileName = fileName;
    
    size_t extPos = sFileName.find_first_of(".");
    if (extPos == std::string::npos)
        return false;
    
    std::string sExt = sFileName.substr(extPos + 1);
    if (!sExt.compare("obj")) {
        
    }
    
    return false;
}
