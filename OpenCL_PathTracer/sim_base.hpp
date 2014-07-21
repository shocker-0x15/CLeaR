//
//  sim_base.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/11.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_sim_base_hpp
#define OpenCL_PathTracer_sim_base_hpp

#include <stdint.h>
#include <cmath>
#include <algorithm>
#include <half.h>

#define M_PI_F ((float)M_PI)
#define sw2(m, s0, s1) float2((m).s0, (m).s1)
#define sw3(m, s0, s1, s2) float3((m).s0, (m).s1, (m).s2)
#define sw4(m, s0, s1, s2, s3) float4((m).s0, (m).s1, (m).s2, (m).s3)

namespace sim {
    typedef uint8_t uchar;
    typedef uint16_t ushort;
    typedef uint32_t uint;
    typedef uint64_t ulong;
    typedef uint64_t uintptr_t;
    
    uint global_ids[2];
    uint global_sizes[2];
    uint global_offsets[2];
    inline uint get_global_id(uint dim) {
        return global_ids[dim];
    }
    inline uint get_global_size(uint dim) {
        return global_sizes[dim];
    }
    inline uint get_global_offset(uint dim) {
        return global_offsets[dim];
    }
    
    
    inline float as_float(uint bits) {
        return *(float*)&bits;
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
    
    
    struct uchar4 {
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
        union {
            uchar w;
            uchar s3;
        };
        
        uchar4() : x(0), y(0), z(0) { };
        uchar4(uchar xx, uchar yy, uchar zz, uchar ww) : x(xx), y(yy), z(zz), w(ww) { };
        uchar4(const uchar4 &v) : x(v.x), y(v.y), z(v.z), w(v.w) { };
        
        uchar4 operator+(const uchar4 &r) const {
            uchar4 ret{x, y, z, w};
            ret.x += r.x;
            ret.y += r.y;
            ret.z += r.z;
            ret.w += r.w;
            return ret;
        }
        uchar4 &operator+=(const uchar4 &r) {
            this->x += r.x;
            this->y += r.y;
            this->z += r.z;
            this->w += r.w;
            return *this;
        }
        uchar4 operator-(const uchar4 &r) const {
            uchar4 ret{x, y, z, w};
            ret.x -= r.x;
            ret.y -= r.y;
            ret.z -= r.z;
            ret.w -= r.w;
            return ret;
        }
        uchar4 &operator-=(const uchar4 &r) {
            this->x -= r.x;
            this->y -= r.y;
            this->z -= r.z;
            this->w -= r.w;
            return *this;
        }
        uchar4 operator*(const uchar4 &r) const {
            uchar4 ret{x, y, z, w};
            ret.x *= r.x;
            ret.y *= r.y;
            ret.z *= r.z;
            ret.w *= r.w;
            return ret;
        }
        uchar4 &operator*=(const uchar4 &r) {
            this->x *= r.x;
            this->y *= r.y;
            this->z *= r.z;
            this->w *= r.w;
            return *this;
        }
        uchar4 operator*(uchar s) const {
            uchar4 ret{x, y, z, w};
            ret.x *= s;
            ret.y *= s;
            ret.z *= s;
            ret.w *= s;
            return ret;
        }
        uchar4 &operator*=(uchar s) {
            this->x *= s;
            this->y *= s;
            this->z *= s;
            this->w *= s;
            return *this;
        }
        friend uchar4 operator*(uchar s, const uchar4 &r) {
            uchar4 ret(s * r.x, s * r.y, s * r.z, s * r.w);
            return ret;
        }
        uchar4 operator/(const uchar4 &r) const {
            uchar4 ret{x, y, z, w};
            ret.x /= r.x;
            ret.y /= r.y;
            ret.z /= r.z;
            ret.w /= r.w;
            return ret;
        }
        uchar4 &operator/=(const uchar4 &r) {
            this->x /= r.x;
            this->y /= r.y;
            this->z /= r.z;
            this->w /= r.w;
            return *this;
        }
        uchar4 operator/(uchar s) const {
            uchar4 ret{x, y, z, w};
            float rec = 1.0f / s;
            ret.x *= rec;
            ret.y *= rec;
            ret.z *= rec;
            ret.w *= rec;
            return ret;
        }
        uchar4 &operator/=(uchar s) {
            float rec = 1.0f / s;
            this->x *= rec;
            this->y *= rec;
            this->z *= rec;
            this->w *= rec;
            return *this;
        }
        uchar4 operator+() const {
            return *this;
        }
        uchar4 operator-() const {
            return uchar4(-x, -y, -z, -w);
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
        float3(const float2 &v2, float zz) : x(v2.x), y(v2.y), z(zz) { };
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
        
        bool hasNanInf() const {
            return (std::isnan(x) || std::isnan(y) || std::isnan(z) ||
                    std::isinf(x) || std::isinf(y) || std::isinf(z));
        }
        bool hasNeg() const {
            return x < 0.0f || y < 0.0f || z < 0.0f;
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
    
    inline float3 convert_float3(const uchar3 &v) {
        return float3(v.x, v.y, v.z);
    }
    
    
    struct float4 {
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
        union {
            float w;
            float a;
            float s3;
        };
        
        float4() : x(0), y(0), z(0), w(0) { };
        float4(float val) : x(val), y(val), z(val), w(val) { };
        float4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) { };
        float4(const float3 &v3, float ww) : x(v3.x), y(v3.y), z(v3.z), w(ww) { };
        float4(const float4 &v) : x(v.x), y(v.y), z(v.z), w(v.w) { };
        
        float4 operator+(const float4 &r) const {
            float4 ret{x, y, z, w};
            ret.x += r.x;
            ret.y += r.y;
            ret.z += r.z;
            ret.w += r.w;
            return ret;
        }
        float4 &operator+=(const float4 &r) {
            this->x += r.x;
            this->y += r.y;
            this->z += r.z;
            this->w += r.w;
            return *this;
        }
        float4 operator+(float s) const {
            float4 ret{x, y, z, w};
            ret.x += s;
            ret.y += s;
            ret.z += s;
            ret.w += s;
            return ret;
        }
        float4 operator-(const float4 &r) const {
            float4 ret{x, y, z, w};
            ret.x -= r.x;
            ret.y -= r.y;
            ret.z -= r.z;
            ret.w -= r.w;
            return ret;
        }
        float4 &operator-=(const float4 &r) {
            this->x -= r.x;
            this->y -= r.y;
            this->z -= r.z;
            this->w -= r.w;
            return *this;
        }
        float4 operator*(const float4 &r) const {
            float4 ret{x, y, z, w};
            ret.x *= r.x;
            ret.y *= r.y;
            ret.z *= r.z;
            ret.w *= r.w;
            return ret;
        }
        float4 &operator*=(const float4 &r) {
            this->x *= r.x;
            this->y *= r.y;
            this->z *= r.z;
            this->w *= r.w;
            return *this;
        }
        float4 operator*(float s) const {
            float4 ret{x, y, z, w};
            ret.x *= s;
            ret.y *= s;
            ret.z *= s;
            ret.w *= s;
            return ret;
        }
        float4 &operator*=(float s) {
            this->x *= s;
            this->y *= s;
            this->z *= s;
            this->w *= s;
            return *this;
        }
        friend float4 operator*(float s, const float4 &r) {
            float4 ret{s * r.x, s * r.y, s * r.z, s * r.w};
            return ret;
        }
        float4 operator/(const float4 &r) const {
            float4 ret{x, y, z, w};
            ret.x /= r.x;
            ret.y /= r.y;
            ret.z /= r.z;
            ret.w /= r.w;
            return ret;
        }
        float4 &operator/=(const float4 &r) {
            this->x /= r.x;
            this->y /= r.y;
            this->z /= r.z;
            this->w /= r.w;
            return *this;
        }
        float4 operator/(float s) const {
            float4 ret{x, y, z, w};
            float rec = 1.0f / s;
            ret.x *= rec;
            ret.y *= rec;
            ret.z *= rec;
            ret.w *= rec;
            return ret;
        }
        float4 &operator/=(float s) {
            float rec = 1.0f / s;
            this->x *= rec;
            this->y *= rec;
            this->z *= rec;
            this->w *= rec;
            return *this;
        }
        float4 operator+() const {
            return *this;
        }
        float4 operator-() const {
            return float4(-x, -y, -z, -w);
        }
    };
    
    inline float dot(const float4 &a, const float4 &b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    

    struct float16 {
        union { float s0; };
        union { float s1; };
        union { float s2; };
        union { float s3; };
        union { float s4; };
        union { float s5; };
        union { float s6; };
        union { float s7; };
        union { float s8; };
        union { float s9; };
        union { float sa, sA; };
        union { float sb, sB; };
        union { float sc, sC; };
        union { float sd, sD; };
        union { float se, sE; };
        union { float sf, sF; };
        
        float16() :
        s0(0), s1(0), s2(0), s3(0),
        s4(0), s5(0), s6(0), s7(0),
        s8(0), s9(0), sa(0), sb(0),
        sc(0), sd(0), se(0), sf(0) { };
        float16(float val) :
        s0(val), s1(val), s2(val), s3(val),
        s4(val), s5(val), s6(val), s7(val),
        s8(val), s9(val), sa(val), sb(val),
        sc(val), sd(val), se(val), sf(val) { };
        float16(float s0, float s1, float s2, float s3,
                float s4, float s5, float s6, float s7,
                float s8, float s9, float sa, float sb,
                float sc, float sd, float se, float sf) :
        s0(s0), s1(s1), s2(s2), s3(s3),
        s4(s4), s5(s5), s6(s6), s7(s7),
        s8(s8), s9(s9), sa(sa), sb(sb),
        sc(sc), sd(sd), se(se), sf(sf) { };
        float16(const float16 &v) :
        s0(v.s0), s1(v.s1), s2(v.s2), s3(v.s3),
        s4(v.s4), s5(v.s5), s6(v.s6), s7(v.s7),
        s8(v.s8), s9(v.s9), sa(v.sa), sb(v.sb),
        sc(v.sc), sd(v.sd), se(v.se), sf(v.sf) { };
    };
    
    
    float4 vload_half4(size_t offset, const half* p) {
        float4 ret;
        const half* base = p + offset * 4;
        ret.s0 = float(*(base + 0));
        ret.s1 = float(*(base + 1));
        ret.s2 = float(*(base + 2));
        ret.s3 = float(*(base + 3));
        return ret;
    }
    
    
    template <typename T>
    T clamp(const T &a, const T &minv, const T &maxv) {
        return std::max(std::min(a, maxv), minv);
    }
}

#endif
