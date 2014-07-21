#ifndef sim_texture_cl
#define sim_texture_cl

#include "sim_global.hpp"
#include "sim_texture_structures.hpp"

namespace sim {
    color evaluateColorTexture(const uchar* textureData, float2 uv);
    float evaluateFloatTexture(const uchar* textureData, float2 uv);
    float evaluateAlphaTexture(const uchar* textureData, float2 uv);
    color proceduralColorTexture(const ProceduralTextureHead* textureData, float2 uv);
    float proceduralFloatTexture(const ProceduralTextureHead* textureData, float2 uv);
    
    //------------------------
    
    color evaluateColorTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *textureData;
        switch (textureType) {
            case TextureType_ColorConstant: {
                const Float3ConstantTexture* f3Const = (const Float3ConstantTexture*)textureData;
                return f3Const->value;
            }
            case TextureType_ColorImageRGB8x3:
            case TextureType_ColorImageRGBA8x4:
            {
                const ImageTexture* image = (const ImageTexture*)textureData;
                uint x = clamp((uint)fmodf(image->width * uv.s0, image->width), 0u, image->width - 1);
                uint y = clamp((uint)fmodf(image->height * uv.s1, image->height), 0u, image->height - 1);
                textureData += image->offsetData;
                uchar3 value = *((const uchar3*)textureData + y * image->width + x);
                return convert_float3(value) / 255.0f;
            }
            case TextureType_ColorImageRGBA16Fx4: {
                const ImageTexture* image = (const ImageTexture*)textureData;
                uint x = clamp((uint)fmodf(image->width * uv.s0, image->width), 0u, image->width - 1);
                uint y = clamp((uint)fmodf(image->height * uv.s1, image->height), 0u, image->height - 1);
                textureData += image->offsetData;
                float4 f4Val = vload_half4(y * image->width + x, (const half*)textureData);
                return sw3(f4Val, x, y, z);
            }
            case TextureType_ColorProcedural: {
                return proceduralColorTexture((const ProceduralTextureHead*)textureData, uv);
            }
            case TextureType_GrayImage8: {
                const ImageTexture* image = (const ImageTexture*)textureData;
                uint x = clamp((uint)fmodf(image->width * uv.s0, image->width), 0u, image->width - 1);
                uint y = clamp((uint)fmodf(image->height * uv.s1, image->height), 0u, image->height - 1);
                textureData += image->offsetData;
                uchar value = *((const uchar*)textureData + y * image->width + x);
                return float3(value / 255.0f);
            }
            default:
                break;
        }
        
        return colorZero;
    }
    
    float evaluateFloatTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *textureData;
        switch (textureType) {
            case TextureType_FloatConstant: {
                const FloatConstantTexture* fConst = (const FloatConstantTexture*)textureData;
                return fConst->value;
            }
            case TextureType_FloatImage: {
                const ImageTexture* image = (const ImageTexture*)textureData;
                uint x = clamp((uint)fmodf(image->width * uv.s0, image->width), 0u, image->width - 1);
                uint y = clamp((uint)fmodf(image->height * uv.s1, image->height), 0u, image->height - 1);
                textureData += image->offsetData;
                return *((const float*)textureData + y * image->width + x);
            }
            case TextureType_FloatProcedural: {
                return proceduralFloatTexture((const ProceduralTextureHead*)textureData, uv);
            }
            default:
                break;
        }
        
        return 0.0f;
    }
    
    float evaluateAlphaTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *textureData;
        switch (textureType) {
            case TextureType_ColorImageRGBA8x4: {
                const ImageTexture* image = (const ImageTexture*)textureData;
                uint x = clamp((uint)fmodf(image->width * uv.s0, image->width), 0u, image->width - 1);
                uint y = clamp((uint)fmodf(image->height * uv.s1, image->height), 0u, image->height - 1);
                textureData += image->offsetData;
                uchar value = *((const uchar*)((const uchar4*)textureData + y * image->width + x) + 3);
                return value / 255.0f;
            }
            case TextureType_GrayImage8: {
                const ImageTexture* image = (const ImageTexture*)textureData;
                uint x = clamp((uint)fmodf(image->width * uv.s0, image->width), 0u, image->width - 1);
                uint y = clamp((uint)fmodf(image->height * uv.s1, image->height), 0u, image->height - 1);
                textureData += image->offsetData;
                return *((const uchar*)textureData + y * image->width + x) / 255.0f;
            }
            case TextureType_FloatProcedural: {
                return proceduralFloatTexture((const ProceduralTextureHead*)textureData, uv);
            }
            default:
                break;
        }
        
        return 1.0f;
    }
    
    color proceduralColorTexture(const ProceduralTextureHead* textureData, float2 uv) {
        switch (textureData->procedureType) {
            case ColorProceduralType_CheckerBoard: {
                const Float3CheckerBoardTexture* f3Checker = (const Float3CheckerBoardTexture*)textureData;
                return f3Checker->c[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
            }
            case ColorProceduralType_CheckerBoardBump: {
                const Float3CheckerBoardBumpTexture* f3CheckerBump = (const Float3CheckerBoardBumpTexture*)textureData;
                float halfWidth = f3CheckerBump->width * 0.5f;
                
                float uComp = 0.0f;
                float absWrapU = fmodf(fabsf(uv.s0), 1.0f);
                if (absWrapU < halfWidth * 0.5f || absWrapU > 1.0f - halfWidth * 0.5f)
                    uComp = 1.0f;
                else if (absWrapU > 0.5f - halfWidth * 0.5f && absWrapU < 0.5f + halfWidth * 0.5f)
                    uComp = -1.0f;
                
                float vComp = 0.0f;
                float absWrapV = fmodf(fabsf(uv.s1), 1.0f);
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
                
                return 0.5f * normalize(vector3(uComp, vComp, 1.0f)) + 0.5f;
            }
            default:
                return colorZero;
        }
    }
    
    float proceduralFloatTexture(const ProceduralTextureHead* textureData, float2 uv) {
        switch (textureData->procedureType) {
            case FloatProceduralType_CheckerBoard: {
                const FloatCheckerBoardTexture* fChecker = (const FloatCheckerBoardTexture*)textureData;
                return fChecker->v[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
            }
            default:
                return 0.0f;
        }
    }
}

#endif
