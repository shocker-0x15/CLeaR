//
//  CreateFunctions.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/11.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "CreateFunctions.hpp"
#include "ImageLoader.hpp"

namespace TextureType {
    enum Values {
        ColorConstant = 0,
        ColorImage,
        ColorProcedural,
        FloatConstant,
        FloatImage,
        FloatProcedural,
    };
};

namespace ColorProceduralType {
    enum Values {
        CheckerBoard = 0,
        CheckerBoardBump
    };
};

namespace FloatProceduralType {
    enum Values {
        CheckerBoard = 0,
    };
};

namespace BxDFID {
    enum Values {
        Diffuse = 0,
        SpecularReflection,
        SpecularTransmission,
        NewWard,
        AshikhminS,
        AshikhminD,
    };
};

namespace FresnelID {
    enum Values {
        NoOp = 0,
        Conductor,
        Dielectric,
    };
};

namespace EEDFID {
    enum Values {
        DiffuseEmission = 0,
    };
};

void MaterialCreator::createFloat3ConstantTexture(const char* name, float s0, float s1, float s2) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    cl_float3 f3Val;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::ColorConstant, 16);
    f3Val.s0 = s0; f3Val.s1 = s1; f3Val.s2 = s2;
    addDataAligned<cl_float3>(texData, f3Val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFloatConstantTexture(const char* name, float val) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::FloatConstant, 4);
    addDataAligned<cl_float>(texData, val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createImageTexture(const char* name, const char* filename) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint32_t w, h;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::ColorImage, 4);
    addDataAligned<cl_uint>(texData, 0);// width
    addDataAligned<cl_uint>(texData, 0);// height
//    // DELETE ME/
//    align(texData, sizeof(cl_half) * 4);
//    loadEnvMap("images/LA_Downtown_Afternoon_Fishing_3k.exr", texData);
//    // /DELETE ME
    align(texData, sizeof(cl_uchar3));
    loadImage(filename, texData, &w, &h, false);
    *(cl_uint*)(texData->data() + texHead + sizeof(cl_uint) * 1) = w;
    *(cl_uint*)(texData->data() + texHead + sizeof(cl_uint) * 2) = h;
    scene->addTexture(texHead, name);
}

void MaterialCreator::createNormalMapTexture(const char* name, const char* filename) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint32_t w, h;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::ColorImage, 4);
    addDataAligned<cl_uint>(texData, 0);// width
    addDataAligned<cl_uint>(texData, 0);// height
    align(texData, sizeof(cl_uchar3));
    loadImage(filename, texData, &w, &h, true);
    *(cl_uint*)(texData->data() + texHead + sizeof(cl_uint) * 1) = w;
    *(cl_uint*)(texData->data() + texHead + sizeof(cl_uint) * 2) = h;
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFloat3CheckerBoardTexture(const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::ColorProcedural, 16);
    addDataAligned<cl_uchar>(texData, ColorProceduralType::CheckerBoard);
    cl_float3 f3Val;
    f3Val.s0 = c0r; f3Val.s1 = c0g; f3Val.s2 = c0b;
    addDataAligned<cl_float3>(texData, f3Val);
    f3Val.s0 = c1r; f3Val.s1 = c1g; f3Val.s2 = c1b;
    addDataAligned<cl_float3>(texData, f3Val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFloat3CheckerBoardBumpTexture(const char* name, float width, bool reverse) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::ColorProcedural, 4);
    addDataAligned<cl_uchar>(texData, ColorProceduralType::CheckerBoardBump);
    addDataAligned<cl_float>(texData, width);
    addDataAligned<cl_uchar>(texData, reverse);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFloatCheckerBoardTexture(const char* name, float c0, float c1) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, TextureType::FloatProcedural, 4);
    addDataAligned<cl_uchar>(texData, FloatProceduralType::CheckerBoard);
    addDataAligned<cl_float>(texData, c0);
    addDataAligned<cl_float>(texData, c1);
    scene->addTexture(texHead, name);
}


void MaterialCreator::createFresnelNoOp(const char* name) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, FresnelID::NoOp, 1);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFresnelConductor(const char* name, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, FresnelID::Conductor, 16);
    cl_float3 f3Val;
    f3Val.s0 = eta_r; f3Val.s1 = eta_g; f3Val.s2 = eta_b;
    addDataAligned<cl_float3>(texData, f3Val);
    f3Val.s0 = k_r; f3Val.s1 = k_g; f3Val.s2 = k_b;
    addDataAligned<cl_float3>(texData, f3Val);
    scene->addTexture(texHead, name);
}

void MaterialCreator::createFresnelDielectric(const char* name, float etaExt, float etaInt) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = addDataAligned<cl_uchar>(texData, FresnelID::Dielectric, 4);
    addDataAligned<float>(texData, etaExt);
    addDataAligned<float>(texData, etaInt);
    scene->addTexture(texHead, name);
}


void MaterialCreator::createDiffuseBRDF(size_t reflectanceIdx, size_t sigmaIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, BxDFID::Diffuse, 4);
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)sigmaIdx);//Roughness
}

void MaterialCreator::createSpecularBRDF(size_t reflectanceIdx, size_t fresnelIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, BxDFID::SpecularReflection, 4);
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)fresnelIdx);//Fresnel
}

void MaterialCreator::createSpecularBTDF(size_t transmissionIdx, float etaExt, float etaInt) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, BxDFID::SpecularTransmission, 4);
    addDataAligned<cl_uint>(matData, (cl_uint)transmissionIdx);//Transmittance
    addDataAligned<cl_float>(matData, etaExt);
    addDataAligned<cl_float>(matData, etaInt);
    char fresnelName[256] = "internalFresnelDielectric_";
    strcat(fresnelName, matName);
    createFresnelDielectric(fresnelName, etaExt, etaInt);
    addDataAligned<cl_uint>(matData, (cl_uint)scene->idxOfTex(fresnelName));
}

