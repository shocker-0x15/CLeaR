//
//  Vector3f.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/07/16.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "Vector3f.hpp"

const Vector3f Vector3f::Zero = Vector3f(0, 0, 0);
const Vector3f Vector3f::One = Vector3f(1, 1, 1);
const Vector3f Vector3f::NaN = Vector3f(NAN, NAN, NAN);
const Vector3f Vector3f::Ex = Vector3f(1, 0, 0);
const Vector3f Vector3f::Ey = Vector3f(0, 1, 0);
const Vector3f Vector3f::Ez = Vector3f(0, 0, 1);
