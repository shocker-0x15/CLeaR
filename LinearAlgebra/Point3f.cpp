//
//  Point3f.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 12/08/29.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "Point3f.hpp"

const Point3f Point3f::Zero = Point3f(0, 0, 0);
const Point3f Point3f::One = Point3f(1, 1, 1);
const Point3f Point3f::NaN = Point3f(NAN, NAN, NAN);
const Point3f Point3f::Inf = Point3f(INFINITY, INFINITY, INFINITY);