//
//  Vector3f.hpp
//  GIRenderer
//
//  Created by 渡部 心 on 11/07/16.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef GIRenderer_Vector3f_hpp
#define GIRenderer_Vector3f_hpp

#include <cstdio>
#include <cmath>

class Vector3f {
public:
    float x, y, z;
    
    Vector3f() : x(0), y(0), z(0) { };
    Vector3f(float xx, float yy, float zz) : x(xx), y(yy), z(zz) { };
    
    Vector3f operator+(const Vector3f &vec) const {
        return Vector3f(this->x + vec.x, this->y + vec.y, this->z + vec.z);
    }
    
    Vector3f operator-(const Vector3f &vec) const {
        return Vector3f(this->x - vec.x, this->y - vec.y, this->z - vec.z);
    };
    
    Vector3f operator*(float s) const {
        return Vector3f(this->x * s, this->y * s, this->z * s);
    };
    
    Vector3f operator*(const Vector3f &vec) const {
        return Vector3f(this->x * vec.x, this->y * vec.y, this->z * vec.z);
    };
    
    friend inline Vector3f operator*(float s, const Vector3f &vec) {
        return Vector3f(s * vec.x, s * vec.y, s * vec.z);
    };
    
    Vector3f operator/(float s) const {
        return Vector3f(this->x / s, this->y / s, this->z / s);
    };
    
    Vector3f operator/(const Vector3f &vec) const {
        return Vector3f(this->x / vec.x, this->y / vec.y, this->z / vec.z);
    };
    
    Vector3f &operator+=(const Vector3f &vec) {
        this->x += vec.x;
        this->y += vec.y;
        this->z += vec.z;
        return *this;
    };
    
    Vector3f &operator-=(const Vector3f &vec) {
        this->x -= vec.x;
        this->y -= vec.y;
        this->z -= vec.z;
        return *this;        
    };
    
    Vector3f &operator*=(float s) {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return *this;
    };
    
    Vector3f &operator*=(const Vector3f &vec) {
        this->x *= vec.x;
        this->y *= vec.y;
        this->z *= vec.z;
        return *this;
    };
    
    Vector3f &operator/=(float s) {
        this->x /= s;
        this->y /= s;
        this->z /= s;
        return *this;
    };
    
    Vector3f &operator/=(const Vector3f &vec) {
        this->x /= vec.x;
        this->y /= vec.y;
        this->z /= vec.z;
        return *this;
    };
    
    Vector3f operator+() const {
        return *this;
    };
    
    Vector3f operator-() const {
        return Vector3f(-this->x, -this->y, -this->z);
    };
    
    bool operator==(const Vector3f &vec) const {
        return this->x == vec.x && this->y == vec.y && this->z == vec.z;
    };
    
    bool operator!=(const Vector3f &vec) const {
        return this->x != vec.x || this->y != vec.y || this->z != vec.z;
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
    
    Vector3f reverse() const {
        return Vector3f(-this->x, -this->y, -this->z);
    };
    
    Vector3f reciprocal() const {
        return Vector3f(1.0 / this->x, 1.0 / this->y, 1.0 / this->z);
    }
    
    Vector3f normalize() const {
        float length = sqrtf(this->x * this->x + this->y * this->y + this->z * this->z);
        return Vector3f(this->x / length, this->y / length, this->z / length);
    };
    
    float length() const {
        return sqrtf(this->x * this->x + this->y * this->y + this->z * this->z);
    };
    
    float sqLength() const {
        return this->x * this->x + this->y * this->y + this->z * this->z;
    };
    
    float sum() const {
        return this->x + this->y + this->z;
    }
    
    float average() const {
        return (this->x + this->y + this->z) / 3;
    };
    
    float max() const {
        return fmaxf(this->x, fmaxf(this->y, this->z));
    }
    
    float min() const {
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
    
    static const Vector3f Zero;//----------ゼロベクトル。
    static const Vector3f One;//-----------全要素1のベクトル。
    static const Vector3f NaN;//-----------非数ベクトル。
    static const Vector3f Ex;//------------x方向単位ベクトル。
    static const Vector3f Ey;//------------y方向単位ベクトル。
    static const Vector3f Ez;//------------z方向単位ベクトル。
    
    static float dot(const Vector3f &vec1, const Vector3f &vec2);
    static Vector3f cross(const Vector3f &vec1, const Vector3f &vec2);
    static float absDot(const Vector3f &vec1, const Vector3f &vec2);
    static Vector3f min(const Vector3f &vec1, const Vector3f &vec2);
    static Vector3f max(const Vector3f &vec1, const Vector3f &vec2);
};

inline float Vector3f::dot(const Vector3f &vec1, const Vector3f &vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

inline Vector3f Vector3f::cross(const Vector3f &vec1, const Vector3f &vec2) {
    return Vector3f(vec1.y * vec2.z - vec1.z * vec2.y, 
                    vec1.z * vec2.x - vec1.x * vec2.z, 
                    vec1.x * vec2.y - vec1.y * vec2.x);
}

inline float Vector3f::absDot(const Vector3f &vec1, const Vector3f &vec2) {
    return fabsf(vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z);
}

inline Vector3f Vector3f::min(const Vector3f &vec1, const Vector3f &vec2) {
    return Vector3f(fminf(vec1.x, vec2.x), fminf(vec1.y, vec2.y), fminf(vec1.z, vec2.z));
}

inline Vector3f Vector3f::max(const Vector3f &vec1, const Vector3f &vec2) {
    return Vector3f(fmaxf(vec1.x, vec2.x), fmaxf(vec1.y, vec2.y), fmaxf(vec1.z, vec2.z));
}

#endif