void MaterialCreator::createNewWardBRDF(size_t reflectanceIdx, size_t anisoXIdx, size_t anisoYIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, BxDFID::NewWard, 4);
    addDataAligned<cl_uint>(matData, (cl_uint)reflectanceIdx);//Reflectance
    addDataAligned<cl_uint>(matData, (cl_uint)anisoXIdx);//Roughness X
    addDataAligned<cl_uint>(matData, (cl_uint)anisoYIdx);//Roughness Y
}

void MaterialCreator::createAshikhminSBRDF(size_t RsIdx, size_t nuIdx, size_t nvIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, BxDFID::AshikhminS, 4);
    addDataAligned<cl_uint>(matData, (cl_uint)RsIdx);//Perpendicular Reflectance of Specular
    addDataAligned<cl_uint>(matData, (cl_uint)nuIdx);//exponent u
    addDataAligned<cl_uint>(matData, (cl_uint)nvIdx);//exponent v
}

void MaterialCreator::createAshikhminDBRDF(size_t RdIdx, size_t RsIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    addDataAligned<cl_uchar>(matData, BxDFID::AshikhminD, 4);
    addDataAligned<cl_uint>(matData, (cl_uint)RdIdx);//Reflectance of Diffuse
    addDataAligned<cl_uint>(matData, (cl_uint)RsIdx);//Perpendicular Reflectance of Specular Coating
}


void MaterialCreator::createMatteMaterial(const char* name, const char* bump, const char* reflectance, const char* sigma) {
    beginMaterial(name);
    std::vector<uint8_t>* matData = &scene->materialsData;
    bool hasBump = bump != nullptr;
    addDataAligned<cl_uchar>(matData, hasBump);
    addDataAligned<cl_uint>(matData, hasBump ? (cl_uint)scene->idxOfTex(bump) : 0);
    createDiffuseBRDF(scene->idxOfTex(reflectance), scene->idxOfTex(sigma));
    endMaterial();
}

void MaterialCreator::createGlassMaterial(const char* name, const char* bump, const char* R, const char* T, float etaExt, float etaInt) {
    beginMaterial(name);
    std::vector<uint8_t>* matData = &scene->materialsData;
    bool hasBump = bump != nullptr;
    addDataAligned<cl_uchar>(matData, hasBump);
    addDataAligned<cl_uint>(matData, hasBump ? (cl_uint)scene->idxOfTex(bump) : 0);
    char fresnelName[256] = "internalFresnelDielectric_";
    strcat(fresnelName, matName);
    createFresnelDielectric(fresnelName, etaExt, etaInt);
    createSpecularBRDF(scene->idxOfTex(R), scene->idxOfTex(fresnelName));
    createSpecularBTDF(scene->idxOfTex(T), etaExt, etaInt);
    endMaterial();
}

void MaterialCreator::createMetalMaterial(const char* name, const char* bump, const char* R, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b) {
    beginMaterial(name);
    std::vector<uint8_t>* matData = &scene->materialsData;
    bool hasBump = bump != nullptr;
    addDataAligned<cl_uchar>(matData, hasBump);
    addDataAligned<cl_uint>(matData, hasBump ? (cl_uint)scene->idxOfTex(bump) : 0);
    char fresnelName[256] = "internalFresnelConductor_";
    strcat(fresnelName, matName);
    createFresnelConductor(fresnelName, eta_r, eta_g, eta_b, k_r, k_g, k_b);
    createSpecularBRDF(scene->idxOfTex(R), scene->idxOfTex(fresnelName));
    endMaterial();
}

void MaterialCreator::createNewWardMaterial(const char* name, const char* bump, const char* reflectance, const char* anisoX, const char* anisoY) {
    beginMaterial(name);
    std::vector<uint8_t>* matData = &scene->materialsData;
    bool hasBump = bump != nullptr;
    addDataAligned<cl_uchar>(matData, hasBump);
    addDataAligned<cl_uint>(matData, hasBump ? (cl_uint)scene->idxOfTex(bump) : 0);
    createNewWardBRDF(scene->idxOfTex(reflectance), scene->idxOfTex(anisoX), scene->idxOfTex(anisoY));
    endMaterial();
}

void MaterialCreator::createAshikhminMaterial(const char *name, const char* bump, const char* Rs, const char* Rd, const char* nu, const char* nv) {
    beginMaterial(name);
    std::vector<uint8_t>* matData = &scene->materialsData;
    bool hasBump = bump != nullptr;
    addDataAligned<cl_uchar>(matData, hasBump);
    addDataAligned<cl_uint>(matData, hasBump ? (cl_uint)scene->idxOfTex(bump) : 0);
    createAshikhminSBRDF(scene->idxOfTex(Rs), scene->idxOfTex(nu), scene->idxOfTex(nv));
    createAshikhminDBRDF(scene->idxOfTex(Rd), scene->idxOfTex(Rs));
    endMaterial();
}

void MaterialCreator::createMixMaterial(const char* name, const char* mat0, const char* mat1, const char* ratio) {
    
}


void MaterialCreator::createDiffuseEDF(size_t emittanceIdx) {
    ++numBxDFs;
    std::vector<uint8_t>* lightPropData = &scene->materialsData;
    addDataAligned<cl_uchar>(lightPropData, EEDFID::DiffuseEmission);
    addDataAligned<cl_uint>(lightPropData, (cl_uint)emittanceIdx);//Emittance
}

void MaterialCreator::createDiffuseLightProperty(const char* name, const char* emittance) {
    beginLightProperty(name);
    createDiffuseEDF(scene->idxOfTex(emittance));
    endLightProperty();
}
