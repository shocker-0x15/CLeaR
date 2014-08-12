//
//  LinearAlgebra.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/08/22.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "LinearAlgebra.h"
#include <math.h>
#include <stdio.h>

namespace LinearAlgebra {
    Matrix4f LookAt(float ex, float ey, float ez, float tx, float ty, float tz, float ux, float uy, float uz) {
        Matrix4f matrix;
        tx = ex - tx;
        ty = ey - ty;
        tz = ez - tz;
        float l = sqrtf(tx * tx + ty * ty + tz * tz);
        matrix.m20 = tx / l;
        matrix.m21 = ty / l;
        matrix.m22 = tz / l;
        
        tx = uy * matrix.m22 - uz * matrix.m21;
        ty = uz * matrix.m20 - ux * matrix.m22;
        tz = ux * matrix.m21 - uy * matrix.m20;
        
        l = sqrtf(tx * tx + ty * ty + tz * tz);
        matrix.m00 = tx / l;
        matrix.m01 = ty / l;
        matrix.m02 = tz / l;
        
        matrix.m10 = matrix.m21 * matrix.m02 - matrix.m22 * matrix.m01;
        matrix.m11 = matrix.m22 * matrix.m00 - matrix.m20 * matrix.m02;
        matrix.m12 = matrix.m20 * matrix.m01 - matrix.m21 * matrix.m00;
        
        matrix.m03 = -(ex * matrix.m00 + ey * matrix.m01 + ez * matrix.m02);
        matrix.m13 = -(ex * matrix.m10 + ey * matrix.m11 + ez * matrix.m12);
        matrix.m23 = -(ex * matrix.m20 + ey * matrix.m21 + ez * matrix.m22);
        
        matrix.m30 = matrix.m31 = matrix.m32 = 0.0f;
        matrix.m33 = 1.0f;
        
        return matrix;
    }

    Matrix4f ScaleMatrix(float sx, float sy, float sz) {
        Matrix4f matrix;
        matrix.m00 = sx;
        matrix.m11 = sy;
        matrix.m22 = sz;
        matrix.m33 = 1.0f;
        matrix.m10 = matrix.m20 = matrix.m30 = matrix.m01 = 
        matrix.m21 = matrix.m31 = matrix.m02 = matrix.m12 = 
        matrix.m32 = matrix.m03 = matrix.m13 = matrix.m23 = 0.0f;
        
        return matrix;
    }

    Matrix4f TranslateMatrix(float tx, float ty, float tz) {
        Matrix4f matrix;
        matrix.m00 = matrix.m11 = matrix.m22 = matrix.m33 = 1.0f;
        matrix.m10 = matrix.m20 = matrix.m30 = matrix.m01 = 
        matrix.m21 = matrix.m31 = matrix.m02 = matrix.m12 = 
        matrix.m32 = 0.0f;
        matrix.m03 = tx;
        matrix.m13 = ty;
        matrix.m23 = tz;
        
        return matrix;
    }

    Matrix4f RotateMatrix(float angle, float rx, float ry, float rz) {
        Matrix4f matrix;
        float length = sqrtf(rx * rx + ry * ry + rz * rz);
        rx /= length;
        ry /= length;
        rz /= length;
        float c = cosf(angle);
        float s = sinf(angle);
        float oneMinusC = 1 - c;
        
        matrix.m00 = rx * rx * oneMinusC + c;
        matrix.m10 = rx * ry * oneMinusC + rz * s;
        matrix.m20 = rz * rx * oneMinusC - ry * s;
        matrix.m01 = rx * ry * oneMinusC - rz * s;
        matrix.m11 = ry * ry * oneMinusC + c;
        matrix.m21 = ry * rz * oneMinusC + rx * s;
        matrix.m02 = rz * rx * oneMinusC + ry * s;
        matrix.m12 = ry * rz * oneMinusC - rx * s;
        matrix.m22 = rz * rz * oneMinusC + c;
        
        matrix.m30 = matrix.m31 = matrix.m32 =
        matrix.m03 = matrix.m13 = matrix.m23 = 0.0f;
        matrix.m33 = 1.0f;
        
        return matrix;
    }
    
