//
//  sim_base.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/11.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_sim_base_h
#define OpenCL_PathTracer_sim_base_h

#include <stdint.h>
#include <cmath>

namespace sim {
#define M_PI_F ((float)M_PI)
    
    typedef uint8_t uchar;
    typedef uint16_t ushort;
    typedef uint32_t uint;
    typedef uint64_t ulong;
    typedef uint64_t uintptr_t;
    
    struct uchar3;
    struct float2;
    struct float3;
    
    uint global_ids[2];
    uint global_sizes[2];
    uint global_offsets[2];
    uint get_global_id(uint dim) {
        return global_ids[dim];
    }
    uint get_global_size(uint dim) {
        return global_sizes[dim];
    }
    uint get_global_offset(uint dim) {
        return global_offsets[dim];
    }
    
    struct uchar3 {
        union {
            uchar x;
            uchar s0;
        };
        union {
            uchar y;
            uchar s1;
        };
        union {
            uchar z;
            uchar s2;
        };
        uchar dum;
        
        uchar3() : x(0), y(0), z(0) { };
        uchar3(uchar xx, uchar yy, uchar zz) : x(xx), y(yy), z(zz) { };
        uchar3(const uchar3 &v) : x(v.x), y(v.y), z(v.z) { };
        
        uchar3 operator+(const uchar3 &r) const {
            uchar3 ret{x, y, z};
            ret.x += r.x;
            ret.y += r.y;
            ret.z += r.z;
            return ret;
        }
        uchar3 &operator+=(const uchar3 &r) {
            this->x += r.x;
            this->y += r.y;
            this->z += r.z;
            return *this;
        }
        uchar3 operator-(const uchar3 &r) const {
            uchar3 ret{x, y, z};
            ret.x -= r.x;
            ret.y -= r.y;
            ret.z -= r.z;
            return ret;
        }
        uchar3 &operator-=(const uchar3 &r) {
            this->x -= r.x;
            this->y -= r.y;
            this->z -= r.z;
            return *this;
        }
        uchar3 operator*(const uchar3 &r) const {
            uchar3 ret{x, y, z};
            ret.x *= r.x;
            ret.y *= r.y;
            ret.z *= r.z;
            return ret;
        }
        uchar3 &operator*=(const uchar3 &r) {
            this->x *= r.x;
            this->y *= r.y;
            this->z *= r.z;
            return *this;
        }
        uchar3 operator*(uchar s) const {
            uchar3 ret{x, y, z};
            ret.x *= s;
            ret.y *= s;
            ret.z *= s;
            return ret;
        }
        uchar3 &operator*=(uchar s) {
            this->x *= s;
            this->y *= s;
            this->z *= s;
            return *this;
        }
        friend uchar3 operator*(uchar s, const uchar3 &r) {
            uchar3 ret(s * r.x, s * r.y, s * r.z);
            return ret;
        }
        uchar3 operator/(const uchar3 &r) const {
            uchar3 ret{x, y, z};
            ret.x /= r.x;
            ret.y /= r.y;
            ret.z /= r.z;
            return ret;
        }
        uchar3 &operator/=(const uchar3 &r) {
            this->x /= r.x;
            this->y /= r.y;
            this->z /= r.z;
            return *this;
        }
        uchar3 operator/(uchar s) const {
            uchar3 ret{x, y, z};
            float rec = 1.0f / s;
            ret.x *= rec;
            ret.y *= rec;
            ret.z *= rec;
            return ret;
        }
        uchar3 &operator/=(uchar s) {
            float rec = 1.0f / s;
            this->x *= rec;
            this->y *= rec;
            this->z *= rec;
            return *this;
        }
        uchar3 operator+() const {
            return *this;
        }
        uchar3 operator-() const {
            return uchar3(-x, -y, -z);
        }
    };
    
    struct float2 {
        union {
            float x;
            float s0;
        };
        union {
            float y;
            float s1;
        };
        
        float2() : x(0), y(0) { };
        float2(float xx, float yy) : x(xx), y(yy) { };
        float2(const float2 &v) : x(v.x), y(v.y) { };
        
