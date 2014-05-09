//
//  Face.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Face_hpp
#define OpenCL_PathTracer_Face_hpp

#include <stdint.h>

struct Face {
    uint32_t p0, p1, p2;
    uint32_t ns0, ns1, ns2;
    uint32_t uv0, uv1, uv2;
    uint16_t matPtr, lightPtr; uint8_t dum[8];
    
    Face() :
    p0(UINT32_MAX), p1(UINT32_MAX), p2(UINT32_MAX),
    ns0(UINT32_MAX), ns1(UINT32_MAX), ns2(UINT32_MAX),
    uv0(UINT32_MAX), uv1(UINT32_MAX), uv2(UINT32_MAX),
    matPtr(UINT16_MAX), lightPtr(UINT16_MAX) { };
    
    static Face make_p(uint32_t p0, uint32_t p1, uint32_t p2, uint16_t matPtr, uint16_t lightPtr = UINT16_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
    
    static Face make_pt(uint32_t p0, uint32_t p1, uint32_t p2,
                        uint32_t uv0, uint32_t uv1, uint32_t uv2,
                        uint16_t matPtr, uint16_t lightPtr = UINT16_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.uv0 = uv0; face.uv1 = uv1; face.uv2 = uv2;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
    
    static Face make_pn(uint32_t p0, uint32_t p1, uint32_t p2,
                        uint32_t ns0, uint32_t ns1, uint32_t ns2,
                        uint16_t matPtr, uint16_t lightPtr = UINT16_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.ns0 = ns0; face.ns1 = ns1; face.ns2 = ns2;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
    
    static Face make_pnt(uint32_t p0, uint32_t p1, uint32_t p2,
                         uint32_t ns0, uint32_t ns1, uint32_t ns2,
                         uint32_t uv0, uint32_t uv1, uint32_t uv2,
                         uint16_t matPtr, uint16_t lightPtr = UINT16_MAX) {
        Face face;
        face.p0 = p0; face.p1 = p1; face.p2 = p2;
        face.ns0 = ns0; face.ns1 = ns1; face.ns2 = ns2;
        face.uv0 = uv0; face.uv1 = uv1; face.uv2 = uv2;
        face.matPtr = matPtr; face.lightPtr = lightPtr;
        return face;
    }
};

#endif
