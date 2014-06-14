#ifndef sim_global_cl
#define sim_global_cl

#include "cl12.hpp"
#include "sim_base.hpp"

namespace sim {
#define printfF3(f3) printf(#f3": %f, %f, %f\n", (f3).x, (f3).y, (f3).z)
#define printfMat4(mat) printf(#mat": \n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",\
                               head->localToWorld.s0, head->localToWorld.s4, head->localToWorld.s8, head->localToWorld.sc,\
                               head->localToWorld.s1, head->localToWorld.s5, head->localToWorld.s9, head->localToWorld.sd,\
                               head->localToWorld.s2, head->localToWorld.s6, head->localToWorld.sa, head->localToWorld.se,\
                               head->localToWorld.s3, head->localToWorld.s7, head->localToWorld.sb, head->localToWorld.sf)
#define printSize(t) printf("sizeof("#t"): %u\n", sizeof(t))
    
    typedef float2 point2;
    typedef float3 vector3;
    typedef float3 point3;
    typedef float3 color;
    typedef float4 vector4;
    typedef float4 point4;
    typedef float16 mat4x4;
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
    
    //52bytes
    typedef struct {
        uint p0, p1, p2;
        uint vn0, vn1, vn2;
        uint vt0, vt1, vt2;
        uint uv0, uv1, uv2;
        ushort matPtr, lightPtr;
    } Face;
    
    //8bytes
    typedef struct {
        uchar atInfinity; uchar dum0[3];
        uint reference;
    } LightInfo;
    
    //32bytes
    typedef struct {
        point3 min, max;
    } BBox;
    
    //48bytes
    typedef struct {
        BBox bbox;
        uint children[2]; uchar dum[8];
    } BVHNode;
    
    //112bytes
    typedef struct {
        point3 p;
        vector3 gNormal;
        vector3 sNormal;
        vector3 sTangent;
        vector3 uDir;
        float2 uv;
        float t;
        uint faceID;
        bool hasTangent; uchar dum[15];
    } Intersection;
    
    //96bytes
    typedef struct {
        point3 p;
        vector3 gNormal;
        vector3 sNormal;
        vector3 sTangent;
        vector3 uDir;
        float2 uv;
        uint faceID;
        bool hasTangent; uchar dum[3];
    } LightPosition;
    
    //48bytes
    typedef struct {
        point3 p;
        vector3 n;
        float2 uv; uchar dum[8];
    } LensPosition;
    
    //128bytes
    typedef struct {
        uint width, height; uchar dum0[56];
        mat4x4 localToWorld;
    } CameraHead;
    
    typedef struct {
        uint dummy;
    } EnvironmentHead;
    
    typedef struct {
        point3* vertices;
        vector3* normals;
        vector3* tangents;
        float2* uvs;
        Face* faces;
        LightInfo* lights;
        uint numLights;
        uchar* materialsData;
        uchar* texturesData;
        BVHNode* BVHNodes;
        CameraHead* camera;
        EnvironmentHead* environment;
        uchar* lightPowerCDF;
    } Scene;
    
    //------------------------
    
    inline void memcpyG2P(uchar* dst, const uchar* src, uint numBytes);
    inline void AlignPtr(uchar** ptr, uintptr_t bytes);
    inline void AlignPtrG(const uchar** ptr, uintptr_t bytes);
    uchar* AlignPtrAdd(uchar** ptr, uintptr_t bytes);
    const uchar* AlignPtrAddG(const uchar** ptr, uintptr_t bytes);
    inline bool zeroVec(const float3* v);
    inline float maxComp(const float3* v);
    inline float luminance(const color* c);
    void makeTangent(const vector3* n, vector3* tangent);
    inline vector3 worldToLocal(const vector3* s, const vector3* t, const vector3* n, const vector3* v);
    inline vector3 localToWorld(const vector3* s, const vector3* t, const vector3* n, const vector3* v);
    inline float distance2(const point3* p0, const point3* p1);
    inline void LightPositionFromIntersection(const Intersection* isect, LightPosition* lpos);
    
    //------------------------
    
    inline void memcpyG2P(uchar* dst, const uchar* src, uint numBytes) {
        for (uint i = 0; i < numBytes; ++i)
            *(dst++) = *(src++);
    }
    
    inline void AlignPtr(uchar** ptr, uintptr_t bytes) {
        *ptr = (uchar*)(((uintptr_t)*ptr + (bytes - 1)) & ~(bytes - 1));
    }
    
    inline void AlignPtrG(const uchar** ptr, uintptr_t bytes) {
        *ptr = (const uchar*)(((uintptr_t)*ptr + (bytes - 1)) & ~(bytes - 1));
    }
    
    uchar* AlignPtrAdd(uchar** ptr, uintptr_t bytes) {
        uchar* ptrAligned = (uchar*)(((ulong)*ptr + (bytes - 1)) & ~(bytes - 1));
        *ptr = ptrAligned + bytes;
        return ptrAligned;
    }
    
    const uchar* AlignPtrAddG(const uchar** ptr, uintptr_t bytes) {
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
    
    void makeTangent(const vector3* n, vector3* tangent) {
        if (fabsf(n->x) > fabsf(n->y)) {
            float invLen = 1.0f / sqrtf(n->x * n->x + n->z * n->z);
            *tangent = vector3(-n->z * invLen, 0.0f, n->x * invLen);
        }
        else {
            float invLen = 1.0f / sqrtf(n->y * n->y + n->z * n->z);
            *tangent = vector3(0.0f, n->z * invLen, -n->y * invLen);
        }
    }
    
    inline vector3 worldToLocal(const vector3* s, const vector3* t, const vector3* n, const vector3* v) {
        return vector3(dot(*s, *v), dot(*t, *v), dot(*n, *v));
    }
    
    inline vector3 localToWorld(const vector3* s, const vector3* t, const vector3* n, const vector3* v) {
        return vector3(s->x * v->x + t->x * v->y + n->x * v->z,
                       s->y * v->x + t->y * v->y + n->y * v->z,
                       s->z * v->x + t->z * v->y + n->z * v->z);
    }
    
    inline float distance2(const point3* p0, const point3* p1) {
        return (p1->x - p0->x) * (p1->x - p0->x) + (p1->y - p0->y) * (p1->y - p0->y) + (p1->z - p0->z) * (p1->z - p0->z);
    }
    
    inline void LightPositionFromIntersection(const Intersection* isect, LightPosition* lpos) {
        lpos->p = isect->p;
        lpos->gNormal = isect->gNormal;
        lpos->sNormal = isect->sNormal;
        lpos->sTangent = isect->sTangent;
        lpos->uDir = isect->uDir;
        lpos->uv = isect->uv;
        lpos->faceID = isect->faceID;
        lpos->hasTangent = isect->hasTangent;
    }
}

#endif