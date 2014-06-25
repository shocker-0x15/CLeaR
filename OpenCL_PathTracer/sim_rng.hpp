#ifndef sim_rng_cl
#define sim_rng_cl

#include "sim_global.hpp"

namespace sim {
    uint getUInt(uint* randState);
    inline float getFloat0cTo1o(uint* randState);
    inline uint randUInt(float u, uint maxv);
    void concentricSampleDisk(float u1, float u2, float* dx, float* dy);
    vector3 cosineSampleHemisphere(float u1, float u2);
    void uniformSampleTriangle(float u1, float u2, float* b0, float* b1);
    uint sampleDiscrete1D(const uchar* CDF1D, float u, float* prob);
    
    //------------------------
    
    uint getUInt(uint* randState) {
        uint t = randState[0] ^ (randState[0] << 11);
        randState[0] = randState[1];
        randState[1] = randState[2];
        randState[2] = randState[3];
        return randState[3] = (randState[3] ^ (randState[3] >> 19)) ^ (t ^ (t >> 8));
    }
    
    inline float getFloat0cTo1o(uint* randState) {
        uint rand23bit = (getUInt(randState) >> 9) | 0x3f800000;
        return *(float*)&rand23bit - 1.0f;
    }
    
    inline uint randUInt(float u, uint maxv) {
        return std::min((uint)(maxv * u), maxv - 1);
    }
    
    //"A Low Distortion Map Between Disk and Square"
    void concentricSampleDisk(float u1, float u2, float* dx, float* dy) {
        float r, theta;
        float sx = 2 * u1 - 1;
        float sy = 2 * u2 - 1;
        
        if (sx == 0.0f && sy == 0.0f) {
            *dx = 0.0f;
            *dy = 0.0f;
            return;
        }
        if (sx >= -sy) { // region 1 or 2
            if (sx > sy) { // region 1
                r = sx;
                theta = sy / sx;
            }
            else { // region 2
                r = sy;
                theta = 2.0f - sx / sy;
            }
        }
        else { // region 3 or 4
            if (sx > sy) {/// region 4
                r = -sy;
                theta = 6.0f + sx / sy;
            }
            else {// region 3
                r = -sx;
                theta = 4.0f + sy / sx;
            }
        }
        theta *= M_PI_F / 4.0f;
        *dx = r * cosf(theta);
        *dy = r * sinf(theta);
    }
    
    vector3 cosineSampleHemisphere(float u1, float u2) {
        float x, y;
        concentricSampleDisk(u1, u2, &x, &y);
        return float3(x, y, sqrtf(fmaxf(0.0f, 1.0f - x * x - y * y)));
    }
    
    void uniformSampleTriangle(float u1, float u2, float* b0, float* b1) {
        float su1 = sqrtf(u1);
        *b0 = 1.0f - su1;
        *b1 = u2 * su1;
    }
    
    uint sampleDiscrete1D(const uchar* CDF1D, float u, float* prob) {
        uint numElems = *(const uint*)AlignPtrAddG(&CDF1D, sizeof(uint));
        for (uint i = 0; i < numElems; ++i) {
            if (*((const float*)CDF1D + i) > u) {
                *prob = *((const float*)CDF1D + i + numElems);
                return i;
            }
        }
        return UINT_MAX;
    }
}

#endif