//
//  Materials.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/06/15.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Materials_hpp
#define OpenCL_PathTracer_Materials_hpp

#include <cstdint>

//8bytes
typedef struct {
    uint8_t numBxDFs;
    uint8_t hasBump; uint8_t dum0[2];
    uint32_t idx_bump;
} MaterialInfo;

//12bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_R;
    uint32_t idx_sigma;
} DiffuseBRDFInfo;

//12bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_R;
    uint32_t idx_Fresnel;
} SpecularBRDFInfo;

//20bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_T;
    float etaExt;
    float etaInt;
    uint32_t idx_Fresnel;
} SpecularBTDFInfo;

//16bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_R;
    uint32_t idx_anisoX;
    uint32_t idx_anisoY;
} NewWardBRDFInfo;

//16bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_Rs;
    uint32_t idx_nu;
    uint32_t idx_nv;
} AshikhminSBRDFInfo;

//12bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_Rs;
    uint32_t idx_Rd;
} AshikhminDBRDFInfo;


//1bytes
typedef struct {
    uint8_t numEEDFs;
} LightPropertyInfo;

//8bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_M;
} DiffuseEDFInfo;

#endif
