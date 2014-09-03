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
#include "Scene.h"

bool loadModel(const char* fileName, Scene* scene, uint64_t matOverride = -1);

#endif /* defined(__OpenCL_PathTracer__ModelLoader__) */
