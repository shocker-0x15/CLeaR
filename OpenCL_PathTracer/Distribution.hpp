//
//  Distribution.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/07/25.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Distribution_h
#define OpenCL_PathTracer_Distribution_h

#include <cstdint>

namespace DistributionType {
    enum Value {
        Discrete1D = 0,
    };
}

//1byte
typedef struct {
    uint8_t _type;
} DistributionHead;

//16bytes
typedef struct {
    DistributionHead head; uint8_t dum0[3];
    uint32_t numItems;
    int32_t offsetPMF;
    int32_t offsetCDF;
} Discrete1D;

#endif
