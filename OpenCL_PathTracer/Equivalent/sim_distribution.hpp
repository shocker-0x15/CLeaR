//
//  sim_distribution.hpp
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef sim_distribution_cl
#define sim_distribution_cl

#include "sim_global.hpp"

namespace sim {
    typedef enum {
        DistributionType_Discrete1D = 0,
        DistributionType_ContinuousConsts1D,
        DistributionType_ContinuousConsts2D_H,
    } DistributionType;
    
    //1byte
    typedef struct {
        uchar _type;
    } DistributionHead;
    
    //16bytes
    typedef struct {
        DistributionHead head; uchar dum0[3];
        uint numItems;
        int offsetPMF;
        int offsetCDF;
    } Discrete1D;
    
    //28bytes
    typedef struct {
        DistributionHead head; uchar dum0[3];
        uint numValues;
        float startDomain, endDomain;
        float widthStratum;
        int offsetPDF;
        int offsetCDF;
    } ContinuousConsts1D;
    
    //36bytes
    typedef struct {
        DistributionHead head; uchar dum0[3];
        int offsetChildren;
        ContinuousConsts1D distParent;
    } ContinuousConsts2D_H;
    
    //------------------------
    
    uint sampleDiscrete1D(const Discrete1D* dist, float u, float* prob);
    inline float probDiscrete1D(const Discrete1D* dist, uint num);
    float sampleContinuousConsts1D(const ContinuousConsts1D* dist, float u, float* PDF);
    inline float PDFContinuousConsts1D(const ContinuousConsts1D* dist, float point);
    float2 sampleContinuousConsts2D_H(const ContinuousConsts2D_H* dist, float u0, float u1, float* PDF);
    float PDFContinuousConsts2D_H(const ContinuousConsts2D_H* dist, const float2* p);
    
    //------------------------
    
    uint sampleDiscrete1D(const Discrete1D* dist, float u, float* prob) {
        const float* CDF = convertPtrCG(float, dist, dist->offsetCDF);
        uint start = 0;
        uint end = dist->numItems + 1;
        uint i = end / 2;
        while (true) {
            if (*(CDF + i) > u)
                end = i;
            else
                start = i;
            
            if (end - start == 1) {
                *prob = *(convertPtrCG(float, dist, dist->offsetPMF) + start);
                return start;
            }
            
            i = (end - start) / 2 + start;
        }
        *prob = 0.0f;
        return UINT_MAX;
    }
    
    inline float probDiscrete1D(const Discrete1D* dist, uint num) {
        return *(convertPtrCG(float, dist, dist->offsetPMF) + num);
    }
    
    float sampleContinuousConsts1D(const ContinuousConsts1D* dist, float u, float* PDF) {
        const float* CDF = convertPtrCG(float, dist, dist->offsetCDF);
        uint start = 0;
        uint end = dist->numValues + 1;
        uint i = end / 2;
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
    
    inline float PDFContinuousConsts1D(const ContinuousConsts1D* dist, float point) {
        return *(convertPtrCG(float, dist, dist->offsetPDF) +
                 (uint)((point - dist->startDomain) / dist->widthStratum));//min(*, dist->numValues - 1)でクランプしたほうが良いかも。
    }
    
    float2 sampleContinuousConsts2D_H(const ContinuousConsts2D_H* dist, float u0, float u1, float* PDF) {
        float2 ret;
        float PDFTop;
        ret.s1 = sampleContinuousConsts1D(&dist->distParent, u1, &PDFTop);
        uint idxChild = (uint)((ret.s1 - dist->distParent.startDomain) / dist->distParent.widthStratum);
        ret.s0 = sampleContinuousConsts1D(convertPtrCG(ContinuousConsts1D, dist, dist->offsetChildren) + idxChild, u0, PDF);
        *PDF *= PDFTop;
        return ret;
    }
    
    float PDFContinuousConsts2D_H(const ContinuousConsts2D_H* dist, const float2* p) {
        float ret;
        ret = PDFContinuousConsts1D(&dist->distParent, p->s1);
        uint idxChild = (uint)((p->s1 - dist->distParent.startDomain) / dist->distParent.widthStratum);
        ret *= PDFContinuousConsts1D(convertPtrCG(ContinuousConsts1D, dist, dist->offsetChildren) + idxChild, p->s0);
        return ret;
    }
}

#endif
