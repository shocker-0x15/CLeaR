#ifndef sim_texture_cl
#define sim_texture_cl

#include "sim_global.h"

namespace sim {
    color evaluateColorTexture(const uchar* textureData, float2 uv);
    float evaluateFloatTexture(const uchar* textureData, float2 uv);
    color proceduralTexture(const uchar* textureData, float2 uv);
    
    color evaluateColorTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *(textureData++);
        switch (textureType) {
            case 0: {
                return *(color*)AlignPtrAddG(&textureData, sizeof(color));
            }
            case 1: {
                uint width = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint height = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint x = clamp((uint)fmodf(width * uv.s0, width), 0u, width - 1);
                uint y = clamp((uint)fmodf(height * uv.s1, height), 0u, height - 1);
                uchar3 value = *((uchar3*)textureData + y * width + x);
                return convert_float3(value) / 255.0f;
            }
            case 2: {
                return proceduralTexture(textureData, uv);
            }
            default:
                break;
        }
        
        return color(0.0f, 0.0f, 0.0f);
    }
    
    float evaluateFloatTexture(const uchar* textureData, float2 uv) {
        uchar textureType = *(textureData++);
        switch (textureType) {
            case 10: {
                return *(float*)AlignPtrAddG(&textureData, sizeof(float));
            }
            case 11: {
                uint width = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint height = *(uint*)AlignPtrAddG(&textureData, sizeof(uint));
                uint x = clamp((uint)fmodf(width * uv.s0, width), 0u, width - 1);
                uint y = clamp((uint)fmodf(height * uv.s1, height), 0u, height - 1);
                return *((float*)textureData + y * width + x);
            }
            case 12: {
                break;
            }
            default:
                break;
        }
        
        return 0.0f;
    }
    
    color proceduralTexture(const uchar* textureData, float2 uv) {
        uchar procedure = *(textureData++);
        if (procedure == 0) {
            color c[2];
            memcpyG2P((uchar*)&c[0], AlignPtrAddG(&textureData, sizeof(color)), sizeof(color));
            memcpyG2P((uchar*)&c[1], AlignPtrAddG(&textureData, sizeof(color)), sizeof(color));
            
            return c[((uint)(uv.s0 * 2) + (uint)(uv.s1 * 2)) % 2];
        }
        
        return color(0.0f, 0.0f, 0.0f);
    }
    
}

#endif