#ifndef sim_global_cl
#define sim_global_cl

#include "cl12.hpp"
#include "sim_base.h"

namespace sim {
#define printfF3(f3) printf(#f3": %f, %f, %f\n", (f3).x, (f3).y, (f3).z)
    
    typedef float3 vector3;
    typedef float3 point3;
    typedef float3 color;
#define colorZero color(0.0f, 0.0f, 0.0f)
#define colorOne color(1.0f, 1.0f, 1.0f)
#define EPSILON 0.00001f;
    
#define ALIGN(ad, w) ((ad) + ((w) - 1)) & ~((w) - 1)
    
    //48bytes
    typedef struct {
        point3 org;
        vector3 dir;
        uint depth; uchar dum[12];
    } Ray;
    
    //48bytes
    typedef struct {
        uint p0, p1, p2;
        uint ns0, ns1, ns2;
        uint uv0, uv1, uv2;
        ushort matPtr, lightPtr; uchar dum[8];
    } Face;
    
    //32bytes
    typedef struct {
        point3 min, max;
    } BBox;
    
    //48bytes
    typedef struct {
        BBox bbox;
        uint children[2]; uchar dum[8];
    } BVHNode;
    
    //64bytes
    typedef struct {
        point3 p;
        vector3 ng;
        vector3 ns;
        float2 uv;
        float t;
        uint faceID;
    } Intersection;
    
    //64bytes
    typedef struct {
        point3 p;
        vector3 ng;
        vector3 ns;
        float2 uv;
        uint faceID; uchar dum[4];
    } LightPosition;
    
    typedef struct {
        point3* vertices;
        vector3* normals;
        vector3* tangents;
        float2* uvs;
        Face* faces;
        uint* lights;
        uint numLights;
        uchar* materialsData;
        uchar* lightsData;
        uchar* texturesData;
        BVHNode* BVHNodes;
    } Scene;
    
    //------------------------
    
    inline void memcpyG2P(uchar* dst, const uchar* src, uint numBytes);
    inline bool zeroVec(const float3* v);
    inline float maxComp(const float3* v);
    inline void makeBasis(const vector3* n, vector3* s, vector3* t);
    inline vector3 worldToLocal(const vector3* s, const vector3* t, const vector3* n, const vector3* v);
    inline vector3 localToWorld(const vector3* s, const vector3* t, const vector3* n, const vector3* v);
    inline void LightPositionFromIntersection(const Intersection* isect, LightPosition* lpos);
    
    //------------------------
    
    inline void memcpyG2P(uchar* dst, const uchar* src, uint numBytes) {
        for (uint i = 0; i < numBytes; ++i)
            *(dst++) = *(src++);
    }
    
    inline void AlignPtr(uchar** ptr, uintptr_t bytes) {
        *ptr = (uchar*)(((uintptr_t)*ptr + (bytes - 1)) & ~(bytes - 1));
    }
    
    inline uchar* AlignPtrAdd(uchar** ptr, uintptr_t bytes) {
        uchar* ptrAligned = (uchar*)(((ulong)*ptr + (bytes - 1)) & ~(bytes - 1));
        *ptr = ptrAligned + bytes;
        return ptrAligned;
    }
    
    inline const uchar* AlignPtrAddG(const uchar** ptr, uintptr_t bytes) {
        const uchar* ptrAligned = (const uchar*)(((ulong)*ptr + (bytes - 1)) & ~(bytes - 1));
        *ptr = ptrAligned + bytes;
        return ptrAligned;
    }
    
    inline bool zeroVec(const float3* v) {
        return v->x == 0.0f && v->y == 0.0f && v->z == 0.0f;
    }
    
    inline float maxComp(const float3* v) {
        return fmaxf(v->x, fmaxf(v->y, v->z));
    }
    
    inline float luminance(const color* c) {
        return 0.2126f * c->r + 0.7152f * c->g + 0.0722f * c->b;
    }
    
    inline void makeBasis(const vector3* n, vector3* s, vector3* t) {
        if (fabsf(n->x) > fabsf(n->y)) {
            float invLen = 1.0f / sqrtf(n->x * n->x + n->z * n->z);
            *s = vector3(-n->z * invLen, 0.0f, n->x * invLen);
        }
        else {
            float invLen = 1.0f / sqrtf(n->y * n->y + n->z * n->z);
            *s = vector3(0.0f, n->z * invLen, -n->y * invLen);
        }
        *t = cross(*n, *s);
    }
    
    inline vector3 worldToLocal(const vector3* s, const vector3* t, const vector3* n, const vector3* v) {
        return vector3(dot(*s, *v), dot(*t, *v), dot(*n, *v));
    }
    
    inline vector3 localToWorld(const vector3* s, const vector3* t, const vector3* n, const vector3* v) {
        return vector3(s->x * v->x + t->x * v->y + n->x * v->z,
                       s->y * v->x + t->y * v->y + n->y * v->z,
                       s->z * v->x + t->z * v->y + n->z * v->z);
    }
    
    inline float dist2(const point3* p0, const point3* p1) {
        return (p1->x - p0->x) * (p1->x - p0->x) + (p1->y - p0->y) * (p1->y - p0->y) + (p1->z - p0->z) * (p1->z - p0->z);
    }
    
    inline void LightPositionFromIntersection(const Intersection* isect, LightPosition* lpos) {
        lpos->p = isect->p;
        lpos->ng = isect->ng;
        lpos->ns = isect->ns;
        lpos->uv = isect->uv;
        lpos->faceID = isect->faceID;
    }
}

#endif