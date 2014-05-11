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
#include "Scene.hpp"

void createFloat3ConstantTexture(Scene* scene, const char* name, float s0, float s1, float s2);

void createFloatConstantTexture(Scene* scene, const char* name, float val);

void createImageTexture(Scene* scene, const char* name, const char* filename);

void createCheckerBoardTexture(Scene* scene, const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b);

void createDiffuseMaterial(Scene* scene, const char* name, size_t reflectanceIdx, size_t sigmaIdx);

void createWardMaterial(Scene* scene, const char* name, size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx);

void createDiffuseLightProperty(Scene* scene, const char* name, size_t emittanceIdx);

#endif
