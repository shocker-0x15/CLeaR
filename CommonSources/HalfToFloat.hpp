//
//  HalfToFloat.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/08/29.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_HalfToFloat_hpp
#define OpenCL_PathTracer_HalfToFloat_hpp

#include <half.h>

const half::uif _toFloat[1 << 16] =
#include "toFloat.h"

// Winで何故かhalf=>floatのLUTがうまくライブラリに組み込めておらず変換に失敗するため、自作の変換関数の定義。
inline float halfToFloat(const half &_h) {
    return _toFloat[_h.bits()].f;
}

#endif
