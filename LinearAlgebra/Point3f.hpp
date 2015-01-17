//
//  Point3f.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 12/08/29.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Point3f_hpp
#define OpenCL_PathTracer_Point3f_hpp

#include "Vector3f.hpp"
//#include <stdio.h>
//#include <math.h>

class Point3f {
public:
    float x, y, z;
    
    Point3f() : x(0), y(0), z(0) { };
    constexpr Point3f(float xx, float yy, float zz) : x(xx), y(yy), z(zz) { };
    
    Point3f operator+(const Point3f &p) const {
        return Point3f(this->x + p.x, this->y + p.y, this->z + p.z);
    }
    
    Vector3f operator-(const Point3f &p) const {
        return Vector3f(this->x - p.x, this->y - p.y, this->z - p.z);
    };
    
    Point3f operator*(float s) const {
        return Point3f(this->x * s, this->y * s, this->z * s);
    };
    
    Point3f operator*(const Point3f &p) const {
        return Point3f(this->x * p.x, this->y * p.y, this->z * p.z);
    };
    
    friend inline Point3f operator*(float s, const Point3f &p) {
        return Point3f(s * p.x, s * p.y, s * p.z);
    };
    
    Point3f operator/(float s) const {
        return Point3f(this->x / s, this->y / s, this->z / s);
    };
    
    Point3f &operator+=(const Point3f &p) {
        this->x += p.x;
        this->y += p.y;
        this->z += p.z;
        return *this;
    };
    
    Point3f &operator*=(float s) {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return *this;
    };
    
    Point3f &operator/=(float s) {
        this->x /= s;
        this->y /= s;
        this->z /= s;
        return *this;
    };
    
    //---------------------------------------------------------------
    // ベクトルとの演算．
    Point3f operator+(const Vector3f &vec) const {
        return Point3f(this->x + vec.x, this->y + vec.y, this->z + vec.z);
    }
    
    friend inline Vector3f operator+(const Vector3f &vec, const Point3f &p) {
        return Vector3f(vec.x + p.x, vec.y + p.y, vec.z + p.z);
    };
    
    Point3f operator-(const Vector3f &vec) const {
        return Point3f(this->x - vec.x, this->y - vec.y, this->z - vec.z);
    };
    
    friend inline Vector3f operator-(const Vector3f &vec, const Point3f &p) {
        return Vector3f(vec.x - p.x, vec.y - p.y, vec.z - p.z);
    };
    
    Point3f &operator+=(const Vector3f &vec) {
        this->x += vec.x;
        this->y += vec.y;
        this->z += vec.z;
        return *this;
    };
    
    Point3f &operator-=(const Vector3f &vec) {
        this->x -= vec.x;
        this->y -= vec.y;
        this->z -= vec.z;
        return *this;
    };
    //---------------------------------------------------------------
    
    Point3f operator+() const {
        return *this;
    };
    
    Point3f operator-() const {
        return Point3f(-this->x, -this->y, -this->z);
    };
    
    bool operator==(const Point3f &p) const {
        return this->x == p.x && this->y == p.y && this->z == p.z;
    };
    
    bool operator!=(const Point3f &p) const {
        return this->x != p.x || this->y != p.y || this->z != p.z;
    };
    
    float &operator[](unsigned int index) {
        switch (index) {
            case 0:
                return this->x;
            case 1:
                return this->y;
            case 2:
                return this->z;
            default:
                return this->x;
        }
    }
    
    float operator[](unsigned int index) const {
        switch (index) {
            case 0:
                return this->x;
            case 1:
                return this->y;
            case 2:
                return this->z;
            default:
                return this->x;
        }
    }
    
    void print() const {
        printf("(%f, %f, %f)\n", this->x, this->y, this->z);
    };
    
    float maxValue() const {
        return fmaxf(this->x, fmaxf(this->y, this->z));
    }
    
    float minValue() const {
        return fminf(this->x, fminf(this->y, this->z));
    }
    
    bool hasNaN() {
        using std::isnan;
        return isnan(this->x) || isnan(this->y) || isnan(this->z);
    }
    
    bool hasInf() const {
        using std::isinf;
        return isinf(this->x) || isinf(this->y) || isinf(this->z);
    }
    
    static const Point3f Zero;//----------全要素0の点。
    static const Point3f One;//-----------全要素1の点。
    static const Point3f NaN;//-----------非数の点。
    static const Point3f Inf;//-----------全要素無限の点。
    
    static Point3f min(const Point3f &p1, const Point3f &p2);
    static Point3f max(const Point3f &p1, const Point3f &p2);
};

inline Point3f Point3f::min(const Point3f &p1, const Point3f &p2) {
    return Point3f(fminf(p1.x, p2.x), fminf(p1.y, p2.y), fminf(p1.z, p2.z));
}

inline Point3f Point3f::max(const Point3f &p1, const Point3f &p2) {
    return Point3f(fmaxf(p1.x, p2.x), fmaxf(p1.y, p2.y), fmaxf(p1.z, p2.z));
}

#endif /* defined(__GIRenderer__Point3f__) */
