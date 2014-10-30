//
//  bvh_traversal.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef device_bvh_traversal_cl
#define device_bvh_traversal_cl

#include "global.cl"
#include "scene.cl"
#include "texture.cl"

//32bytes
struct __attribute__((aligned(16))) BBox {
    point3 min, max;
};

#ifdef USE_LBVH
// 48bytes
struct __attribute__((aligned(16))) LBVHInternalNode {
    BBox bbox;
    bool isChild[2]; uchar dum0[2];
    uint c[2];
};

// 48bytes
struct __attribute__((aligned(16))) LBVHLeafNode {
    BBox bbox;
    uint objIdx;
};
#else
//48bytes
struct __attribute__((aligned(16))) BVHNode {
    BBox bbox;
    uint children[2];
};
#endif

//------------------------

bool rayTriangleIntersection(const Scene* scene,
                             const float3* org, const float3* dir, ushort faceIdx,
                             float* t, Intersection* isect);
bool rayAABBIntersection(const global BBox* bb, const float3* org, const float3* dir);
bool rayIntersection(const Scene* scene,
                     const point3* org, const vector3* dir, Intersection* isect);

//------------------------

bool rayTriangleIntersection(const Scene* scene,
                             const float3* org, const float3* dir, ushort faceIdx,
                             float* t, Intersection* isect) {
    const global Face* face = scene->faces + faceIdx;
    const global point3* p0 = scene->vertices + face->p0;
    const global point3* p1 = scene->vertices + face->p1;
    const global point3* p2 = scene->vertices + face->p2;
    
    float3 edge01 = *p1 - *p0;
    float3 edge02 = *p2 - *p0;
    
    float3 p = cross(*dir, edge02);
    float det = dot(edge01, p);
    if (det > -0.000001f && det < 0.000001f)
        return false;
    float invDet = 1.0f / det;
    
    float3 d = *org - *p0;
    
    float b1 = dot(d, p) * invDet;
    if (b1 < 0.0f || b1 > 1.0f)
        return false;
    
    float3 q = cross(d, edge01);
    
    float b2 = dot(*dir, q) * invDet;
    if (b2 < 0.0f || b1 + b2 > 1.0f)
        return false;
    
    *t = dot(edge02, q) * invDet;
    if (*t < 0.0f)
        return false;
    isect->p = *org + *dir * *t;
    isect->gNormal = normalize(cross(edge01, edge02));
    
    float b0 = 1.0f - b1 - b2;
    
    bool hasVNormal = face->vn0 != UINT_MAX && face->vn1 != UINT_MAX && face->vn2 != UINT_MAX;
    if (hasVNormal)
        isect->sNormal = normalize(b0 * *(scene->normals + face->vn0) +
                                   b1 * *(scene->normals + face->vn1) +
                                   b2 * *(scene->normals + face->vn2));
    else
        isect->sNormal = isect->gNormal;
    
    isect->hasTangent = face->vt0 != UINT_MAX && face->vt1 != UINT_MAX && face->vt2 != UINT_MAX;
    if (isect->hasTangent)
        isect->sTangent = normalize(b0 * *(scene->tangents + face->vt0) +
                                    b1 * *(scene->tangents + face->vt1) +
                                    b2 * *(scene->tangents + face->vt2));
    
    bool hasUV = face->uv0 != UINT_MAX && face->uv1 != UINT_MAX && face->uv2 != UINT_MAX;
    if (hasUV) {
        float2 uv0 = *(scene->uvs + face->uv0);
        float2 uv1 = *(scene->uvs + face->uv1);
        float2 uv2 = *(scene->uvs + face->uv2);
        isect->uv = b0 * uv0 + b1 * uv1 + b2 * uv2;
        
        float2 dUV0m2 = uv0 - uv2;
        float2 dUV1m2 = uv1 - uv2;
        float3 dP0m2 = *p0 - *p2;
        float3 dP1m2 = *p1 - *p2;
        float invDetUV = 1.0f / (dUV0m2.x * dUV1m2.y - dUV0m2.y * dUV1m2.x);
        float3 uDir = invDetUV * (float3)(dUV1m2.y * dP0m2.x - dUV0m2.y * dP1m2.x,
                                          dUV1m2.y * dP0m2.y - dUV0m2.y * dP1m2.y,
                                          dUV1m2.y * dP0m2.z - dUV0m2.y * dP1m2.z);
        if (hasVNormal)
            isect->uDir = normalize(cross(cross(isect->sNormal, uDir), isect->sNormal));
        else
            isect->uDir = normalize(uDir);
    }
    
    if (hasUV && face->alphaTexPtr != UINT_MAX) {
        //原因不明だがOS X (MBP Retina Late 2013 GT750M)だとこのprintfがあると落ちない。
        //アルファテクスチャーが無いシーン、つまりこのif文に入り得ないシーンでもこの有無によって挙動が変わるので、
        //コンパイラーがif文によるWARPの実行マスク更新に関して誤ったコードを出力しているのかもしれない。
        //printf("")を入れることによって生成されるコードが変わり、結果的に正しくなっているのかも。
        //そういった意味ではprintfである必要は無いかも。
        printf("");
        if (evaluateAlphaTexture(scene->texturesData + face->alphaTexPtr, isect->uv) == 0.0f)
            return false;
    }
    
    return true;
}

