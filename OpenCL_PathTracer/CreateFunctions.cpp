//
//  CreateFunctions.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/11.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "CreateFunctions.hpp"
#include "ImageLoader.hpp"

void createFloat3ConstantTexture(Scene* scene, const char* name, float s0, float s1, float s2) {
    cl_float3 f3Val;
    uint64_t texHead = addDataAligned<cl_uchar>(&scene->texturesData, 0);
    f3Val.s0 = s0; f3Val.s1 = s1; f3Val.s2 = s2;
    addDataAligned<cl_float3>(&scene->texturesData, f3Val);
    scene->addTexture(texHead, name);
}

void createFloatConstantTexture(Scene* scene, const char* name, float val) {
    uint64_t texHead = addDataAligned<cl_uchar>(&scene->texturesData, 10);
    addDataAligned<cl_float>(&scene->texturesData, val);
    scene->addTexture(texHead, name);
}

void createImageTexture(Scene* scene, const char* name, const char* filename) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint32_t w, h;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, 1);
    addDataAligned<cl_uint>(texData, 0);
    addDataAligned<cl_uint>(texData, 0);
    align(texData, sizeof(cl_uchar3));
    loadImage(filename, texData, &w, &h);
    *(cl_uint*)(texData->data() + texHead + sizeof(cl_uint) * 1) = w;
    *(cl_uint*)(texData->data() + texHead + sizeof(cl_uint) * 2) = h;
    scene->addTexture(texHead, name);
}

void createCheckerBoardTexture(Scene* scene, const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, 2);
    addDataAligned<cl_uchar>(texData, 0);
    cl_float3 f3Val;
    f3Val.s0 = c0r; f3Val.s1 = c0g; f3Val.s2 = c0b;
    addDataAligned<cl_float3>(texData, f3Val);
    f3Val.s0 = c1r; f3Val.s1 = c1g; f3Val.s2 = c1b;
    addDataAligned<cl_float3>(texData, f3Val);
    scene->addTexture(texHead, name);
}

void createDiffuseMaterial(Scene* scene, const char* name, size_t reflectanceIdx, size_t sigmaIdx) {
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t matHead = addDataAligned<cl_uchar>(matData, 1);//Diffuse
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)sigmaIdx);//Roughness
    scene->addMaterial(matHead, name);
}

void createWardMaterial(Scene* scene, const char* name, size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx) {
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t matHead = addDataAligned<cl_uchar>(matData, 4);//Diffuse
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)anisoXIdx);//Roughness X
    addDataAligned<cl_uint>(matData, (cl_uint)anisoYIdx);//Roughness Y
    scene->addMaterial(matHead, name);
}

void createDiffuseLightProperty(Scene* scene, const char* name, size_t emittanceIdx) {
    std::vector<uint8_t>* lightPropData = &scene->lightPropsData;
    uint64_t lightHead = addDataAligned<cl_uchar>(lightPropData, 1);//Diffuse
    addDataAligned<cl_uint>(lightPropData, (cl_uint)emittanceIdx);//Emittance
    scene->addLightProp(lightHead, name);
}