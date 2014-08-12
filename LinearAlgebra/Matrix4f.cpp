//
//  Matrix4f.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/08/22.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "Matrix4f.hpp"

const Matrix4f Matrix4f::Identity = Matrix4f(Vector4f(1, 0, 0, 0),
                                             Vector4f(0, 1, 0, 0),
                                             Vector4f(0, 0, 1, 0),
                                             Vector4f(0, 0, 0, 1));
const Matrix4f Matrix4f::Zero = Matrix4f(Vector4f(0, 0, 0, 0),
                                         Vector4f(0, 0, 0, 0),
                                         Vector4f(0, 0, 0, 0),
                                         Vector4f(0, 0, 0, 0));
const Matrix4f Matrix4f::One = Matrix4f(Vector4f(1, 1, 1, 1),
                                        Vector4f(1, 1, 1, 1),
                                        Vector4f(1, 1, 1, 1),
                                        Vector4f(1, 1, 1, 1));
const Matrix4f Matrix4f::NaN = Matrix4f(Vector4f(NAN, NAN, NAN, NAN),
                                        Vector4f(NAN, NAN, NAN, NAN),
                                        Vector4f(NAN, NAN, NAN, NAN),
                                        Vector4f(NAN, NAN, NAN, NAN));
