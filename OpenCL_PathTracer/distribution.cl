#ifndef device_distribution_cl
#define device_distribution_cl

#include "global.cl"

typedef enum {
    DistributionType_Discrete1D = 0,
} DistributionType;

//1byte
typedef struct __attribute__((aligned(1))) {
    uchar _type;
} DistributionHead;

//16bytes
typedef struct __attribute__((aligned(4))) {
    DistributionHead head;
    uint numItems __attribute__((aligned(4)));
    int offsetPMF;
    int offsetCDF;
} Discrete1D;

//------------------------

uint sampleDiscrete1D(const global Discrete1D* dist, float u, float* prob);

//------------------------

uint sampleDiscrete1D(const global Discrete1D* dist, float u, float* prob) {
    const global float* CDF = (const global float*)((const global uchar*)dist + dist->offsetCDF);
    for (uint i = 0; i < dist->numItems; ++i) {
        if (*(CDF + i) > u) {
            *prob = *((const global float*)((const global uchar*)dist + dist->offsetPMF) + i);
            return i;
        }
    }
    return UINT_MAX;
}

float probDiscrete1D(const global Discrete1D* dist, uint num) {
    return *((const global float*)((const global uchar*)dist + dist->offsetPMF) + num);
}

#endif
