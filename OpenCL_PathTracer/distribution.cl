//
//  distribution.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef device_distribution_cl
#define device_distribution_cl

#include "global.cl"

typedef enum {
    DistributionType_Discrete1D = 0,
    DistributionType_ContinuousConsts1D,
    DistributionType_ContinuousConsts2D_H,
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

//28bytes
typedef struct __attribute__((aligned(4))) {
    DistributionHead head;
    uint numValues __attribute__((aligned(4)));
    float startDomain, endDomain;
    float widthStratum;
    int offsetPDF;
    int offsetCDF;
} ContinuousConsts1D;

//36bytes
typedef struct __attribute__((aligned(4))) {
    DistributionHead head;
    int offsetChildren __attribute__((aligned(4)));
    ContinuousConsts1D distParent;
} ContinuousConsts2D_H;

//------------------------

uint sampleDiscrete1D(const global Discrete1D* dist, float u, float* prob);
inline float probDiscrete1D(const global Discrete1D* dist, uint num);
float sampleContinuousConsts1D(const global ContinuousConsts1D* dist, float u, float* PDF);
inline float PDFContinuousConsts1D(const global ContinuousConsts1D* dist, float point);
float2 sampleContinuousConsts2D_H(const global ContinuousConsts2D_H* dist, float u0, float u1, float* PDF);
float PDFContinuousConsts2D_H(const global ContinuousConsts2D_H* dist, const float2* p);

//------------------------

uint sampleDiscrete1D(const global Discrete1D* dist, float u, float* prob) {
    const global float* CDF = convertPtrCG(float, dist, dist->offsetCDF);
    for (uint i = 0; i < dist->numItems; ++i) {
        if (*(CDF + i) > u) {
            *prob = *(convertPtrCG(float, dist, dist->offsetPMF) + i);
            return i;
        }
    }
    *prob = 0.0f;
    return UINT_MAX;
}

inline float probDiscrete1D(const global Discrete1D* dist, uint num) {
    return *(convertPtrCG(float, dist, dist->offsetPMF) + num);
}

float sampleContinuousConsts1D(const global ContinuousConsts1D* dist, float u, float* PDF) {
    const global float* CDF = convertPtrCG(float, dist, dist->offsetCDF);
//    for (uint i = 0; i < dist->numValues; ++i) {
//        if (*(CDF + i + 1) > u) {
//            float t = (u - *(CDF + i)) / (*(CDF + i + 1) - *(CDF + i));
//            *PDF = *(convertPtrCG(float, dist, dist->offsetPDF) + i);
//            return dist->startDomain + (i + t) * dist->widthStratum;
//        }
//    }
//    *PDF = 0.0f;
//    return 0.0f;
    
    uint start = 0;
    uint end = dist->numValues;
    uint i = dist->numValues / 2;
    while (true) {
        if (*(CDF + i) > u)
            end = i;
        else
            start = i;
        
        if (end - start == 1) {
            float t = (u - *(CDF + start)) / (*(CDF + end) - *(CDF + start));
            *PDF = *(convertPtrCG(float, dist, dist->offsetPDF) + start);
            return dist->startDomain + (start + t) * dist->widthStratum;
        }
        
        i = (end - start) / 2 + start;
    }
    *PDF = 0.0f;
    return 0.0f;
}

inline float PDFContinuousConsts1D(const global ContinuousConsts1D* dist, float point) {
    return *(convertPtrCG(float, dist, dist->offsetPDF) +
             (uint)((point - dist->startDomain) / dist->widthStratum));//min(*, dist->numValues - 1)でクランプしたほうが良いかも。
}

float2 sampleContinuousConsts2D_H(const global ContinuousConsts2D_H* dist, float u0, float u1, float* PDF) {
    float2 ret;
    float PDFTop;
    ret.s1 = sampleContinuousConsts1D(&dist->distParent, u1, &PDFTop);
    uint idxChild = (uint)((ret.s1 - dist->distParent.startDomain) / dist->distParent.widthStratum);
    ret.s0 = sampleContinuousConsts1D(convertPtrCG(ContinuousConsts1D, dist, dist->offsetChildren) + idxChild, u0, PDF);
    *PDF *= PDFTop;
    return ret;
}

float PDFContinuousConsts2D_H(const global ContinuousConsts2D_H* dist, const float2* p) {
    float ret;
    ret = PDFContinuousConsts1D(&dist->distParent, p->s1);
    uint idxChild = (uint)((p->s1 - dist->distParent.startDomain) / dist->distParent.widthStratum);
    ret *= PDFContinuousConsts1D(convertPtrCG(ContinuousConsts1D, dist, dist->offsetChildren) + idxChild, p->s0);
    return ret;
}

#endif
