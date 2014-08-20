//
//  texture.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#ifndef device_texture_cl
#define device_texture_cl

#include "global.cl"
#include "texture_structures.cl"

constant uint pseeds[] = {
    100,  86, 165, 128, 122,   3,  56, 127,  55, 180, 198,  14, 178, 176,  24,  16,
    210, 161,  34, 217,  78, 242, 160, 241, 109,  48, 248,  89, 136,  58, 170,  85,
    125, 179,  75,  30, 145,  84, 194, 193, 131,  82, 184, 250, 246, 143, 104, 228,
    251,  81, 158, 208,  52, 245,  25, 197, 224, 232, 141, 139, 182,   8,  77,  47,
    230, 171,  91,  10, 212, 151,  23,  93,  32, 255, 199, 159,  18,   9,  41, 119,
    147, 138, 195,  66, 172, 121, 129,  11,  43, 120, 162, 133, 186, 177, 202, 105,
    225, 218, 103, 142, 130, 244, 206, 189,  96,  64,  95, 163, 216,  67, 111,  83,
    118, 124, 188,   1,  63,  33, 205,  98, 169, 213,  39, 254, 196, 148,  21, 192,
     60, 140, 167,  44,  99,  13, 126,  72, 168, 123,  12,  31, 203, 220, 134, 164,
     88,   0, 223, 153,  40, 173, 215, 207,  69, 106, 201,  57,  38, 107,  29,  62,
    146,  71, 243,  15, 183,   2,  90,  45, 113,  49,   4,  17, 115, 211, 237,  20,
    227,  97,  50, 234, 219, 229, 152, 226, 190, 181, 101,  68,  27,  87, 135, 236,
    214,  59,  61, 209, 187,  74, 156, 137,  42, 240, 253, 233,  28,  19,  36, 238,
    204,  22,  79,  70,  37,  26, 157, 154,   6,  76, 155, 110,  80, 117, 132,  73,
    112,  92, 191, 144,  46, 114,  35, 239,  65,   7, 200, 102, 235, 150, 185, 108,
    249, 221, 149, 231,  51,  94, 175, 247, 166, 252,   5, 222, 174,  53, 116,  54,
    
    100,  86, 165, 128, 122,   3,  56, 127,  55, 180, 198,  14, 178, 176,  24,  16,
    210, 161,  34, 217,  78, 242, 160, 241, 109,  48, 248,  89, 136,  58, 170,  85,
    125, 179,  75,  30, 145,  84, 194, 193, 131,  82, 184, 250, 246, 143, 104, 228,
    251,  81, 158, 208,  52, 245,  25, 197, 224, 232, 141, 139, 182,   8,  77,  47,
    230, 171,  91,  10, 212, 151,  23,  93,  32, 255, 199, 159,  18,   9,  41, 119,
    147, 138, 195,  66, 172, 121, 129,  11,  43, 120, 162, 133, 186, 177, 202, 105,
    225, 218, 103, 142, 130, 244, 206, 189,  96,  64,  95, 163, 216,  67, 111,  83,
    118, 124, 188,   1,  63,  33, 205,  98, 169, 213,  39, 254, 196, 148,  21, 192,
    60, 140, 167,  44,  99,  13, 126,  72, 168, 123,  12,  31, 203, 220, 134, 164,
    88,   0, 223, 153,  40, 173, 215, 207,  69, 106, 201,  57,  38, 107,  29,  62,
    146,  71, 243,  15, 183,   2,  90,  45, 113,  49,   4,  17, 115, 211, 237,  20,
    227,  97,  50, 234, 219, 229, 152, 226, 190, 181, 101,  68,  27,  87, 135, 236,
    214,  59,  61, 209, 187,  74, 156, 137,  42, 240, 253, 233,  28,  19,  36, 238,
    204,  22,  79,  70,  37,  26, 157, 154,   6,  76, 155, 110,  80, 117, 132,  73,
    112,  92, 191, 144,  46, 114,  35, 239,  65,   7, 200, 102, 235, 150, 185, 108,
    249, 221, 149, 231,  51,  94, 175, 247, 166, 252,   5, 222, 174,  53, 116,  54,
};

color evaluateColorTexture(const global uchar* textureData, float2 uv);
float evaluateFloatTexture(const global uchar* textureData, float2 uv);
float evaluateAlphaTexture(const global uchar* textureData, float2 uv);
color proceduralColorTexture(const global ProceduralTextureHead* textureData, float2 uv);
float proceduralFloatTexture(const global ProceduralTextureHead* textureData, float2 uv);

//------------------------

