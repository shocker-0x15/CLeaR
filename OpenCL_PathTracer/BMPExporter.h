//
//  BMPExporter.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/07/28.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_BMPExporter_h
#define OpenCL_PathTracer_BMPExporter_h

#include <cstdint>

void saveBMP(const char* filename, const void* pixels, uint32_t width, uint32_t height);

#endif