bool rayAABBIntersection(const global BBox* bb, const float3* org, const float3* dir) {
    float candidatePlane[3];
    float a_org[3] = {org->x, org->y, org->z};
    float a_dir[3] = {dir->x, dir->y, dir->z};
    float bbmin[3] = {bb->min.x, bb->min.y, bb->min.z};
    float bbmax[3] = {bb->max.x, bb->max.y, bb->max.z};
    char LowMidUp[3];
    for (int i = 0; i < 3; ++i) {
        if (a_org[i] < bbmin[i]) {
            LowMidUp[i] = -1;
            candidatePlane[i] = bbmin[i];
        }
        else if (a_org[i] > bbmax[i]) {
            LowMidUp[i] = 1;
            candidatePlane[i] = bbmax[i];
        }
        else {
            LowMidUp[i] = 0;
        }
    }
    bool outside = (bool)(LowMidUp[0] | LowMidUp[1] | LowMidUp[2]);
    
    if (outside) {
        float maxt[3];
        for (int i = 0; i < 3; ++i) {
            if (LowMidUp[i] != 0 && a_dir[i] != 0)
                maxt[i] = (candidatePlane[i] - a_org[i]) / a_dir[i];
            else
                maxt[i] = -1;
        }
        
        int whichPlane = 0;
        if (maxt[whichPlane] < maxt[1])
            whichPlane = 1;
        if (maxt[whichPlane] < maxt[2])
            whichPlane = 2;
        
        if (maxt[whichPlane] < 0)
            return false;
        
        float coord;
        for (int i = 0; i < 3; ++i) {
            if (i != whichPlane) {
                coord = a_org[i] + maxt[whichPlane] * a_dir[i];
                if (coord < bbmin[i] || coord > bbmax[i])
                    return false;
            }
        }
    }
    
    return true;
}

#ifdef USE_LBVH
bool rayIntersection(const Scene* scene,
                     const point3* org, const vector3* dir, Intersection* isect) {
    float t = INFINITY;
    
    uint idxStack[64];
    uint depth = 0;
    idxStack[depth++] = 0;
    while (depth > 0) {
        --depth;
        const global LBVHInternalNode* inode = scene->LBVHInternalNodes + idxStack[depth];
        if (rayAABBIntersection(&inode->bbox, org, dir)) {
            for (int i = 0; i < 2; ++i) {
                if (inode->isChild[i] == false) {
                    idxStack[depth++] = inode->c[i];
                    continue;
                }
                const global LBVHLeafNode* lnode = scene->LBVHLeafNodes + inode->c[i];
                if (rayAABBIntersection(&lnode->bbox, org, dir)) {
                    float tt = INFINITY;
                    Intersection lIsect;
                    if (rayTriangleIntersection(scene, org, dir, lnode->objIdx, &tt, &lIsect)) {
                        if (tt < t) {
                            t = tt;
                            *isect = lIsect;
                            isect->t = t;
                            isect->faceID = lnode->objIdx;
                        }
                    }
                }
            }
        }
    }
    
    return !isinf(t);
}
#else
bool rayIntersection(const Scene* scene,
                     const point3* org, const vector3* dir, Intersection* isect) {
    float t = INFINITY;
    
    uint idxStack[64];
    uint depth = 0;
    idxStack[depth++] = 0;
    while (depth > 0) {
        --depth;
        const global BVHNode* node = scene->BVHNodes + idxStack[depth];
        if (rayAABBIntersection(&node->bbox, org, dir)) {
            if (node->children[1] != UINT_MAX) {
                idxStack[depth++] = node->children[1];
                idxStack[depth++] = node->children[0];
            }
            else {
                float tt = INFINITY;
                Intersection lIsect;
                if (rayTriangleIntersection(scene, org, dir, node->children[0], &tt, &lIsect)) {
                    if (tt < t) {
                        t = tt;
                        *isect = lIsect;
                        isect->t = t;
                        isect->faceID = node->children[0];
                    }
                }
            }
        }
    }
    
    return !isinf(t);
}
#endif

#endif