color evaluateColorTexture(const global uchar* textureData, float2 uv) {
    uchar textureType = *textureData;
    switch (textureType) {
        case TextureType_ColorConstant: {
            const global Float3ConstantTexture* f3Const = (const global Float3ConstantTexture*)textureData;
            return f3Const->value;
        }
        case TextureType_ColorImageRGB8x3:
        case TextureType_ColorImageRGBA8x4:
        {
            const global ImageTexture* image = (const global ImageTexture*)textureData;
            uint x = clamp((uint)fmod(image->width * uv.s0, image->width), 0u, image->width - 1);
            uint y = clamp((uint)fmod(image->height * uv.s1, image->height), 0u, image->height - 1);
            textureData += image->offsetData;
            uchar3 value = *((const global uchar3*)textureData + y * image->width + x);
            return convert_float3(value) / 255.0f;
        }
        case TextureType_ColorImageRGBA16Fx4: {
            const global ImageTexture* image = (const global ImageTexture*)textureData;
            uint x = clamp((uint)fmod(image->width * uv.s0, image->width), 0u, image->width - 1);
            uint y = clamp((uint)fmod(image->height * uv.s1, image->height), 0u, image->height - 1);
            textureData += image->offsetData;
            return vload_half4(y * image->width + x, (const global half*)textureData).xyz;
        }
        case TextureType_ColorProcedural: {
            return proceduralColorTexture((const global ProceduralTextureHead*)textureData, uv);
        }
        case TextureType_GrayImage8: {
            const global ImageTexture* image = (const global ImageTexture*)textureData;
            uint x = clamp((uint)fmod(image->width * uv.s0, image->width), 0u, image->width - 1);
            uint y = clamp((uint)fmod(image->height * uv.s1, image->height), 0u, image->height - 1);
            textureData += image->offsetData;
            uchar value = *((const global uchar*)textureData + y * image->width + x);
            return (float3)(value / 255.0f);
        }
        default:
            break;
    }
    
    return colorZero;
}

float evaluateFloatTexture(const global uchar* textureData, float2 uv) {
    uchar textureType = *textureData;
    switch (textureType) {
        case TextureType_FloatConstant: {
            const global FloatConstantTexture* fConst = (const global FloatConstantTexture*)textureData;
            return fConst->value;
        }
        case TextureType_FloatImage: {
            const global ImageTexture* image = (const global ImageTexture*)textureData;
            uint x = clamp((uint)fmod(image->width * uv.s0, image->width), 0u, image->width - 1);
            uint y = clamp((uint)fmod(image->height * uv.s1, image->height), 0u, image->height - 1);
            textureData += image->offsetData;
            return *((const global float*)textureData + y * image->width + x);
        }
        case TextureType_FloatProcedural: {
            return proceduralFloatTexture((const global ProceduralTextureHead*)textureData, uv);
        }
        default:
            break;
    }
    
    return 0.0f;
}

float evaluateAlphaTexture(const global uchar* textureData, float2 uv) {
    uchar textureType = *textureData;
    switch (textureType) {
        case TextureType_ColorImageRGBA8x4: {
            const global ImageTexture* image = (const global ImageTexture*)textureData;
            uint x = clamp((uint)fmod(image->width * uv.s0, image->width), 0u, image->width - 1);
            uint y = clamp((uint)fmod(image->height * uv.s1, image->height), 0u, image->height - 1);
            textureData += image->offsetData;
            uchar value = *((const global uchar*)((const global uchar4*)textureData + y * image->width + x) + 3);
            return value / 255.0f;
        }
        case TextureType_GrayImage8: {
            const global ImageTexture* image = (const global ImageTexture*)textureData;
            uint x = clamp((uint)fmod(image->width * uv.s0, image->width), 0u, image->width - 1);
            uint y = clamp((uint)fmod(image->height * uv.s1, image->height), 0u, image->height - 1);
            textureData += image->offsetData;
            return *((const global uchar*)textureData + y * image->width + x) / 255.0f;
        }
        case TextureType_FloatProcedural: {
            return proceduralFloatTexture((const global ProceduralTextureHead*)textureData, uv);
        }
        default:
            break;
    }
    
    return 1.0f;
}

color proceduralColorTexture(const global ProceduralTextureHead* textureData, float2 uv) {
    switch (textureData->procedureType) {
        case ColorProcedureType_CheckerBoard: {
            const global Float3CheckerBoardTexture* f3Checker = (const global Float3CheckerBoardTexture*)textureData;
            return f3Checker->c[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
        }
        case ColorProcedureType_CheckerBoardBump: {
            const global Float3CheckerBoardBumpTexture* f3CheckerBump = (const global Float3CheckerBoardBumpTexture*)textureData;
            float halfWidth = f3CheckerBump->width * 0.5f;
            
            float uComp = 0.0f;
            float absWrapU = fmod(fabs(uv.s0), 1.0f);
            if (absWrapU < halfWidth * 0.5f || absWrapU > 1.0f - halfWidth * 0.5f)
                uComp = 1.0f;
            else if (absWrapU > 0.5f - halfWidth * 0.5f && absWrapU < 0.5f + halfWidth * 0.5f)
                uComp = -1.0f;
            
            float vComp = 0.0f;
            float absWrapV = fmod(fabs(uv.s1), 1.0f);
            if (absWrapV < halfWidth * 0.5f || absWrapV > 1.0f - halfWidth * 0.5f)
                vComp = 1.0f;
            else if (absWrapV > 0.5f - halfWidth * 0.5f && absWrapV < 0.5f + halfWidth * 0.5f)
                vComp = -1.0f;
            
            if (absWrapV > 0.5f)
                uComp *= -1;
            if (absWrapU > 0.5f)
                vComp *= -1;
            if (f3CheckerBump->reverse) {
                uComp *= -1;
                vComp *= -1;
            }
            
            return 0.5f * normalize((vector3)(uComp, vComp, 1.0f)) + 0.5f;
        }
        case ColorProcudureType_Random1: {
            const global Float3Random1Texture* f3Random = (const global Float3Random1Texture*)textureData;
        }
        default:
            return colorZero;
    }
}

float proceduralFloatTexture(const global ProceduralTextureHead* textureData, float2 uv) {
    switch (textureData->procedureType) {
        case FloatProcedureType_CheckerBoard: {
            const global FloatCheckerBoardTexture* fChecker = (const global FloatCheckerBoardTexture*)textureData;
            return fChecker->v[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
        }
        default:
            return 0.0f;
    }
}

#endif
