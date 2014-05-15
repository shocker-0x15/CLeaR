//
//  CreateFunctions.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/11.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "CreateFunctions.hpp"
#include "ImageLoader.hpp"

void MaterialCreator::createFloat3ConstantTexture(const char* name, float s0, float s1, float s2) {
    cl_float3 f3Val;
    uint64_t texHead = addDataAligned<cl_uchar>(&scene->texturesData, 0);
    f3Val.s0 = s0; f3Val.s1 = s1; f3Val.s2 = s2;
    addDataAligned<cl_float3>(&scene->texturesData, f3Val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFloatConstantTexture(const char* name, float val) {
    uint64_t texHead = addDataAligned<cl_uchar>(&scene->texturesData, 10);
    addDataAligned<cl_float>(&scene->texturesData, val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createImageTexture(const char* name, const char* filename) {
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

void MaterialCreator::createCheckerBoardTexture(const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b) {
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


void MaterialCreator::createFresnelNoOp(const char* name) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, 0);//NoOp
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFresnelConductor(const char* name, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, 1);//Conductor
    cl_float3 f3Val;
    f3Val.s0 = eta_r; f3Val.s1 = eta_g; f3Val.s2 = eta_b;
    addDataAligned<cl_float3>(texData, f3Val);
    f3Val.s0 = k_r; f3Val.s1 = k_g; f3Val.s2 = k_b;
    addDataAligned<cl_float3>(texData, f3Val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFresnelDielectric(const char* name, float etaExt, float etaInt) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, 2);//Dielectric
    addDataAligned<float>(texData, etaExt);
    addDataAligned<float>(texData, etaInt);
    scene->addTexture(texHead, name);
}


void MaterialCreator::createDiffuseBRDF(size_t reflectanceIdx, size_t sigmaIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, 1);//Diffuse
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)sigmaIdx);//Roughness
}

void MaterialCreator::createSpecularBRDF(size_t reflectanceIdx, size_t fresnelIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, 2);//Specular Reflection
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)fresnelIdx);//Fresnel
}

void MaterialCreator::createSpecularBTDF(size_t transmissionIdx, float etaExt, float etaInt) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, 3);//Specular Transmission
    addDataAligned<cl_uint>(matData, (cl_uint)transmissionIdx);//Transmittance
    addDataAligned<cl_float>(matData, etaExt);
    addDataAligned<cl_float>(matData, etaInt);
    char fresnelName[256] = "internalFresnelDielectric_";
    strcat(fresnelName, matName);
    createFresnelDielectric(fresnelName, etaExt, etaInt);
    addDataAligned<cl_uint>(matData, (cl_uint)scene->idxOfTex(fresnelName));
}

void MaterialCreator::createWardBRDF(size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, 4);//Diffuse
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)anisoXIdx);//Roughness X
    addDataAligned<cl_uint>(matData, (cl_uint)anisoYIdx);//Roughness Y
}


void MaterialCreator::createMatteMaterial(const char* name, size_t reflectanceIdx, size_t sigmaIdx) {
    beginMaterial(name);
    createDiffuseBRDF(reflectanceIdx, sigmaIdx);
    endMaterial();
}

void MaterialCreator::createGlassMaterial(const char* name, size_t RIdx, size_t TIdx, float etaExt, float etaInt) {
    beginMaterial(name);
    char fresnelName[256] = "internalFresnelDielectric_";
    strcat(fresnelName, matName);
    createFresnelDielectric(fresnelName, etaExt, etaInt);
    createSpecularBRDF(RIdx, scene->idxOfTex(fresnelName));
    createSpecularBTDF(TIdx, etaExt, etaInt);
    endMaterial();
}

void MaterialCreator::createMetalMaterial(const char* name, size_t RIdx, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b) {
    beginMaterial(name);
    char fresnelName[256] = "internalFresnelConductor_";
    strcat(fresnelName, matName);
    createFresnelConductor(fresnelName, eta_r, eta_g, eta_b, k_r, k_g, k_b);
    createSpecularBRDF(RIdx, scene->idxOfTex(fresnelName));
    endMaterial();
}

void MaterialCreator::createWardMaterial(const char* name, size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx) {
    beginMaterial(name);
    createWardBRDF(reflectanceIdx, anisoXIdx, anisoYIdx);
    endMaterial();
}

void MaterialCreator::createMixMaterial(const char* name, size_t mat0Idx, size_t mat1Idx, size_t ratioIdx) {
    
}


void MaterialCreator::createDiffuseEDF(size_t emittanceIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* lightPropData = &scene->lightPropsData;
    addDataAligned<cl_uchar>(lightPropData, 1);//Diffuse
    addDataAligned<cl_uint>(lightPropData, (cl_uint)emittanceIdx);//Emittance
}

void MaterialCreator::createDiffuseLightProperty(const char* name, size_t emittanceIdx) {
    beginLightProperty(name);
    createDiffuseEDF(emittanceIdx);
    endLightProperty();
}
