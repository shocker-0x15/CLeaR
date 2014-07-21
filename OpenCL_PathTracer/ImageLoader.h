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
#include <cstdint>

namespace ColorChannel {
    enum Value {
        RGB8x3 = 0,
        RGBA8x4,
        RGBA16Fx4,
        Gray8,
    };
}

bool loadImage(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height, ColorChannel::Value* color, bool gammaCorrection);

#endif /* defined(__OpenCL_TEST__ImageLoader__) */
