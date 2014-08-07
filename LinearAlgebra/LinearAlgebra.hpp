//
//  LinearAlgebra.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/08/22.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_LinearAlgebra_hpp
#define OpenCL_PathTracer_LinearAlgebra_hpp

#include "Matrix4f.hpp"

namespace LinearAlgebra {
    Matrix4f LookAt(float ex, float ey, float ez, float tx, float ty, float tz, float ux, float uy, float uz);
    Matrix4f ScaleMatrix(float sx, float sy, float sz);
    Matrix4f TranslateMatrix(float tx, float ty, float tz);
    Matrix4f RotateMatrix(float angle, float rx, float ry, float rz);
    Matrix4f CameraMatrix(float aspect, float fovY, float near, float far);
    
    void PrintMatrix(const Matrix4f &matrix);
    
    void IdentityQuaternion(float* quaternion);
    void CopyQuaternion(const float* q0, float* quaternion);
    void MultiplyQuaternion(const float* q0, const float* q1, float* quaternion);
    Matrix4f QuaternionToRotMat(const float* q0);
}

#endif
