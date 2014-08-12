//
//  Matrix4f.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/08/22.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Matrix4f_hpp
#define OpenCL_PathTracer_Matrix4f_hpp

#include "Vector3f.hpp"
#include "Vector4f.hpp"
#include "Point3f.hpp"

class Matrix4f {
public:
    float m00, m10, m20, m30;
    float m01, m11, m21, m31;
    float m02, m12, m22, m32;
    float m03, m13, m23, m33;
    
    Matrix4f() : 
        m00(0), m10(0), m20(0), m30(0),
        m01(0), m11(0), m21(0), m31(0),
        m02(0), m12(0), m22(0), m32(0),
        m03(0), m13(0), m23(0), m33(0)
    { };
    
    Matrix4f(float* array) : 
        m00(array[ 0]), m10(array[ 1]), m20(array[ 2]), m30(array[ 3]),
        m01(array[ 4]), m11(array[ 5]), m21(array[ 6]), m31(array[ 7]),
        m02(array[ 8]), m12(array[ 9]), m22(array[10]), m32(array[11]),
        m03(array[12]), m13(array[13]), m23(array[14]), m33(array[15])
    { };
    
    Matrix4f(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) : 
        m00(v0.x), m10(v0.y), m20(v0.z), m30(0.0f),
        m01(v1.x), m11(v1.y), m21(v1.z), m31(0.0f),
        m02(v2.x), m12(v2.y), m22(v2.z), m32(0.0f),
        m03(0.0f), m13(0.0f), m23(0.0f), m33(1.0f)
    { };
    
    Matrix4f(const Vector4f &v0, const Vector4f &v1, const Vector4f &v2, const Vector4f &v3) :
        m00(v0.x), m10(v0.y), m20(v0.z), m30(v0.w),
        m01(v1.x), m11(v1.y), m21(v1.z), m31(v1.w),
        m02(v2.x), m12(v2.y), m22(v2.z), m32(v2.w),
        m03(v3.x), m13(v3.y), m23(v3.z), m33(v3.w)
    { };
    
    Vector4f Column(int c) const {
        switch (c) {
            case 0:
                return Vector4f(m00, m10, m20, m30);
            case 1:
                return Vector4f(m01, m11, m21, m31);
            case 2:
                return Vector4f(m02, m12, m22, m32);
            case 3:
                return Vector4f(m03, m13, m23, m33);
            default:
                return Vector4f::Zero;
        }
    };
    
    Vector4f Row(int r) const {
        switch (r) {
            case 0:
                return Vector4f(m00, m01, m02, m03);
            case 1:
                return Vector4f(m10, m11, m12, m13);
            case 2:
                return Vector4f(m20, m21, m22, m23);
            case 3:
                return Vector4f(m30, m31, m32, m33);
            default:
                return Vector4f::Zero;
        }
    };
    