    Matrix4f CameraMatrix(float aspect, float fovY, float near, float far) {
        Matrix4f matrix;
        float f = 1.0f / tanf(fovY / 2);
        float dz = far - near;
        
        matrix.m00 = f / aspect;
        matrix.m11 = f;
        matrix.m22 = -(near + far) / dz;
        matrix.m32 = -1.0f;
        matrix.m23 = -2.0f * far * near / dz;
        matrix.m10 = matrix.m20 = matrix.m30 =
        matrix.m01 = matrix.m21 = matrix.m31 =
        matrix.m02 = matrix.m12 =
        matrix.m03 = matrix.m13 = matrix.m33 = 0.0f;
        
        return matrix;
    }
    
    void PrintMatrix(const Matrix4f &matrix) {
        printf("%10.3f, %10.3f, %10.3f, %10.3f\n", matrix.m00, matrix.m01, matrix.m02, matrix.m03);
        printf("%10.3f, %10.3f, %10.3f, %10.3f\n", matrix.m10, matrix.m11, matrix.m12, matrix.m13);
        printf("%10.3f, %10.3f, %10.3f, %10.3f\n", matrix.m20, matrix.m21, matrix.m22, matrix.m23);
        printf("%10.3f, %10.3f, %10.3f, %10.3f\n", matrix.m30, matrix.m31, matrix.m32, matrix.m33);
        printf("\n");
    }
    
    void IdentityQuaternion(float* quaternion) {
        quaternion[0] = 1.0f;
        quaternion[1] = 0.0f;
        quaternion[2] = 0.0f;
        quaternion[3] = 0.0f;
    }
    
    void CopyQuaternion(const float* q0, float* quaternion) {
        quaternion[0] = q0[0];
        quaternion[1] = q0[1];
        quaternion[2] = q0[2];
        quaternion[3] = q0[3];
    }
    
    void MultiplyQuaternion(const float* q0, const float* q1, float* quaternion) {
        quaternion[0] = q0[0] * q1[0] - q0[1] * q1[1] - q0[2] * q1[2] - q0[3] * q1[3];
        quaternion[1] = q0[0] * q1[1] + q0[1] * q1[0] + q0[2] * q1[3] - q0[3] * q1[2];
        quaternion[2] = q0[0] * q1[2] - q0[1] * q1[3] + q0[2] * q1[0] + q0[3] * q1[1];
        quaternion[3] = q0[0] * q1[3] + q0[1] * q1[2] - q0[2] * q1[1] + q0[3] * q1[0];
    }
    
    Matrix4f QuaternionToRotMat(const float* q0) {
        Matrix4f matrix;
        float x2 = q0[1] * q0[1] * 2.0f;
        float y2 = q0[2] * q0[2] * 2.0f;
        float z2 = q0[3] * q0[3] * 2.0f;
        float xy = q0[1] * q0[2] * 2.0f;
        float yz = q0[2] * q0[3] * 2.0f;
        float zx = q0[3] * q0[1] * 2.0f;
        float xw = q0[1] * q0[0] * 2.0f;
        float yw = q0[2] * q0[0] * 2.0f;
        float zw = q0[3] * q0[0] * 2.0f;
        
        matrix.m00 = 1.0f - y2 - z2;
        matrix.m10 = xy + zw;
        matrix.m20 = zx - yw;
        matrix.m01 = xy - zw;
        matrix.m11 = 1.0f - z2 - x2;
        matrix.m21 = yz + xw;
        matrix.m02 = zx + yw;
        matrix.m12 = yz - xw;
        matrix.m22 = 1.0f - x2 - y2;
        matrix.m30 = matrix.m31 = matrix.m32 = 
        matrix.m03 = matrix.m13 = matrix.m23 = 0.0f;
        matrix.m33 = 1.0f;
        
        return matrix;
    }
}
