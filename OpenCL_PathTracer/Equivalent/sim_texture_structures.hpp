//
//  sim_texture_structures.hpp
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef sim_texture_structures_cl
#define sim_texture_structures_cl

#include "sim_global.hpp"

namespace sim {
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
    typedef struct {
        uchar texType; uchar dum0[15];
        float3 value;
    } Float3ConstantTexture;
    
    //8bytes
    typedef struct {
        uchar texType; uchar dum0[3];
        float value;
    } FloatConstantTexture;
    
    //16bytes
    typedef struct {
        uchar texType; uchar dum0[3];
        uint width, height;
        int offsetData;
    } ImageTexture;
    
    //16bytes
    typedef struct {
        uchar texType; uchar dum0[3];
        uint width, height;
        int offsetData;
    } NormalMapTexture;
    
    //2bytes
    typedef struct {
        uchar texType;
        uchar procedureType;
    } ProceduralTextureHead;
    
    //48bytes
    typedef struct {
        ProceduralTextureHead head; uchar dum0[14];
        float3 c[2];
    } Float3CheckerBoardTexture;
    
    //12bytes
    typedef struct {
        ProceduralTextureHead head; uchar dum0[2];
        float width;
        uchar reverse; uchar dum1[3];
    } Float3CheckerBoardBumpTexture;
    
    //12bytes
    typedef struct {
        ProceduralTextureHead head; uchar dum0[2];
        float v[2];
    } FloatCheckerBoardTexture;
    
    
    //1bytes
    typedef struct {
        uchar fresnelType;
    } FresnelHead;
    
    //1bytes
    typedef struct {
        FresnelHead head;
    } FresnelNoOp;
    
    //48bytes
    typedef struct {
        FresnelHead head; uchar dum0[15];
        float3 eta, k;
    } FresnelConductor;
    
    //12bytes
    typedef struct {
        FresnelHead head; uchar dum0[3];
        float etaExt, etaInt;
    } FresnelDielectric;
}

#endif
