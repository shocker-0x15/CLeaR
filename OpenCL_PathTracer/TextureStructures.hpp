//
//  TextureStructures.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/07/14.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_TextureStructures_hpp
#define OpenCL_PathTracer_TextureStructures_hpp

#include <cstdint>

namespace TextureType {
    enum Value {
        ColorConstant = 0,
        ColorImageRGB8x3,
        ColorImageRGBA8x4,
        ColorImageRGBA16Fx4,
        ColorProcedural,
        GrayImage8,
        FloatConstant,
        FloatImage,
        FloatProcedural,
    };
};

namespace ColorProcedureType {
    enum Value {
        CheckerBoard = 0,
        CheckerBoardBump
    };
};

namespace FloatProcedureType {
    enum Value {
        CheckerBoard = 0,
    };
};

namespace FresnelID {
    enum Value {
        NoOp = 0,
        Conductor,
        Dielectric,
    };
};


//32bytes
typedef struct {
    cl_uchar texType; uint8_t dum0[15];
    cl_float3 value;
} Float3ConstantTexture;

//8bytes
typedef struct {
    cl_uchar texType; uint8_t dum0[3];
    cl_float value;
} FloatConstantTexture;

//12bytes
typedef struct {
    cl_uchar texType; uint8_t dum0[3];
    cl_uint width, height;
    cl_int offsetData;
} ImageTexture;

//12bytes
typedef struct {
    cl_uchar texType; uint8_t dum0[3];
    cl_uint width, height;
    cl_int offsetData;
} NormalMapTexture;

//2bytes
typedef struct {
    cl_uchar texType;
    cl_uchar procedureType;
} ProceduralTextureHead;

//48bytes
typedef struct {
    ProceduralTextureHead head; uint8_t dum0[14];
    cl_float3 c[2];
} Float3CheckerBoardTexture;

//12bytes
typedef struct {
    ProceduralTextureHead head; uint8_t dum0[2];
    cl_float width;
    cl_uchar reverse; uint8_t dum1[3];
} Float3CheckerBoardBumpTexture;

//12bytes
typedef struct {
    ProceduralTextureHead head; uint8_t dum0[2];
    cl_float v[2];
} FloatCheckerBoardTexture;


//1bytes
typedef struct {
    cl_uchar fresnelType;
} FresnelHead;

//1bytes
typedef struct {
    FresnelHead head;
} FresnelNoOp;

//48bytes
typedef struct {
    FresnelHead head; uint8_t dum0[15];
    cl_float3 eta, k;
} FresnelConductor;

//12bytes
typedef struct {
    FresnelHead head; uint8_t dum0[3];
    cl_float etaExt, etaInt;
} FresnelDielectric;

#endif
