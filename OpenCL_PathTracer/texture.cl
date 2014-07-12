#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#ifndef device_texture_cl
#define device_texture_cl

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
    ColorProceduralType_CheckerBoard = 0,
    ColorProceduralType_CheckerBoardBump,
} ColorProceduralType;

typedef enum {
    FloatProceduralType_CheckerBoard = 0,
} FloatProceduralType;

color evaluateColorTexture(const global uchar* textureData, float2 uv);
float evaluateFloatTexture(const global uchar* textureData, float2 uv);
float evaluateAlphaTexture(const global uchar* textureData, float2 uv);
color proceduralColorTexture(const global uchar* textureData, float2 uv);
float proceduralFloatTexture(const global uchar* textureData, float2 uv);

//------------------------

color evaluateColorTexture(const global uchar* textureData, float2 uv) {
    uchar textureType = *(textureData++);
    switch (textureType) {
        case TextureType_ColorConstant: {
            return *(const global color*)AlignPtrAddG(&textureData, sizeof(color));
        }
        case TextureType_ColorImageRGB8x3:
        case TextureType_ColorImageRGBA8x4:
        {
            uint width = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint height = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint x = clamp((uint)fmod(width * uv.s0, width), 0u, width - 1);
            uint y = clamp((uint)fmod(height * uv.s1, height), 0u, height - 1);
            AlignPtrG(&textureData, 128);
            uchar3 value = *((const global uchar3*)textureData + y * width + x);
            return convert_float3(value) / 255.0f;
        }
        case TextureType_ColorImageRGBA16Fx4: {
            uint width = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint height = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint x = clamp((uint)fmod(width * uv.s0, width), 0u, width - 1);
            uint y = clamp((uint)fmod(height * uv.s1, height), 0u, height - 1);
            AlignPtrG(&textureData, 128);
            return vload_half4(y * width + x, (const global half*)textureData).xyz;
        }
        case TextureType_ColorProcedural: {
            return proceduralColorTexture(textureData, uv);
        }
        case TextureType_GrayImage8: {
            uint width = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint height = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint x = clamp((uint)fmod(width * uv.s0, width), 0u, width - 1);
            uint y = clamp((uint)fmod(height * uv.s1, height), 0u, height - 1);
            AlignPtrG(&textureData, 128);
            uchar value = *((const global uchar*)textureData + y * width + x);
            return (float3)(value / 255.0f);
        }
        default:
            break;
    }
    
    return colorZero;
}

float evaluateFloatTexture(const global uchar* textureData, float2 uv) {
    uchar textureType = *(textureData++);
    switch (textureType) {
        case TextureType_FloatConstant: {
            return *(const global float*)AlignPtrAddG(&textureData, sizeof(float));
        }
        case TextureType_FloatImage: {
            uint width = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint height = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint x = clamp((uint)fmod(width * uv.s0, width), 0u, width - 1);
            uint y = clamp((uint)fmod(height * uv.s1, height), 0u, height - 1);
            AlignPtrG(&textureData, 128);
            return *((const global float*)textureData + y * width + x);
        }
        case TextureType_FloatProcedural: {
            return proceduralFloatTexture(textureData, uv);
        }
        default:
            break;
    }
    
    return 0.0f;
}

float evaluateAlphaTexture(const global uchar* textureData, float2 uv) {
    uchar textureType = *(textureData++);
    switch (textureType) {
        case TextureType_ColorImageRGBA8x4: {
            uint width = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint height = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint x = clamp((uint)fmod(width * uv.s0, width), 0u, width - 1);
            uint y = clamp((uint)fmod(height * uv.s1, height), 0u, height - 1);
            AlignPtrG(&textureData, 128);
            uchar value = *((const global uchar*)((const global uchar4*)textureData + y * width + x) + 3);
            return value / 255.0f;
        }
        case TextureType_GrayImage8: {
            uint width = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint height = *(const global uint*)AlignPtrAddG(&textureData, sizeof(uint));
            uint x = clamp((uint)fmod(width * uv.s0, width), 0u, width - 1);
            uint y = clamp((uint)fmod(height * uv.s1, height), 0u, height - 1);
            AlignPtrG(&textureData, 128);
            return *((const global uchar*)textureData + y * width + x) / 255.0f;
        }
        case TextureType_FloatProcedural: {
            return proceduralFloatTexture(textureData, uv);
        }
        default:
            break;
    }
    
    return 1.0f;
}

color proceduralColorTexture(const global uchar* textureData, float2 uv) {
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
            if (reverse) {
                uComp *= -1;
                vComp *= -1;
            }
            
            return 0.5f * normalize((vector3)(uComp, vComp, 1.0f)) + 0.5f;
        }
        default:
            return colorZero;
    }
}

float proceduralFloatTexture(const global uchar* textureData, float2 uv) {
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

#endif