    Matrix4f Invert() const {
        int ColumnIndex[] = {0, 0, 0, 0};
        int RowIndex[] = {0, 0, 0, 0};
        int PivotIndex[] = {-1, -1, -1, -1};
        
        float inverse[4][4];
        inverse[0][0] = m00; inverse[1][0] = m01; inverse[2][0] = m02; inverse[3][0] = m03;
        inverse[0][1] = m10; inverse[1][1] = m11; inverse[2][1] = m12; inverse[3][1] = m13;
        inverse[0][2] = m20; inverse[1][2] = m21; inverse[2][2] = m22; inverse[3][2] = m23;
        inverse[0][3] = m30; inverse[1][3] = m31; inverse[2][3] = m32; inverse[3][3] = m33;
        
        int iColumn = 0;
        int iRow = 0;
        for (int i = 0; i < 4; ++i) {
            float maxPivot = 0.0f;
            for (int j = 0; j < 4; ++j) {
                if (PivotIndex[j] != 0) {
                    for (int k = 0; k < 4; k++) {
                        if (PivotIndex[k] == -1) {
                            float absValue = fabsf(inverse[j][k]);
                            if (absValue > maxPivot) {
                                maxPivot = absValue;
                                iRow = j;
                                iColumn = k;
                            }
                        }
                        else if (PivotIndex[k] > 0) {
                            return *this;
                        }
                    }
                }
            }
            
            (PivotIndex[iColumn])++;
            
            if (iRow != iColumn) {
                for (int j = 0; j < 4; ++j) {
                    float f = inverse[iRow][j];
                    inverse[iRow][j] = inverse[iColumn][j];
                    inverse[iColumn][j] = f;
                }
            }
            
            RowIndex[i] = iRow;
            ColumnIndex[i] = iColumn;
            
            float Pivot = inverse[iColumn][iColumn];
            if (Pivot == 0.0f) {
                return Matrix4f::NaN;
            }
            
            float oneOverPivot = 1.0f / Pivot;
            inverse[iColumn][iColumn] = 1.0f;
            for (int j = 0; j < 4; ++j) {
                inverse[iColumn][j] *= oneOverPivot;
            }
            
            for (int j = 0; j < 4; ++j) {
                if (iColumn != j) {
                    float f = inverse[j][iColumn];
                    inverse[j][iColumn] = 0.0f;
                    for (int k = 0; k < 4; k++) {
                        inverse[j][k] -= inverse[iColumn][k] * f;
                    }
                }
            }
        }
        
        for (int i = 3; i > -1; i--) {
            int ir = RowIndex[i];
            int ic = ColumnIndex[i];
            for (int j = 0; j < 4; ++j) {
                float f = inverse[j][ir];
                inverse[j][ir] = inverse[j][ic];
                inverse[j][ic] = f;
            }
        }
        
        return Matrix4f(Vector4f(inverse[0][0], inverse[0][1], inverse[0][2], inverse[0][3]), 
                        Vector4f(inverse[1][0], inverse[1][1], inverse[1][2], inverse[1][3]), 
                        Vector4f(inverse[2][0], inverse[2][1], inverse[2][2], inverse[2][3]), 
                        Vector4f(inverse[3][0], inverse[3][1], inverse[3][2], inverse[3][3]));
    };
    
    Matrix4f Transpose() const {
        return Matrix4f(Vector4f(m00, m01, m02, m03), 
                        Vector4f(m10, m11, m12, m13), 
                        Vector4f(m20, m21, m22, m23), 
                        Vector4f(m30, m31, m32, m33));
    };
    
    Matrix4f operator+(const Matrix4f &mat) const {
        return Matrix4f(this->Column(0) + mat.Column(0), 
                        this->Column(1) + mat.Column(1), 
                        this->Column(2) + mat.Column(2), 
                        this->Column(3) + mat.Column(3));
    };
    
    Matrix4f operator-(const Matrix4f &mat) const {
        return Matrix4f(this->Column(0) - mat.Column(0), 
                        this->Column(1) - mat.Column(1), 
                        this->Column(2) - mat.Column(2), 
                        this->Column(3) - mat.Column(3));
    };
    
    Matrix4f operator*(float s) const {
        return Matrix4f(this->Column(0) * s, 
                        this->Column(1) * s, 
                        this->Column(2) * s, 
                        this->Column(3) * s);
    };
    
    Matrix4f operator*(const Matrix4f &mat) const {
        return Matrix4f(Vector4f(Vector4f::dot(this->Row(0), mat.Column(0)),
                                 Vector4f::dot(this->Row(1), mat.Column(0)),
                                 Vector4f::dot(this->Row(2), mat.Column(0)),
                                 Vector4f::dot(this->Row(3), mat.Column(0))),
                        
                        Vector4f(Vector4f::dot(this->Row(0), mat.Column(1)),
                                 Vector4f::dot(this->Row(1), mat.Column(1)),
                                 Vector4f::dot(this->Row(2), mat.Column(1)),
                                 Vector4f::dot(this->Row(3), mat.Column(1))),
                        
                        Vector4f(Vector4f::dot(this->Row(0), mat.Column(2)),
                                 Vector4f::dot(this->Row(1), mat.Column(2)),
                                 Vector4f::dot(this->Row(2), mat.Column(2)),
                                 Vector4f::dot(this->Row(3), mat.Column(2))),
                        
                        Vector4f(Vector4f::dot(this->Row(0), mat.Column(3)),
                                 Vector4f::dot(this->Row(1), mat.Column(3)),
                                 Vector4f::dot(this->Row(2), mat.Column(3)),
                                 Vector4f::dot(this->Row(3), mat.Column(3))));
    };
    
