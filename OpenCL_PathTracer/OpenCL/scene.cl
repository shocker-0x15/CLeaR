//
//  scene.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef device_scene_cl
#define device_scene_cl

#include "global.cl"
#include "distribution.cl"

typedef struct BBox BBox;
#ifdef __USE_LBVH
typedef struct LBVHInternalNode LBVHInternalNode;
typedef struct LBVHLeafNode LBVHLeafNode;
#else
typedef struct BVHNode BVHNode;
#endif

// 56bytes
typedef struct __attribute__((aligned(4))) {
    uint p0, p1, p2;
    uint vn0, vn1, vn2;
    uint vt0, vt1, vt2;
    uint uv0, uv1, uv2;
    uint alphaTexPtr;
    ushort matPtr, lightPtr;
} Face;

// 8bytes
typedef struct __attribute__((aligned(4))) {
    uchar atInfinity;
    uint reference __attribute__((aligned(4)));
} LightInfo;

// 128bytes
typedef struct __attribute__((aligned(64))) {
    uint width, height;
    mat4x4 localToWorld;
} CameraHead;

// 4bytes
typedef struct __attribute__((aligned(4))) {
    uint idx_envLightProperty;
} EnvironmentHead;

typedef struct {
    global point3* vertices;
    global vector3* normals;
    global vector3* tangents;
    global float2* uvs;
    global Face* faces;
    global LightInfo* lights;
    global uchar* materialsData;
    global uchar* texturesData;
    global uchar* otherResourcesData;
#ifdef __USE_LBVH
    global LBVHInternalNode* LBVHInternalNodes;
    global LBVHLeafNode* LBVHLeafNodes;
#else
    global BVHNode* BVHNodes;
#endif
    global CameraHead* camera;
    global EnvironmentHead* environment;
    global Discrete1D* lightPowerDistribution;
} Scene;

#endif
