//
//  sim_bvh_traversal.hpp
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef sim_bvh_traversal_cl
#define sim_bvh_traversal_cl

#include "sim_global.hpp"
#include "sim_scene.hpp"
#include "sim_texture.hpp"

namespace sim {
    // 32bytes
    struct BBox {
        point3 min, max;
    };
    
#ifdef __USE_LBVH
    // 48bytes
    struct LBVHInternalNode {
        BBox bbox;
        bool isChild[2]; uchar dum0[2];
        uint c[2]; uchar dum1[4];
    };
    
    // 48bytes
    struct LBVHLeafNode {
        BBox bbox;
        uint objIdx; uchar dum0[12];
    };
#else
    // 48bytes
    struct BVHNode {
        BBox bbox;
        uint children[2]; uchar dum[8];
    };
#endif
    
    //------------------------
    
    bool rayTriangleIntersection(const Scene* scene,
                                 const point3* org, const vector3* dir, ushort faceIdx,
                                 float* t, Intersection* isect);
    void calcHitpointParameters(const Scene* scene, const Intersection* isect, SurfacePoint* surfPt);
    bool rayAABBIntersection(const BBox* bb, const point3* org, const vector3* dir, float tBound);
    bool rayIntersection(const Scene* scene,
                         const point3* org, const vector3* dir, Intersection* isect);
    
    //------------------------
    
    bool rayTriangleIntersection(const Scene* scene,
                                 const point3* org, const vector3* dir, ushort faceIdx,
                                 float* t, Intersection* isect) {
        const Face* face = scene->faces + faceIdx;
        const point3* p0 = scene->vertices + face->p0;
        const point3* p1 = scene->vertices + face->p1;
        const point3* p2 = scene->vertices + face->p2;
        
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
        
        float tt = dot(edge02, q) * invDet;
        if (tt < 0.0f || tt > *t)
            return false;
        
        *t = tt;
        float b0 = 1.0f - b1 - b2;
        isect->p = *org + *dir * *t;
        isect->gNormal = normalize(cross(edge01, edge02));
        isect->faceID = faceIdx;
        isect->param = float2(b0, b1);
        
        bool hasUV = face->uv0 != UINT_MAX;// && face->uv1 != UINT_MAX && face->uv2 != UINT_MAX;
        if (hasUV) {
            float2 uv0 = *(scene->uvs + face->uv0);
            float2 uv1 = *(scene->uvs + face->uv1);
            float2 uv2 = *(scene->uvs + face->uv2);
            isect->uv = b0 * uv0 + b1 * uv1 + b2 * uv2;
        }
        
        if (hasUV && face->alphaTexPtr != UINT_MAX) {
            if (evaluateAlphaTexture(scene->texturesData + face->alphaTexPtr, isect->uv) == 0.0f)
                return false;
        }
        
        return true;
    }
    
    void calcHitpointParameters(const Scene* scene, const Intersection* isect, SurfacePoint* surfPt) {
        surfPt->faceID = isect->faceID;
        const Face* face = scene->faces + surfPt->faceID;
        surfPt->p = isect->p;
        surfPt->gNormal = isect->gNormal;
        surfPt->uv = isect->uv;
        
        const float b0 = isect->param.s0;
        const float b1 = isect->param.s1;
        const float b2 = 1.0f - b0 - b1;
        
        bool hasVNormal = face->vn0 != UINT_MAX;// && face->vn1 != UINT_MAX && face->vn2 != UINT_MAX;
        if (hasVNormal)
            surfPt->sNormal = normalize(b0 * *(scene->normals + face->vn0) +
                                        b1 * *(scene->normals + face->vn1) +
                                        b2 * *(scene->normals + face->vn2));
        else
            surfPt->sNormal = isect->gNormal;
        
        surfPt->hasTangent = face->vt0 != UINT_MAX;// && face->vt1 != UINT_MAX && face->vt2 != UINT_MAX;
        if (surfPt->hasTangent)
            surfPt->sTangent = normalize(b0 * *(scene->tangents + face->vt0) +
                                         b1 * *(scene->tangents + face->vt1) +
                                         b2 * *(scene->tangents + face->vt2));
        
        bool hasUV = face->uv0 != UINT_MAX;// && face->uv1 != UINT_MAX && face->uv2 != UINT_MAX;
        if (hasUV) {
            const float2 uv0 = *(scene->uvs + face->uv0);
            const float2 uv1 = *(scene->uvs + face->uv1);
            const float2 uv2 = *(scene->uvs + face->uv2);
            float2 dUV0m2 = uv0 - uv2;
            float2 dUV1m2 = uv1 - uv2;
            
            const point3 p0 = *(scene->vertices + face->p0);
            const point3 p1 = *(scene->vertices + face->p1);
            const point3 p2 = *(scene->vertices + face->p2);
            float3 dP0m2 = p0 - p2;
            float3 dP1m2 = p1 - p2;
            
            float invDetUV = 1.0f / (dUV0m2.x * dUV1m2.y - dUV0m2.y * dUV1m2.x);
            float3 uDir = invDetUV * float3(dUV1m2.y * dP0m2.x - dUV0m2.y * dP1m2.x,
                                            dUV1m2.y * dP0m2.y - dUV0m2.y * dP1m2.y,
                                            dUV1m2.y * dP0m2.z - dUV0m2.y * dP1m2.z);
            if (hasVNormal)
                surfPt->uDir = normalize(cross(cross(surfPt->sNormal, uDir), surfPt->sNormal));
            else
                surfPt->uDir = normalize(uDir);
        }
    }
    
