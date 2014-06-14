#ifndef sim_texture_cl
#define sim_texture_cl

#include "sim_global.hpp"

namespace sim {
    typedef enum {
        TextureType_ColorConstant = 0,
        TextureType_ColorImage,
        TextureType_ColorProcedural,
        TextureType_FloatConstant,
        TextureType_FloatImage,
        TextureType_FloatProcedural,
    } TextureType;
    
    typedef enum {
        ColorProceduralType_CheckerBoard = 0,
        ColorProceduralType_CheckerBoardBump,
    } ColorProceduralType;
    
    typedef enum {
        FloatProceduralType_CheckerBoard = 0,
    } FloatProceduralType;
    
    color evaluateColorTexture(const uchar* textureData, float2 uv);
    float evaluateFloatTexture(const uchar* textureData, float2 uv);
    color proceduralColorTexture(const uchar* textureData, float2 uv);
    float proceduralFloatTexture(const uchar* textureData, float2 uv);
    
    //------------------------
    
    color evaluateColorTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *(textureData++);
        switch (textureType) {
            case TextureType_ColorConstant: {
                return *(color*)AlignPtrAddG(&textureData, sizeof(color));
            }
            case TextureType_ColorImage: {
                uint width = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint height = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint x = clamp((uint)fmodf(width * uv.s0, width), 0u, width - 1);
                uint y = clamp((uint)fmodf(height * uv.s1, height), 0u, height - 1);
                uchar3 value = *((uchar3*)textureData + y * width + x);
                return convert_float3(value) / 255.0f;
            }
            case TextureType_ColorProcedural: {
                return proceduralColorTexture(textureData, uv);
            }
            default:
            break;
        }
        
        return colorZero;
    }
    
    float evaluateFloatTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *(textureData++);
        switch (textureType) {
            case TextureType_FloatConstant: {
                return *(float*)AlignPtrAddG(&textureData, sizeof(float));
            }
            case TextureType_FloatImage: {
                uint width = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint height = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint x = clamp((uint)fmodf(width * uv.s0, width), 0u, width - 1);
                uint y = clamp((uint)fmodf(height * uv.s1, height), 0u, height - 1);
                return *((float*)textureData + y * width + x);
            }
            case TextureType_FloatProcedural: {
                return proceduralFloatTexture(textureData, uv);
            }
            default:
            break;
        }
        
        return 0.0f;
    }
    
    color proceduralColorTexture(const uchar* textureData, float2 uv) {
        uchar procedure = *(textureData++);
        switch (procedure) {
            case ColorProceduralType_CheckerBoard: {
                color c[2];
                memcpyG2P((uchar*)&c[0], AlignPtrAddG(&textureData, sizeof(color)), sizeof(color));
                memcpyG2P((uchar*)&c[1], AlignPtrAddG(&textureData, sizeof(color)), sizeof(color));
                
                return c[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
            }
            case ColorProceduralType_CheckerBoardBump: {
                float halfWidth;
                bool reverse;
                memcpyG2P((uchar*)&halfWidth, AlignPtrAddG(&textureData, sizeof(float)), sizeof(float));
                halfWidth *= 0.5f;
                memcpyG2P((uchar*)&reverse, AlignPtrAddG(&textureData, sizeof(bool)), sizeof(bool));
                
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
                if (reverse) {
                    uComp *= -1;
                    vComp *= -1;
                }
                
                return 0.5f * normalize(vector3(uComp, vComp, 1.0f)) + 0.5f;
            }
            default:
            return colorZero;
        }
    }
    
    float proceduralFloatTexture(const uchar* textureData, float2 uv) {
        uchar procedure = *(textureData++);
        switch (procedure) {
            case FloatProceduralType_CheckerBoard: {
                float v[2];
                memcpyG2P((uchar*)&v[0], AlignPtrAddG(&textureData, sizeof(float)), sizeof(float));
                memcpyG2P((uchar*)&v[1], AlignPtrAddG(&textureData, sizeof(float)), sizeof(float));
                
                return v[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
            }
            default:
            return 0.0f;
        }
    }
}

#endif