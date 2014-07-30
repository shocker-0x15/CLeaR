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

namespace MatElem {
    enum Value {
        Diffuse = 0,
        SpecularReflection,
        SpecularTransmission,
        NewWard,
        AshikhminS,
        AshikhminD,
    };
};

namespace LPElem {
    enum Value {
        DiffuseEmission = 0,
    };
};

namespace EnvLPElem {
    enum Value {
        ImageBased = 0,
    };
};


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
} DiffuseRElem;

//12bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_R;
    uint32_t idx_Fresnel;
} SpecularRElem;

//20bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_T;
    float etaExt;
    float etaInt;
    uint32_t idx_Fresnel;
} SpecularTElem;

//16bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_R;
    uint32_t idx_anisoX;
    uint32_t idx_anisoY;
} NewWardElem;

//16bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_Rs;
    uint32_t idx_nu;
    uint32_t idx_nv;
} AshikhminSElem;

//12bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_Rs;
    uint32_t idx_Rd;
} AshikhminDElem;


//1bytes
typedef struct {
    uint8_t numEEDFs;
} LightPropertyInfo;

//8bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_M;
} DiffuseLElem;

//16bytes
typedef struct {
    uint8_t id; uint8_t dum0[3];
    uint32_t idx_Le;
    float multiplier;
    uint32_t idx_Dist2D;
} ImageBasedEnvLElem;

#endif