    bool rayAABBIntersection(const BBox* bb, const point3* org, const vector3* dir, float tBound) {
        const point3 bboxmin = bb->min;
        const point3 bboxmax = bb->max;
        int3 lowMidUp = ternaryOp<int3>(*org < bboxmin, -1, ternaryOp<int3>(*org > bboxmax, 1, 0));
        
        if (lowMidUp.x | lowMidUp.y | lowMidUp.z) {
            float3 candidatePlane = ternaryOp(*org < bboxmin, bboxmin, ternaryOp<float3>(*org > bboxmax, bboxmax, 0));
            float3 t = ternaryOp<float3>(lowMidUp != 0 && *dir != 0, (candidatePlane - *org) / *dir, -1.0f);
            float maxt = fmax(t.x, fmax(t.y, t.z));
            if (maxt < 0 || maxt > tBound)
                return false;
            
            point3 coord = *org + maxt * *dir;
            int3 hit = (coord >= bboxmin && coord <= bboxmax) || t == maxt;
            if (!(hit.x && hit.y && hit.z))
                return false;
        }
        
        return true;
    }
    
#ifdef __USE_LBVH
    bool rayIntersection(const Scene* scene,
                         const point3* org, const vector3* dir, SurfacePoint* surfPt) {
        float t = INFINITY;
        
        uint idxStack[64];
        uint depth = 0;
        Intersection isect[2];
        uchar curIsect = 0;
        idxStack[depth++] = 0;
        while (depth > 0) {
            --depth;
            const LBVHInternalNode* inode = scene->LBVHInternalNodes + idxStack[depth];
            if (!rayAABBIntersection(&inode->bbox, org, dir, t))
                continue;
            for (int i = 0; i < 2; ++i) {
                if (inode->isChild[i] == false) {
                    idxStack[depth++] = inode->c[i];
                    continue;
                }
                const LBVHLeafNode* lnode = scene->LBVHLeafNodes + inode->c[i];
                if (!rayAABBIntersection(&lnode->bbox, org, dir, t))
                    continue;
                
                if (rayTriangleIntersection(scene, org, dir, lnode->objIdx, &t, isect + (curIsect + 1) % 2))
                    ++curIsect;
            }
        }
        
        bool hit = !isinf(t);
        if (hit)
            calcHitpointParameters(scene, isect + curIsect % 2, surfPt);
        
        return hit;
    }
#else
    bool rayIntersection(const Scene* scene,
                         const point3* org, const vector3* dir, SurfacePoint* surfPt) {
        float t = INFINITY;
        
        uint idxStack[64];
        uint depth = 0;
        Intersection isect[2];
        uchar curIsect = 0;
        idxStack[depth++] = 0;
        while (depth > 0) {
            --depth;
            const BVHNode* node = scene->BVHNodes + idxStack[depth];
            if (!rayAABBIntersection(&node->bbox, org, dir, t))
                continue;
            if (node->children[1] != UINT_MAX) {
                idxStack[depth++] = node->children[1];
                idxStack[depth++] = node->children[0];
            }
            else {
                if (rayTriangleIntersection(scene, org, dir, node->children[0], &t, isect + (curIsect + 1) % 2))
                    ++curIsect;
            }
        }
        
        bool hit = !isinf(t);
        if (hit)
            calcHitpointParameters(scene, isect + curIsect % 2, surfPt);
        
        return hit;
    }
#endif
}

#endif
