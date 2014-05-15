//
//  CreateFunctions.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_CreateFunctions_hpp
#define OpenCL_PathTracer_CreateFunctions_hpp

#include "clUtility.hpp"
#include "Scene.hpp"

class MaterialCreator {
    Scene* scene;
    char matName[256];
    uint8_t numBxDFs;
    uint64_t matHead;
    
    MaterialCreator() { };
public:
    static MaterialCreator &sharedInstance() {
        static MaterialCreator mcreator;
        return mcreator;
    };
    
    void setScene(Scene* scene) {
        this->scene = scene;
    }
    
    void beginMaterial(const char* name) {
        strcpy(matName, name);
        numBxDFs = 0;
        matHead = addDataAligned<cl_uchar>(&scene->materialsData, 0);
        scene->addMaterial(matHead, name);
    }
    
    void endMaterial() {
        scene->materialsData[matHead] = numBxDFs;
    }
    
    void beginLightProperty(const char* name) {
        numBxDFs = 0;
        matHead = addDataAligned<cl_uchar>(&scene->lightPropsData, 0);
        scene->addLightProp(matHead, name);
    }
    
    void endLightProperty() {
        scene->lightPropsData[matHead] = numBxDFs;
    }
    
    void createFloat3ConstantTexture(const char* name, float s0, float s1, float s2);
    void createFloatConstantTexture(const char* name, float val);
    void createImageTexture(const char* name, const char* filename);
    void createCheckerBoardTexture(const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b);
    
    void createFresnelNoOp(const char* name);
    void createFresnelConductor(const char* name, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b);
    void createFresnelDielectric(const char* name, float eta, float k);
    
    
    void createDiffuseBRDF(size_t reflectanceIdx, size_t sigmaIdx);
    void createSpecularBRDF(size_t reflectanceIdx, size_t fresnelIdx);
    void createSpecularBTDF(size_t transmissionIdx, float etaExt, float etaInt);
    void createWardBRDF(size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx);
    
    
    void createMatteMaterial(const char* name, size_t reflectanceIdx, size_t sigmaIdx);
    void createGlassMaterial(const char* name, size_t RIdx, size_t TIdx, float etaExt, float etaInt);
    void createMetalMaterial(const char* name, size_t RIdx, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b);
    void createWardMaterial(const char* name, size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx);
    void createMixMaterial(const char* name, size_t mat0Idx, size_t mat1Idx, size_t ratioIdx);
    
    
    void createDiffuseEDF(size_t emittanceIdx);
    
    void createDiffuseLightProperty(const char* name, size_t emittanceIdx);
};

#endif
