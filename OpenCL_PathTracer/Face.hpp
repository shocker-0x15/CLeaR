//
//  Face.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Face_hpp
#define OpenCL_PathTracer_Face_hpp

#include <cstdint>

// 56bytes
struct Face {
    uint32_t p0, p1, p2;
    uint32_t vn0, vn1, vn2;
    uint32_t vt0, vt1, vt2;
    uint32_t uv0, uv1, uv2;
    uint32_t alphaTexPtr;
    uint16_t matPtr, lightPtr;
    
    Face() :
    p0(UINT32_MAX), p1(UINT32_MAX), p2(UINT32_MAX),
    vn0(UINT32_MAX), vn1(UINT32_MAX), vn2(UINT32_MAX),
    vt0(UINT32_MAX), vt1(UINT32_MAX), vt2(UINT32_MAX),
    uv0(UINT32_MAX), uv1(UINT32_MAX), uv2(UINT32_MAX),
    alphaTexPtr(UINT32_MAX),
    matPtr(UINT16_MAX), lightPtr(UINT16_MAX) { };
    
    static Face make_P(uint32_t p0, uint32_t p1, uint32_t p2, uint16_t matPtr, uint16_t lightPtr = UINT16_MAX, uint32_t alphaTexPtr = UINT32_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.alphaTexPtr = alphaTexPtr;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
    
    static Face make_P_UV(uint32_t p0, uint32_t p1, uint32_t p2,
                          uint32_t uv0, uint32_t uv1, uint32_t uv2,
                          uint16_t matPtr, uint16_t lightPtr = UINT16_MAX, uint32_t alphaTexPtr = UINT32_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.uv0 = uv0; face.uv1 = uv1; face.uv2 = uv2;
        face.alphaTexPtr = alphaTexPtr;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
    
    static Face make_P_N(uint32_t p0, uint32_t p1, uint32_t p2,
                         uint32_t ns0, uint32_t ns1, uint32_t ns2,
                         uint16_t matPtr, uint16_t lightPtr = UINT16_MAX, uint32_t alphaTexPtr = UINT32_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.vn0 = ns0; face.vn1 = ns1; face.vn2 = ns2;
        face.alphaTexPtr = alphaTexPtr;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
    
    static Face make_P_N_UV(uint32_t p0, uint32_t p1, uint32_t p2,
                            uint32_t ns0, uint32_t ns1, uint32_t ns2,
                            uint32_t uv0, uint32_t uv1, uint32_t uv2,
                            uint16_t matPtr, uint16_t lightPtr = UINT16_MAX, uint32_t alphaTexPtr = UINT32_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.vn0 = ns0; face.vn1 = ns1; face.vn2 = ns2;
        face.uv0 = uv0; face.uv1 = uv1; face.uv2 = uv2;
        face.alphaTexPtr = alphaTexPtr;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
};

#endif