    friend inline Matrix4f operator*(float s, const Matrix4f &mat) {
        return Matrix4f(s * mat.Column(0), 
                        s * mat.Column(1), 
                        s * mat.Column(2), 
                        s * mat.Column(3));
    };
    
    Matrix4f operator/(float s) const {
        return Matrix4f(this->Column(0) / s, 
                        this->Column(1) / s, 
                        this->Column(2) / s, 
                        this->Column(3) / s);
    }
    
    //--------------------------------------------------------------------------
    //ベクトルとの演算．
    Vector3f operator*(const Vector3f &vec) const {
        return Vector3f(Vector3f::dot(Vector3f(this->m00, this->m01, this->m02), vec),
                        Vector3f::dot(Vector3f(this->m10, this->m11, this->m12), vec),
                        Vector3f::dot(Vector3f(this->m20, this->m21, this->m22), vec));
    };
    
    Vector4f operator*(const Vector4f &vec) const {
        return Vector4f(Vector4f::dot(this->Row(0), vec),
                        Vector4f::dot(this->Row(1), vec),
                        Vector4f::dot(this->Row(2), vec),
                        Vector4f::dot(this->Row(3), vec));
    };
    //--------------------------------------------------------------------------
    
    //--------------------------------------------------------------------------
    //点との演算．
    Point3f operator*(const Point3f &p) const {
        float x = p.x, y = p.y, z = p.z;
        float nx = m00 * x + m01 * y + m02 * z + m03;
        float ny = m10 * x + m11 * y + m12 * z + m13;
        float nz = m20 * x + m21 * y + m22 * z + m23;
        float nw = m30 * x + m31 * y + m32 * z + m33;
        
        if (nw != 1.0f) return Point3f(nx, ny, nz) / nw;
        else return Point3f(nx, ny, nz);
    };
    //--------------------------------------------------------------------------
    
    Matrix4f &operator+=(const Matrix4f &mat) {
        m00 += mat.m00; m01 += mat.m01; m02 += mat.m02; m03 += mat.m03;
        m10 += mat.m10; m11 += mat.m11; m12 += mat.m12; m13 += mat.m13;
        m20 += mat.m10; m21 += mat.m21; m22 += mat.m22; m23 += mat.m23;
        m30 += mat.m10; m31 += mat.m31; m32 += mat.m32; m33 += mat.m33;
        return *this;
    };
    
    Matrix4f &operator-=(const Matrix4f &mat) {
        m00 -= mat.m00; m01 -= mat.m01; m02 -= mat.m02; m03 -= mat.m03;
        m10 -= mat.m10; m11 -= mat.m11; m12 -= mat.m12; m13 -= mat.m13;
        m20 -= mat.m10; m21 -= mat.m21; m22 -= mat.m22; m23 -= mat.m23;
        m30 -= mat.m10; m31 -= mat.m31; m32 -= mat.m32; m33 -= mat.m33;
        return *this; 
    };
    
    Matrix4f &operator*=(float s) {
        m00 *= s; m01 *= s; m02 *= s; m03 *= s;
        m10 *= s; m11 *= s; m12 *= s; m13 *= s;
        m20 *= s; m21 *= s; m22 *= s; m23 *= s;
        m30 *= s; m31 *= s; m32 *= s; m33 *= s;
        return *this;
    };
    
