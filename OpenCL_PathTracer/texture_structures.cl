//
//  texture_structures.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef device_texture_structures_cl
#define device_texture_structures_cl

#include "global.cl"

typedef enum {
    TextureType_ColorConstant = 0,
    TextureType_ColorImageRGB8x3,
    TextureType_ColorImageRGBA8x4,
    TextureType_ColorImageRGBA16Fx4,
    TextureType_ColorProcedural,
    TextureType_GrayImage8,
    TextureType_FloatConstant,
    TextureType_FloatImage,
    TextureType_FloatProcedural,
} TextureType;

typedef enum {
    ColorProcedureType_CheckerBoard = 0,
    ColorProcedureType_CheckerBoardBump,
    ColorProcudureType_Random1,//仮の名前
} ColorProcedureType;

typedef enum {
    FloatProcedureType_CheckerBoard = 0,
} FloatProcedureType;

typedef enum {
    FresnelID_NoOp = 0,
    FresnelID_Conductor,
    FresnelID_Dielectric,
} FresnelID;


//32bytes
typedef struct __attribute__((aligned(16))) {
    uchar texType;
    float3 value __attribute__((aligned(16)));
} Float3ConstantTexture;

//8bytes
typedef struct __attribute__((aligned(4))) {
    uchar texType;
    float value __attribute__((aligned(4)));
} FloatConstantTexture;

//16bytes
typedef struct __attribute__((aligned(4))) {
    uchar texType;
    uint width __attribute__((aligned(4))), height;
    int offsetData;
} ImageTexture;

//16bytes
typedef struct __attribute__((aligned(4))) {
    uchar texType;
    uint width __attribute__((aligned(4))), height;
    int offsetData;
} NormalMapTexture;

//2bytes
typedef struct __attribute__((aligned(1))) {
    uchar texType;
    uchar procedureType;
} ProceduralTextureHead;

//48bytes
typedef struct __attribute__((aligned(16))) {
    ProceduralTextureHead head;
    float3 c[2] __attribute__((aligned(16)));
} Float3CheckerBoardTexture;

//12bytes
typedef struct __attribute__((aligned(4))) {
    ProceduralTextureHead head;
    float width __attribute__((aligned(4)));
    uchar reverse;
} Float3CheckerBoardBumpTexture;

typedef struct __attribute__((aligned(4))) {
    ProceduralTextureHead head;
} Float3Random1Texture;

//12bytes
typedef struct __attribute__((aligned(4))) {
    ProceduralTextureHead head;
    float v[2] __attribute__((aligned(4)));
} FloatCheckerBoardTexture;


//1bytes
typedef struct __attribute__((aligned(1))) {
    uchar fresnelType;
} FresnelHead;

//1bytes
typedef struct __attribute__((aligned(1))) {
    FresnelHead head;
} FresnelNoOp;

//48bytes
typedef struct __attribute__((aligned(16))) {
    FresnelHead head;
    float3 eta __attribute__((aligned(16))), k;
} FresnelConductor;

//12bytes
typedef struct __attribute__((aligned(4))) {
    FresnelHead head;
    float etaExt __attribute__((aligned(4))), etaInt;
} FresnelDielectric;

#endif
