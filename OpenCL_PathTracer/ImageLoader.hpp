//
//  ImageLoader.h
//  OpenCL_TEST
//
//  Created by 渡部 心 on 2014/05/05.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_TEST__ImageLoader__
#define __OpenCL_TEST__ImageLoader__

#include <vector>
#include <stdint.h>

bool loadImage(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height, bool gammaCorrection);
bool loadEnvMap(const char* fileName, std::vector<uint8_t>* storage);

#endif /* defined(__OpenCL_TEST__ImageLoader__) */