    Matrix4f &operator*=(const Matrix4f &mat) {
        Vector4f rows[4] = {this->Row(0), this->Row(1), this->Row(2), this->Row(3)};
        
        m00 = Vector4f::dot(rows[0], mat.Column(0));
        m10 = Vector4f::dot(rows[1], mat.Column(0));
        m20 = Vector4f::dot(rows[2], mat.Column(0));
        m30 = Vector4f::dot(rows[3], mat.Column(0));
        
        m01 = Vector4f::dot(rows[0], mat.Column(1));
        m11 = Vector4f::dot(rows[1], mat.Column(1));
        m21 = Vector4f::dot(rows[2], mat.Column(1));
        m31 = Vector4f::dot(rows[3], mat.Column(1));
        
        m02 = Vector4f::dot(rows[0], mat.Column(2));
        m12 = Vector4f::dot(rows[1], mat.Column(2));
        m22 = Vector4f::dot(rows[2], mat.Column(2));
        m32 = Vector4f::dot(rows[3], mat.Column(2));
        
        m03 = Vector4f::dot(rows[0], mat.Column(3));
        m13 = Vector4f::dot(rows[1], mat.Column(3));
        m23 = Vector4f::dot(rows[2], mat.Column(3));
        m33 = Vector4f::dot(rows[3], mat.Column(3));
        
        return *this;
    };
    
    Matrix4f &operator/=(float s) {
        m00 /= s; m01 /= s; m02 /= s; m03 /= s;
        m10 /= s; m11 /= s; m12 /= s; m13 /= s;
        m20 /= s; m21 /= s; m22 /= s; m23 /= s;
        m30 /= s; m31 /= s; m32 /= s; m33 /= s;
        return *this;  
    };
    
    Matrix4f operator+() const {
        return *this;
    };
    
    Matrix4f operator-() const {
        return Matrix4f(-this->Column(0), -this->Column(1), -this->Column(2), -this->Column(3));
    };
    
    bool operator==(const Matrix4f &mat) const {
        return 
        m00 == mat.m00 && m01 == mat.m01 && m02 == mat.m02 && m03 == mat.m03 && 
        m10 == mat.m10 && m11 == mat.m11 && m12 == mat.m12 && m13 == mat.m13 && 
        m20 == mat.m20 && m21 == mat.m21 && m22 == mat.m22 && m23 == mat.m23 && 
        m30 == mat.m30 && m31 == mat.m31 && m32 == mat.m32 && m33 == mat.m33;
    };
    
    bool operator!=(const Matrix4f &mat) const {
        return 
        m00 != mat.m00 || m01 != mat.m01 || m02 != mat.m02 || m03 != mat.m03 || 
        m10 != mat.m10 || m11 != mat.m11 || m12 != mat.m12 || m13 != mat.m13 || 
        m20 != mat.m20 || m21 != mat.m21 || m22 != mat.m22 || m23 != mat.m23 || 
        m30 != mat.m30 || m31 != mat.m31 || m32 != mat.m32 || m33 != mat.m33;
    };
    
    bool HasNaN() const {
        using std::isnan;
        return 
        isnan(m00) || isnan(m01) || isnan(m02) || isnan(m03) || 
        isnan(m10) || isnan(m11) || isnan(m12) || isnan(m13) || 
        isnan(m20) || isnan(m21) || isnan(m22) || isnan(m23) || 
        isnan(m30) || isnan(m31) || isnan(m32) || isnan(m33);
    };
    
    void getRawData(float* mat) const {
        mat[ 0] = m00; mat[ 4] = m10; mat[ 8] = m20; mat[12] = m30;
        mat[ 1] = m01; mat[ 5] = m11; mat[ 9] = m21; mat[13] = m31;
        mat[ 2] = m02; mat[ 6] = m12; mat[10] = m22; mat[14] = m32;
        mat[ 3] = m03; mat[ 7] = m13; mat[11] = m23; mat[15] = m33;
    }
    
    static const Matrix4f Identity;
    static const Matrix4f Zero;
    static const Matrix4f One;
    static const Matrix4f NaN;
};

#endif
