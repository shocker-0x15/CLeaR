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
#include <cassert>

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
        assert(numBxDFs > 0);
        scene->materialsData[matHead] = numBxDFs;
    }
    
    void beginLightProperty(const char* name) {
        numBxDFs = 0;
        matHead = addDataAligned<cl_uchar>(&scene->materialsData, 0);
        scene->addLightProp(matHead, name);
    }
    
    void endLightProperty() {
        assert(numBxDFs > 0);
        scene->materialsData[matHead] = numBxDFs;
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
    
    
    void createDiffuseBRDF(size_t reflectanceIdx, size_t sigmaIdx);
    void createSpecularBRDF(size_t reflectanceIdx, size_t fresnelIdx);
    void createSpecularBTDF(size_t transmissionIdx, float etaExt, float etaInt);
    void createNewWardBRDF(size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx);
    void createAshikhminSBRDF(size_t RsIdx, size_t nuIdx, size_t nvIdx);
    void createAshikhminDBRDF(size_t RdIdx, size_t RsIdx);
    
    
    void createMatteMaterial(const char* name, const char* bump, const char* reflectance, const char* sigma);
    void createGlassMaterial(const char* name, const char* bump, const char* R, const char* T, float etaExt, float etaInt);
    void createMetalMaterial(const char* name, const char* bump, const char* R, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b);
    void createNewWardMaterial(const char* name, const char* bump, const char* reflectance, const char* anisoX, const char* anisoY);
    void createAshikhminMaterial(const char* name, const char* bump, const char* Rs, const char* Rd, const char* nu, const char* nv);
    void createMixMaterial(const char* name, const char* mat0, const char* mat1, const char* ratio);
    
    
    void createDiffuseEDF(size_t emittanceIdx);
    
    void createDiffuseLightProperty(const char* name, const char* emittance);
};

#endif
