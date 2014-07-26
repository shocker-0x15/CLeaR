#ifndef sim_distribution_cl
#define sim_distribution_cl

#include "sim_global.hpp"

namespace sim {
    typedef enum {
        DistributionType_Discrete1D = 0,
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
    
    //------------------------
    
    uint sampleDiscrete1D(const Discrete1D* dist, float u, float* prob);
    
    //------------------------
    
    uint sampleDiscrete1D(const Discrete1D* dist, float u, float* prob) {
        const float* CDF = (const float*)((const uchar*)dist + dist->offsetCDF);
        for (uint i = 0; i < dist->numItems; ++i) {
            if (*(CDF + i) > u) {
                *prob = *((const float*)((const uchar*)dist + dist->offsetPMF) + i);
                return i;
            }
        }
        return UINT_MAX;
    }
    
    float probDiscrete1D(const Discrete1D* dist, uint num) {
        return *((const float*)((const uchar*)dist + dist->offsetPMF) + num);
    }
}

#endif
