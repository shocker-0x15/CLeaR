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
#include "Scene.h"
#include <cassert>

class MaterialCreator {
    Scene* scene;
    char matName[256];
    uint8_t numBxDFs;
    uint8_t numEEDFs;
    uint64_t matHead;
    uint64_t litHead;
    
    MaterialCreator() { };
public:
    static MaterialCreator &sharedInstance() {
        static MaterialCreator mcreator;
        return mcreator;
    };
    
    void setScene(Scene* scene) {
        this->scene = scene;
    }
    
    void beginMaterial(const char* name, const char* bump = nullptr) {
        strcpy(matName, name);
        numBxDFs = 0;
        std::vector<uint8_t>* matData = &scene->materialsData;
        matHead = CLUtil::addDataAligned<cl_uchar>(matData, 0);
        scene->addMaterial(matHead, name);
        bool hasBump = bump != nullptr;
        CLUtil::addDataAligned<cl_uchar>(matData, hasBump);
        CLUtil::addDataAligned<cl_uint>(matData, hasBump ? (cl_uint)scene->idxOfTex(bump) : 0);
    }
    
    void endMaterial() {
        assert(numBxDFs > 0);
        scene->materialsData[matHead] = numBxDFs;
    }
    
    void beginLightProperty(const char* name) {
        numEEDFs = 0;
        litHead = CLUtil::addDataAligned<cl_uchar>(&scene->materialsData, 0);
        scene->addLightProp(litHead, name);
    }
    
    void endLightProperty() {
        assert(numEEDFs >= 0);
        scene->materialsData[litHead] = numEEDFs;
    }
    
    void createFloat3ConstantTexture(const char* name, float s0, float s1, float s2);
    void createFloatConstantTexture(const char* name, float val);
    void createImageTexture(const char* name, const char* filename);
    void createNormalMapTexture(const char* name, const char* filename);
    void createFloat3CheckerBoardTexture(const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b);
    void createFloat3CheckerBoardBumpTexture(const char* name, float width = 0.01f, bool reverse = false);
    void createFloatCheckerBoardTexture(const char* name, float c0, float c1);
    
    void createFresnelNoOp(const char* name);
    void createFresnelConductor(const char* name, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b);
    void createFresnelDielectric(const char* name, float eta, float k);
    
    float averageLuminance(const char* image, float xLeft, float xRight, float yTop, float yBottom);
    void createContinuousConsts2D_H_FromImageTexture(const char* name, const char* image, bool zenithCorrection);
    
    
    void createDiffuseRElem(const char* reflectance, const char* sigma);
    void createSpecularRElem(const char* reflectance, const char* fresnel);
    void createSpecularTElem(const char* transmittance, float etaExt, float etaInt);
    void createNewWardElem(const char* reflectance, const char* anisoX, const char* anisoY);
    void createAshikhminSElem(const char* Rs, const char* nu, const char* nv);
    void createAshikhminDElem(const char* Rd, const char* Rs);
    
    void createMatteMaterial(const char* name, const char* bump, const char* reflectance, const char* sigma);
    void createGlassMaterial(const char* name, const char* bump, const char* R, const char* T, float etaExt, float etaInt);
    void createMetalMaterial(const char* name, const char* bump, const char* R, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b);
    void createNewWardMaterial(const char* name, const char* bump, const char* reflectance, const char* anisoX, const char* anisoY);
    void createAshikhminMaterial(const char* name, const char* bump, const char* Rs, const char* Rd, const char* nu, const char* nv);
    void createMixMaterial(const char* name, const char* mat0, const char* mat1, const char* ratio);
    
    
    void createDiffuseLElem(const char* emittance);
    
    void createDiffuseLightProperty(const char* name, const char* emittance);
    
    
    void createImageBasedEnvLElem(const char* radiance, float multiplier = 1.0f);
    
    void createImageBasedEnvLightPropety(const char* name, const char* radiance, float multiplier = 1.0f);
};

#endif
