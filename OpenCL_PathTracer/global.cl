#ifndef global_cl
#define global_cl

#define printfF3(f3) printf(#f3": %f, %f, %f\n", (f3).x, (f3).y, (f3).z)

typedef float3 vector3;
typedef float3 point3;
typedef float3 color;
#define EPSILON 0.00001f;

#define ALIGN(ad, w) ((ad) + ((w) - 1)) & ~((w) - 1)

typedef struct {
    point3 org;
    vector3 dir;
    uint depth;
} Ray;

//48bytes
typedef struct __attribute__((aligned(16))) {
    uint p0, p1, p2;
    uint ns0, ns1, ns2;
    uint uv0, uv1, uv2;
    ushort matPtr, lightPtr;
} Face;

//32bytes
typedef struct __attribute__((aligned(16))) {
    point3 min, max;
} BBox;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BBox bbox;
    uint children[2];
} BVHNode;

//64bytes
typedef struct __attribute__((aligned(16))) {
    point3 p;
    vector3 ng;
    vector3 sg;
    float2 uv;
    float t;
    uint faceID;
} Intersection;

typedef struct __attribute__((aligned(16))) {
    point3 p;
    vector3 ng;
    vector3 sg;
    float2 uv;
    uint faceID;
} LightPosition;

typedef struct {
    global point3* vertices;
    global vector3* normals;
    global vector3* binormals;
    global float2* uvs;
    global Face* faces;
    global uint* lights;
    uint numLights;
    global uchar* materialsData;
    global uchar* lightsData;
    global uchar* texturesData;
    global BVHNode* BVHNodes;
} Scene;

//------------------------

inline void memcpyG2P(uchar* dst, const global uchar* src, uint numBytes);
inline bool zeroVec(const float3* v);
inline float maxComp(const float3* v);
inline void makeBasis(const vector3* n, vector3* s, vector3* t);
inline vector3 worldToLocal(const vector3* s, const vector3* t, const vector3* n, const vector3* v);
inline vector3 localToWorld(const vector3* s, const vector3* t, const vector3* n, const vector3* v);
inline void LightPositionFromIntersection(const Intersection* isect, LightPosition* lpos);

//------------------------

inline void memcpyG2P(uchar* dst, const global uchar* src, uint numBytes) {
    for (int i = 0; i < numBytes; ++i)
        *(dst++) = *(src++);
}

inline uchar* AlignPtrAdd(uchar** ptr, uint bytes) {
    uchar* ptrAligned = (uchar*)(((uintptr_t)*ptr + (bytes - 1)) & ~(bytes - 1));
    *ptr = ptrAligned + bytes;
    return ptrAligned;
}

inline const global uchar* AlignPtrAddG(const global uchar** ptr, uint bytes) {
    const global uchar* ptrAligned = (const global uchar*)(((uintptr_t)*ptr + (bytes - 1)) & ~(bytes - 1));
    *ptr = ptrAligned + bytes;
    return ptrAligned;
}

inline bool zeroVec(const float3* v) {
    return v->x == 0.0f && v->y == 0.0f && v->z == 0.0f;
}

inline float maxComp(const float3* v) {
    return fmax(v->x, fmax(v->y, v->z));
}

inline void makeBasis(const vector3* n, vector3* s, vector3* t) {
    if (fabs(n->x) > fabs(n->y)) {
        float invLen = 1.0f / sqrt(n->x * n->x + n->z * n->z);
        *s = (vector3)(-n->z * invLen, 0.0f, n->x * invLen);
    }
    else {
        float invLen = 1.0f / sqrt(n->y * n->y + n->z * n->z);
        *s = (vector3)(0.0f, n->z * invLen, -n->y * invLen);
    }
    *t = cross(*n, *s);
}

inline vector3 worldToLocal(const vector3* s, const vector3* t, const vector3* n, const vector3* v) {
    return (vector3)(dot(*s, *v), dot(*t, *v), dot(*n, *v));
}

inline vector3 localToWorld(const vector3* s, const vector3* t, const vector3* n, const vector3* v) {
    return (vector3)(s->x * v->x + t->x * v->y + n->x * v->z,
                     s->y * v->x + t->y * v->y + n->y * v->z,
                     s->z * v->x + t->z * v->y + n->z * v->z);
}

inline void LightPositionFromIntersection(const Intersection* isect, LightPosition* lpos) {
    lpos->p = isect->p;
    lpos->ng = isect->ng;
    lpos->sg = isect->sg;
    lpos->uv = isect->uv;
    lpos->faceID = isect->faceID;
}

#endif