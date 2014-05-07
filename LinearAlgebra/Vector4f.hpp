//
//  Vector4f.hpp
//  GIRenderer
//
//  Created by 渡部 心 on 11/08/22.
//  Copyright (c) 2011年 __MyCompanyName__. All rights reserved.
//

#ifndef GIRenderer_Vector4f_hpp
#define GIRenderer_Vector4f_hpp

#include <cstdio>
#include <cmath>
#include "Vector3f.hpp"

class Vector4f {
public:
    float x, y, z, w;
    
    Vector4f() : x(0), y(0), z(0), w(0) { };
    Vector4f(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) { };
    Vector4f(const Vector3f &vec3) : x(vec3.x), y(vec3.y), z(vec3.z) { };
    Vector4f(const Vector3f &vec3, float ww) : x(vec3.x), y(vec3.y), z(vec3.z), w(ww) { };
    
    Vector4f operator+(const Vector4f &vec) const {
        return Vector4f(this->x + vec.x, this->y + vec.y, this->z + vec.z, this->w + vec.w);
    }
    
    Vector4f operator-(const Vector4f &vec) const {
        return Vector4f(this->x - vec.x, this->y - vec.y, this->z - vec.z, this->w - vec.w);
    };
    
    Vector4f operator*(float s) const {
        return Vector4f(this->x * s, this->y * s, this->z * s, this-> w * s);
    };
    
    Vector4f operator*(const Vector4f &vec) const {
        return Vector4f(this->x * vec.x, this->y * vec.y, this->z * vec.z, this->w * vec.w);
    };
    
    friend inline Vector4f operator*(float s, const Vector4f &vec) {
        return Vector4f(s * vec.x, s * vec.y, s * vec.z, s * vec.w);
    };
    
    Vector4f operator/(float s) const {
        return Vector4f(this->x / s, this->y / s, this->z / s, this->w / s);
    };
    
    Vector4f operator/(const Vector4f &vec) const {
        return Vector4f(this->x / vec.x, this->y / vec.y, this->z / vec.z, this->w / vec.w);
    };
    
    Vector4f &operator+=(const Vector4f &vec) {
        this->x += vec.x;
        this->y += vec.y;
        this->z += vec.z;
        this->w += vec.w;
        return *this;
    };
    
    Vector4f &operator-=(const Vector4f &vec) {
        this->x -= vec.x;
        this->y -= vec.y;
        this->z -= vec.z;
        this->w -= vec.w;
        return *this;        
    };
    
    Vector4f &operator*=(float s) {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        this->w *= s;
        return *this;
    };
    
    Vector4f &operator*=(const Vector4f &vec) {
        this->x *= vec.x;
        this->y *= vec.y;
        this->z *= vec.z;
        this->w *= vec.w;
        return *this;
    };
    
    Vector4f &operator/=(float s) {
        this->x /= s;
        this->y /= s;
        this->z /= s;
        this->w /= s;
        return *this;
    };
    
    Vector4f &operator/=(const Vector4f &vec) {
        this->x /= vec.x;
        this->y /= vec.y;
        this->z /= vec.z;
        this->w /= vec.w;
        return *this;
    };
    
    Vector4f operator+() const {
        return *this;
    };
    
    Vector4f operator-() const {
        return Vector4f(-this->x, -this->y, -this->z, -this->w);
    };
    
    bool operator==(const Vector4f &vec) const {
        return this->x == vec.x && this->y == vec.y && this->z == vec.z && this->w == vec.w;
    };
    
    bool operator!=(const Vector4f &vec) const {
        return this->x != vec.x || this->y != vec.y || this->z != vec.z || this->w != vec.w;
    };
    
    float &operator[](unsigned int index) {
        switch (index) {
            case 0:
                return this->x;
            case 1:
                return this->y;
            case 2:
                return this->z;
            case 3:
                return this->w;
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
            case 3:
                return this->w;
            default:
                return this->x;
        }
    }
    
    Vector3f vec3() const {
        return Vector3f(this->x, this->y, this->z);
    }
    
    Vector4f reverse() const {
        return Vector4f(-this->x, -this->y, -this->z, -this->w);
    };
    
    Vector4f reciprocal() const {
        return Vector4f(1.0 / this->x, 1.0 / this->y, 1.0 / this->z, 1.0 / this->w);
    }
    
    Vector4f normalize() const {
        float length = sqrtf(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
        return Vector4f(this->x / length, this->y / length, this->z / length, this->w / length);
    };
    
    float length() const {
        return sqrtf(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
    };
    
    float sqLength() const {
        return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
    };
    
    float sum() const {
        return this->x + this->y + this->z + this->w;
    }
    
    float average() const {
        return (this->x + this->y + this->z + this->w) / 4;
    };
    
    float maxValue() const {
        return fmaxf(this->x, fmaxf(this->y, fmaxf(this->z, this->w)));
    }
    
    float minValue() const {
        return fminf(this->x, fminf(this->y, fminf(this->z, this->w)));
    }
    
    bool hasNaN() const {
        using std::isnan;
        return isnan(this->x) || isnan(this->y) || isnan(this->z) || isnan(this->w);
    }
    
    bool hasInf() const {
        using std::isinf;
        return isinf(this->x) || isinf(this->y) || isinf(this->z) || isinf(this->w);
    }
    
    static const Vector4f Zero;//----------ゼロベクトル。
    static const Vector4f One;//-----------全要素1のベクトル。
    static const Vector4f NaN;//-----------非数ベクトル。
    static const Vector4f Ex;//------------x方向単位ベクトル。
    static const Vector4f Ey;//------------y方向単位ベクトル。
    static const Vector4f Ez;//------------z方向単位ベクトル。
    static const Vector4f Ew;//------------w方向単位ベクトル。
    
    static float dot(const Vector4f &vec1, const Vector4f &vec2);
    static Vector4f min(const Vector4f &vec1, const Vector4f &vec2);
    static Vector4f max(const Vector4f &vec1, const Vector4f &vec2);
};

inline float Vector4f::dot(const Vector4f &vec1, const Vector4f &vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z + vec1.w * vec2.w;
}

inline Vector4f Vector4f::min(const Vector4f &vec1, const Vector4f &vec2) {
    return Vector4f(fminf(vec1.x, vec2.x), fminf(vec1.y, vec2.y), fminf(vec1.z, vec2.z), fminf(vec1.w, vec2.w));
}

inline Vector4f Vector4f::max(const Vector4f &vec1, const Vector4f &vec2) {
    return Vector4f(fmaxf(vec1.x, vec2.x), fmaxf(vec1.y, vec2.y), fmaxf(vec1.z, vec2.z), fmaxf(vec1.w, vec2.w));
}

#endif
