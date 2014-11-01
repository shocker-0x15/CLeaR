//
//  matrix.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef device_matrix_cl
#define device_matrix_cl

#include "global.cl"

inline void mulMat4x4_P4(const mat4x4* m, const point4* p, point4* tp);
inline void mulMat4x4G_P4(const global mat4x4* m, const point4* p, point4* tp);
void mulMat4x4G_P3(const global mat4x4* m, const point3* p, point3* tp);
inline void mulMat4x4_V4(const mat4x4* m, const vector4* v, vector4* tv);
inline void mulMat4x4G_V4(const global mat4x4* m, const vector4* v, vector4* tv);
inline void mulMat4x4G_V3(const global mat4x4* m, const vector3* v, vector3* tv);

inline void mulMat4x4_P4(const mat4x4* m, const point4* p, point4* tp) {
    tp->x = dot(m->s048c, *p);
    tp->y = dot(m->s159d, *p);
    tp->z = dot(m->s26ae, *p);
    tp->w = dot(m->s37bf, *p);
    if (tp->w != 1.0f)
        *tp /= tp->w;
}

inline void mulMat4x4G_P4(const global mat4x4* m, const point4* p, point4* tp) {
    tp->x = dot(m->s048c, *p);
    tp->y = dot(m->s159d, *p);
    tp->z = dot(m->s26ae, *p);
    tp->w = dot(m->s37bf, *p);
    if (tp->w != 1.0f)
        *tp /= tp->w;
}

void mulMat4x4G_P3(const global mat4x4* m, const point3* p, point3* tp) {
    point4 p4 = (point4)(*p, 1.0f);
    tp->x = dot(m->s048c, p4);
    tp->y = dot(m->s159d, p4);
    tp->z = dot(m->s26ae, p4);
    float w = dot(m->s37bf, p4);
    if (w != 1.0f)
        *tp /= w;
}

inline void mulMat4x4_V4(const mat4x4* m, const vector4* v, vector4* tv) {
    tv->x = dot(m->s048, v->xyz);
    tv->y = dot(m->s159, v->xyz);
    tv->z = dot(m->s26a, v->xyz);
}

inline void mulMat4x4G_V4(const global mat4x4* m, const vector4* v, vector4* tv) {
    tv->x = dot(m->s048, v->xyz);
    tv->y = dot(m->s159, v->xyz);
    tv->z = dot(m->s26a, v->xyz);
}

inline void mulMat4x4G_V3(const global mat4x4* m, const vector3* v, vector3* tv) {
    tv->x = dot(m->s048, *v);
    tv->y = dot(m->s159, *v);
    tv->z = dot(m->s26a, *v);
}

#endif