        float2 operator+(const float2 &r) const {
            float2 ret{x, y};
            ret.x += r.x;
            ret.y += r.y;
            return ret;
        }
        float2 &operator+=(const float2 &r) {
            this->x += r.x;
            this->y += r.y;
            return *this;
        }
        float2 operator-(const float2 &r) const {
            float2 ret{x, y};
            ret.x -= r.x;
            ret.y -= r.y;
            return ret;
        }
        float2 &operator-=(const float2 &r) {
            this->x -= r.x;
            this->y -= r.y;
            return *this;
        }
        float2 operator*(const float2 &r) const {
            float2 ret{x, y};
            ret.x *= r.x;
            ret.y *= r.y;
            return ret;
        }
        float2 &operator*=(const float2 &r) {
            this->x *= r.x;
            this->y *= r.y;
            return *this;
        }
        float2 operator*(float s) const {
            float2 ret{x, y};
            ret.x *= s;
            ret.y *= s;
            return ret;
        }
        float2 &operator*=(float s) {
            this->x *= s;
            this->y *= s;
            return *this;
        }
        friend float2 operator*(float s, const float2 &r) {
            float2 ret{s * r.x, s * r.y};
            return ret;
        }
        float2 operator/(const float2 &r) const {
            float2 ret{x, y};
            ret.x /= r.x;
            ret.y /= r.y;
            return ret;
        }
        float2 &operator/=(const float2 &r) {
            this->x /= r.x;
            this->y /= r.y;
            return *this;
        }
        float2 operator/(float s) const {
            float2 ret{x, y};
            float rec = 1.0f / s;
            ret.x *= rec;
            ret.y *= rec;
            return ret;
        }
        float2 &operator/=(float s) {
            float rec = 1.0f / s;
            this->x *= rec;
            this->y *= rec;
            return *this;
        }
        float2 operator+() const {
            return *this;
        }
        float2 operator-() const {
            return float2(-x, -y);
        }
    };
    
    struct float3 {
        union {
            float x;
            float r;
            float s0;
        };
        union {
            float y;
            float g;
            float s1;
        };
        union {
            float z;
            float b;
            float s2;
        };
        float dum;
        
        float3() : x(0), y(0), z(0) { };
        float3(float val) : x(val), y(val), z(val) { };
        float3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) { };
        float3(const float3 &v) : x(v.x), y(v.y), z(v.z) { };
        
        float3 operator+(const float3 &r) const {
            float3 ret{x, y, z};
            ret.x += r.x;
            ret.y += r.y;
            ret.z += r.z;
            return ret;
        }
        float3 &operator+=(const float3 &r) {
            this->x += r.x;
            this->y += r.y;
            this->z += r.z;
            return *this;
        }
        float3 operator+(float s) const {
            float3 ret{x, y, z};
            ret.x += s;
            ret.y += s;
            ret.z += s;
            return ret;
        }
        float3 operator-(const float3 &r) const {
            float3 ret{x, y, z};
            ret.x -= r.x;
            ret.y -= r.y;
            ret.z -= r.z;
            return ret;
        }
        float3 &operator-=(const float3 &r) {
            this->x -= r.x;
            this->y -= r.y;
            this->z -= r.z;
            return *this;
        }
        float3 operator*(const float3 &r) const {
            float3 ret{x, y, z};
            ret.x *= r.x;
            ret.y *= r.y;
            ret.z *= r.z;
            return ret;
        }
        float3 &operator*=(const float3 &r) {
            this->x *= r.x;
            this->y *= r.y;
            this->z *= r.z;
            return *this;
        }
        float3 operator*(float s) const {
            float3 ret{x, y, z};
            ret.x *= s;
            ret.y *= s;
            ret.z *= s;
            return ret;
        }
        float3 &operator*=(float s) {
            this->x *= s;
            this->y *= s;
            this->z *= s;
            return *this;
        }
        friend float3 operator*(float s, const float3 &r) {
            float3 ret{s * r.x, s * r.y, s * r.z};
            return ret;
        }
        float3 operator/(const float3 &r) const {
            float3 ret{x, y, z};
            ret.x /= r.x;
            ret.y /= r.y;
            ret.z /= r.z;
            return ret;
        }
        float3 &operator/=(const float3 &r) {
            this->x /= r.x;
            this->y /= r.y;
            this->z /= r.z;
            return *this;
        }
        float3 operator/(float s) const {
            float3 ret{x, y, z};
            float rec = 1.0f / s;
            ret.x *= rec;
            ret.y *= rec;
            ret.z *= rec;
            return ret;
        }
        float3 &operator/=(float s) {
            float rec = 1.0f / s;
            this->x *= rec;
            this->y *= rec;
            this->z *= rec;
            return *this;
        }
        float3 operator+() const {
            return *this;
        }
        float3 operator-() const {
            return float3(-x, -y, -z);
        }
    };
    
    inline float dot(const float3 &a, const float3 &b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    inline float3 cross(const float3 &a, const float3 &b) {
        return float3(a.y * b.z - a.z * b.y,
                      a.z * b.x - a.x * b.z,
                      a.x * b.y - a.y * b.x);
    }
    
    inline float length(const float3 &v) {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    
    inline float3 normalize(const float3 &v) {
        float l = length(v);
        return v / l;
    }
    
    template <typename T>
    T clamp(const T &a, const T &min, const T &max) {
        return std::max(std::min(a, max), min);
    }
    
    float3 convert_float3(const uchar3 &v) {
        return float3(v.x, v.y, v.z);
    }
}

#endif
