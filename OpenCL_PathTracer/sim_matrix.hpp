#ifndef sim_matrix_cl
#define sim_matrix_cl

#include "sim_global.hpp"

namespace sim {
    inline void mulMat4x4_P4(const mat4x4* m, const point4* p, point4* tp);
    inline void mulMat4x4G_P4(const mat4x4* m, const point4* p, point4* tp);
    void mulMat4x4G_P3(const mat4x4* m, const point3* p, point3* tp);
    inline void mulMat4x4_V4(const mat4x4* m, const vector4* v, vector4* tv);
    inline void mulMat4x4G_V4(const mat4x4* m, const vector4* v, vector4* tv);
    inline void mulMat4x4G_V3(const mat4x4* m, const vector3* v, vector3* tv);
    
    inline void mulMat4x4_P4(const mat4x4* m, const point4* p, point4* tp) {
        tp->x = dot(sw4(*m, s0, s4, s8, sc), *p);
        tp->y = dot(sw4(*m, s1, s5, s9, sd), *p);
        tp->z = dot(sw4(*m, s2, s6, sa, se), *p);
        tp->w = dot(sw4(*m, s3, s7, sb, sf), *p);
        if (tp->w != 1.0f)
            *tp /= tp->w;
    }
    
    inline void mulMat4x4G_P4(const mat4x4* m, const point4* p, point4* tp) {
        tp->x = dot(sw4(*m, s0, s4, s8, sc), *p);
        tp->y = dot(sw4(*m, s1, s5, s9, sd), *p);
        tp->z = dot(sw4(*m, s2, s6, sa, se), *p);
        tp->w = dot(sw4(*m, s3, s7, sb, sf), *p);
        if (tp->w != 1.0f)
            *tp /= tp->w;
    }
    
    void mulMat4x4G_P3(const mat4x4* m, const point3* p, point3* tp) {
        point4 p4 = point4(*p, 1.0f);
        tp->x = dot(sw4(*m, s0, s4, s8, sc), p4);
        tp->y = dot(sw4(*m, s1, s5, s9, sd), p4);
        tp->z = dot(sw4(*m, s2, s6, sa, se), p4);
        float w = dot(sw4(*m, s3, s7, sb, sf), p4);
        if (w != 1.0f)
        *tp /= w;
    }
    
    inline void mulMat4x4_V4(const mat4x4* m, const vector4* v, vector4* tv) {
        tv->x = dot(sw3(*m, s0, s4, s8), sw3(*v, x, y, z));
        tv->y = dot(sw3(*m, s1, s5, s9), sw3(*v, x, y, z));
        tv->z = dot(sw3(*m, s2, s6, sa), sw3(*v, x, y, z));
    }
    
    inline void mulMat4x4G_V4(const mat4x4* m, const vector4* v, vector4* tv) {
        tv->x = dot(sw3(*m, s0, s4, s8), sw3(*v, x, y, z));
        tv->y = dot(sw3(*m, s1, s5, s9), sw3(*v, x, y, z));
        tv->z = dot(sw3(*m, s2, s6, sa), sw3(*v, x, y, z));
    }
    
    inline void mulMat4x4G_V3(const mat4x4* m, const vector3* v, vector3* tv) {
        tv->x = dot(sw3(*m, s0, s4, s8), *v);
        tv->y = dot(sw3(*m, s1, s5, s9), *v);
        tv->z = dot(sw3(*m, s2, s6, sa), *v);
    }
}

#endif
