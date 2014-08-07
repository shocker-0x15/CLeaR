//
//  Vector4f.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/08/22.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "Vector4f.hpp"

const Vector4f Vector4f::Zero = Vector4f(0, 0, 0, 0);
const Vector4f Vector4f::One = Vector4f(1, 1, 1, 1);
const Vector4f Vector4f::NaN = Vector4f(NAN, NAN, NAN, NAN);
const Vector4f Vector4f::Ex = Vector4f(1, 0, 0, 0);
const Vector4f Vector4f::Ey = Vector4f(0, 1, 0, 0);
const Vector4f Vector4f::Ez = Vector4f(0, 0, 1, 0);
const Vector4f Vector4f::Ew = Vector4f(0, 0, 0, 1);
