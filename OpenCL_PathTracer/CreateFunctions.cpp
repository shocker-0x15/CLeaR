//
//  CreateFunctions.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/11.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "CreateFunctions.hpp"
#include "ImageLoader.h"
#include "MaterialStructures.hpp"
#include "TextureStructures.hpp"
#include <half.h>

#include "BMPExporter.h"

void MaterialCreator::createFloat3ConstantTexture(const char* name, float s0, float s1, float s2) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    Float3ConstantTexture f3Const;
    f3Const.texType = TextureType::ColorConstant;
    f3Const.value.s0 = s0;
    f3Const.value.s1 = s1;
    f3Const.value.s2 = s2;
    bool ret = scene->addTexture(addDataAligned(texData, f3Const, 16), name);
    assert(ret);
}

void MaterialCreator::createFloatConstantTexture(const char* name, float val) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    FloatConstantTexture fConst;
    fConst.texType = TextureType::FloatConstant;
    fConst.value = val;
    bool ret = scene->addTexture(addDataAligned(texData, fConst, 4), name);
    assert(ret);
}

void MaterialCreator::createImageTexture(const char* name, const char* filename, bool* hasAlpha) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead;
    if (scene->texFilehasLoaded(filename, &texHead)) {
        bool ret = scene->addTexture(texHead, name);
        assert(ret);
        
        ImageTexture* image = (ImageTexture*)&(*texData)[texHead];
        if (hasAlpha != nullptr)
            *hasAlpha = (image->texType == TextureType::ColorImageRGBA8x4 ||
                         image->texType == TextureType::ColorImageRGBA16Fx4);
    }
    else {
        uint64_t texHead = fillZerosAligned(texData, sizeof(ImageTexture), 4);
        uint64_t imageHead = align(texData, 128);
        uint32_t w, h;
        ColorChannel::Value colorType;
        bool ret = loadImage(filename, texData, &w, &h, &colorType, false);
        assert(ret);
        ImageTexture* image = (ImageTexture*)&(*texData)[texHead];
        image->width = w;
        image->height = h;
        if (colorType == ColorChannel::RGB8x3)
            image->texType = TextureType::ColorImageRGB8x3;
        else if (colorType == ColorChannel::RGBA8x4)
            image->texType = TextureType::ColorImageRGBA8x4;
        else if (colorType == ColorChannel::RGBA16Fx4)
            image->texType = TextureType::ColorImageRGBA16Fx4;
        else if (colorType == ColorChannel::Gray8)
            image->texType = TextureType::GrayImage8;
        image->offsetData = (int32_t)imageHead - (int32_t)texHead;
        ret = scene->addTexture(texHead, name);
        assert(ret);
        ret = scene->addTexFileToDB(texHead, filename);
        assert(ret);
        
        if (hasAlpha != nullptr)
            *hasAlpha = (image->texType == TextureType::ColorImageRGBA8x4 ||
                         image->texType == TextureType::ColorImageRGBA16Fx4);
    }
}

void MaterialCreator::createNormalMapTexture(const char* name, const char* filename) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    uint64_t texHead = fillZerosAligned(texData, sizeof(ImageTexture), 4);
    uint64_t imageHead = align(texData, 128);
    uint32_t w, h;
    ColorChannel::Value colorType;
    bool ret = loadImage(filename, texData, &w, &h, &colorType, false);
    assert(ret);
    ImageTexture* image = (ImageTexture*)&(*texData)[texHead];
    image->texType = TextureType::ColorImageRGB8x3;
    image->width = w;
    image->height = h;
    image->offsetData = (int32_t)imageHead - (int32_t)texHead;
    ret = scene->addTexture(texHead, name);
    assert(ret);
}

void MaterialCreator::createFloat3CheckerBoardTexture(const char* name, float c0r, float c0g, float c0b, float c1r, float c1g, float c1b) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    Float3CheckerBoardTexture f3Checker;
    f3Checker.head.texType = TextureType::ColorProcedural;
    f3Checker.head.procedureType = ColorProcedureType::CheckerBoard;
    f3Checker.c[0].s0 = c0r;
    f3Checker.c[0].s1 = c0g;
    f3Checker.c[0].s2 = c0b;
    f3Checker.c[1].s0 = c1r;
    f3Checker.c[1].s1 = c1g;
    f3Checker.c[1].s2 = c1b;
    bool ret = scene->addTexture(addDataAligned(texData, f3Checker, 16), name);
    assert(ret);
}

void MaterialCreator::createFloat3CheckerBoardBumpTexture(const char* name, float width, bool reverse) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    Float3CheckerBoardBumpTexture f3CheckerBump;
    f3CheckerBump.head.texType = TextureType::ColorProcedural;
    f3CheckerBump.head.procedureType = ColorProcedureType::CheckerBoardBump;
    f3CheckerBump.width = width;
    f3CheckerBump.reverse = (cl_uchar)reverse;
    bool ret = scene->addTexture(addDataAligned(texData, f3CheckerBump, 4), name);
    assert(ret);
}

void MaterialCreator::createFloatCheckerBoardTexture(const char* name, float v0, float v1) {
    std::vector<uint8_t>* texData = &scene->texturesData;
    FloatCheckerBoardTexture fChecker;
    fChecker.head.texType = TextureType::ColorProcedural;
    fChecker.head.procedureType = ColorProcedureType::CheckerBoard;
    fChecker.v[0] = v0;
    fChecker.v[1] = v1;
    bool ret = scene->addTexture(addDataAligned(texData, fChecker, 4), name);
    assert(ret);
}


void MaterialCreator::createFresnelNoOp(const char* name) {
    std::vector<uint8_t>* otherResouces = &scene->otherResouces;
    FresnelNoOp frNoOp;
    frNoOp.head.fresnelType = FresnelID::NoOp;
    bool ret = scene->addOtherResouce(addDataAligned(otherResouces, frNoOp, 1), name);
    assert(ret);
}

void MaterialCreator::createFresnelConductor(const char* name, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b) {
    std::vector<uint8_t>* otherResouces = &scene->otherResouces;
    FresnelConductor frCond;
    frCond.head.fresnelType = FresnelID::Conductor;
    frCond.eta.s0 = eta_r;
    frCond.eta.s1 = eta_g;
    frCond.eta.s2 = eta_b;
    frCond.k.s0 = k_r;
    frCond.k.s1 = k_g;
    frCond.k.s2 = k_b;
    bool ret = scene->addOtherResouce(addDataAligned(otherResouces, frCond, 16), name);
    assert(ret);
}

void MaterialCreator::createFresnelDielectric(const char* name, float etaExt, float etaInt) {
    std::vector<uint8_t>* otherResouces = &scene->otherResouces;
    FresnelDielectric frDiel;
    frDiel.head.fresnelType = FresnelID::Dielectric;
    frDiel.etaExt = etaExt;
    frDiel.etaInt = etaInt;
    bool ret = scene->addOtherResouce(addDataAligned(otherResouces, frDiel, 4), name);
    assert(ret);
}


float MaterialCreator::averageLuminance(const char* image, float xLeft, float xRight, float yTop, float yBottom) {
    ImageTexture* imgTex = (ImageTexture*)&scene->texturesData[scene->idxOfTex(image)];
    void* dataHead = (uint8_t*)imgTex + imgTex->offsetData;
    
    float sumLuminance = 0.0f;
    uint32_t xLeftPix = (uint32_t)xLeft;
    uint32_t xRightPix = (uint32_t)xRight;
    uint32_t yTopPix = (uint32_t)yTop;
    uint32_t yBottomPix = (uint32_t)yBottom;
    
    float R, G, B, luminance;
    
    uint64_t offsetsCorners[] = {
        4 * (yTopPix * imgTex->width + xLeftPix),
        4 * (yTopPix * imgTex->width + xRightPix),
        4 * (yBottomPix * imgTex->width + xLeftPix),
        4 * (yBottomPix * imgTex->width + xRightPix)
    };
    float coeffsCorners[] = {
        (xLeftPix + 1 - xLeft) * (yTopPix + 1 - yTop),
        (xRight - xRightPix) * (yTopPix + 1 - yTop),
        (xLeftPix + 1 - xLeft) * (yBottom - yBottomPix),
        (xRight - xRightPix) * (yBottom - yBottomPix)
    };
    for (uint32_t i = 0; i < 4; ++i) {
        R = float(*((half*)dataHead + offsetsCorners[i] + 0));
        G = float(*((half*)dataHead + offsetsCorners[i] + 1));
        B = float(*((half*)dataHead + offsetsCorners[i] + 2));
        luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        sumLuminance += coeffsCorners[i] * luminance;
    }
    
    float coeffsEdges[] = {
        yTopPix + 1 - yTop,
        xLeftPix + 1 - xLeft,
        xRight - xRightPix,
        yBottom - yBottomPix
    };
    for (uint32_t x = xLeftPix + 1; x < xRightPix; ++x) {
        uint64_t offsetEdge;
        
        offsetEdge = 4 * (yTopPix * imgTex->width + x);
        R = float(*((half*)dataHead + offsetEdge + 0));
        G = float(*((half*)dataHead + offsetEdge + 1));
        B = float(*((half*)dataHead + offsetEdge + 2));
        luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        sumLuminance += coeffsEdges[0] * luminance;
        
        offsetEdge = 4 * (yBottomPix * imgTex->width + x);
        R = float(*((half*)dataHead + offsetEdge + 0));
        G = float(*((half*)dataHead + offsetEdge + 1));
        B = float(*((half*)dataHead + offsetEdge + 2));
        luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        sumLuminance += coeffsEdges[3] * luminance;
    }
    for (uint32_t y = yTopPix + 1; y < yBottomPix; ++y) {
        uint64_t offsetEdge;
        
        offsetEdge = 4 * (y * imgTex->width + xLeftPix);
        R = float(*((half*)dataHead + offsetEdge + 0));
        G = float(*((half*)dataHead + offsetEdge + 1));
        B = float(*((half*)dataHead + offsetEdge + 2));
        luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        sumLuminance += coeffsEdges[1] * luminance;
        
        offsetEdge = 4 * (y * imgTex->width + xRightPix);
        R = float(*((half*)dataHead + offsetEdge + 0));
        G = float(*((half*)dataHead + offsetEdge + 1));
        B = float(*((half*)dataHead + offsetEdge + 2));
        luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        sumLuminance += coeffsEdges[2] * luminance;
    }
    
    for (uint32_t y = yTopPix + 1; y < yBottomPix; ++y) {
        for (uint32_t x = xLeftPix + 1; x < xRightPix; ++x) {
            uint64_t offsetPix = 4 * (y * imgTex->width + x);
            R = float(*((half*)dataHead + offsetPix + 0));
            G = float(*((half*)dataHead + offsetPix + 1));
            B = float(*((half*)dataHead + offsetPix + 2));
            luminance = 0.2126f * R + 0.7152f * G + 0.0722f * B;
            sumLuminance += luminance;
        }
    }
    
    return sumLuminance / ((xRight - xLeft) * (yBottom - yTop));
}

void MaterialCreator::createContinuousConsts2D_H_FromImageTexture(const char* name, const char* image, bool zenithCorrection) {
    uint32_t sizeX = 64, sizeY = 64;
    std::vector<uint8_t>* otherResouces = &scene->otherResouces;
    ImageTexture* imgTex = (ImageTexture*)&scene->texturesData[scene->idxOfTex(image)];
    
    uint64_t CC2DHHead = fillZerosAligned(otherResouces, sizeof(ContinuousConsts2D_H), 4);
    uint64_t pPDFHead = fillZerosAligned(otherResouces, sizeof(float) * sizeY, sizeof(float));
    uint64_t pCDFHead = fillZerosAligned(otherResouces, sizeof(float) * (sizeY + 1), sizeof(float));
    uint64_t childrenHead = fillZerosAligned(otherResouces, sizeof(ContinuousConsts1D) * sizeY, 4);
    uint64_t cPDFsHead = fillZerosAligned(otherResouces, sizeof(float) * sizeX * sizeY, sizeof(float));
    uint64_t cCDFsHead = fillZerosAligned(otherResouces, sizeof(float) * (sizeX + 1) * sizeY, sizeof(float));
    
    uint64_t childHead = childrenHead;
    uint64_t cPDFHead = cPDFsHead;
    uint64_t cCDFHead = cCDFsHead;
    for (uint32_t i = 0; i < sizeY; ++i) {
        ContinuousConsts1D* child = (ContinuousConsts1D*)(otherResouces->data() + childHead);
        child->head._type = DistributionType::ContinuousConsts1D;
        child->startDomain = 0.0f;
        child->endDomain = 1.0f;
        child->widthStratum = (child->endDomain - child->startDomain) / sizeX;
        child->numValues = sizeX;
        child->offsetPDF = (int32_t)cPDFHead - (int32_t)childHead;
        child->offsetCDF = (int32_t)cCDFHead - (int32_t)childHead;
        
        float* cPDF = (float*)(otherResouces->data() + cPDFHead);
        float* cCDF = (float*)(otherResouces->data() + cCDFHead);
        
        float yBase = ((float)imgTex->height / sizeY) * i;
        float yNext = ((float)imgTex->height / sizeY) * (i + 1);
        float correction = zenithCorrection ? cosf(M_PI * (0.5f - (i + 0.5f) / sizeY)) : 1.0f;
        for (uint32_t j = 0; j < sizeX; ++j) {
            float xBase = ((float)imgTex->width / sizeX) * j;
            float xNext = ((float)imgTex->width / sizeX) * (j + 1);
            *(cPDF + j) = averageLuminance(image, xBase, xNext, yBase, yNext) * correction;
        }
        
        *(cCDF + 0) = 0.0f;
        for (uint32_t j = 1; j <= sizeX; ++j) {
            *(cCDF + j) = *(cCDF + j - 1) + *(cPDF + j - 1) * child->widthStratum;
        }
        
        float* pPDFValue = (float*)(otherResouces->data() + pPDFHead) + i;
        *pPDFValue = *(cCDF + sizeX);
        float normCoeff = 1.0f / *(cCDF + sizeX);
        for (uint32_t j = 0; j < sizeX; ++j) {
            *(cPDF + j) *= normCoeff;
            *(cCDF + j + 1) *= normCoeff;
        }
        
        childHead += sizeof(ContinuousConsts1D);
        cPDFHead += sizeof(float) * sizeX;
        cCDFHead += sizeof(float) * (sizeX + 1);
    }
    
    ContinuousConsts2D_H* CC2DH = (ContinuousConsts2D_H*)(otherResouces->data() + CC2DHHead);
    CC2DH->head._type = DistributionType::ContinuousConsts2D_H;
    CC2DH->offsetChildren = (int32_t)childrenHead - (int32_t)CC2DHHead;
    CC2DH->distParent.head._type = DistributionType::ContinuousConsts1D;
    CC2DH->distParent.startDomain = 0.0f;
    CC2DH->distParent.endDomain = 1.0f;
    CC2DH->distParent.widthStratum = (CC2DH->distParent.endDomain - CC2DH->distParent.startDomain) / sizeY;
    CC2DH->distParent.numValues = sizeY;
    CC2DH->distParent.offsetPDF = (int32_t)pPDFHead - (int32_t)CC2DHHead;
    CC2DH->distParent.offsetCDF = (int32_t)pCDFHead - (int32_t)CC2DHHead;
    
    float* pPDF = (float*)(otherResouces->data() + pPDFHead);
    float* pCDF = (float*)(otherResouces->data() + pCDFHead);
    *(pCDF + 0) = 0.0f;
    for (uint32_t i = 1; i <= sizeY; ++i) {
        *(pCDF + i) = *(pCDF + i - 1) + *(pPDF + i - 1) * CC2DH->distParent.widthStratum;
    }
    
    float normCoeff = 1.0f / *(pCDF + sizeY);
    for (uint32_t j = 0; j < sizeY; ++j) {
        *(pPDF + j) *= normCoeff;
        *(pCDF + j + 1) *= normCoeff;
    }
    
    bool ret = scene->addOtherResouce(CC2DHHead, name);
    assert(ret);
    
//    uint32_t byteWidth = sizeX * 3 + sizeX % 4;
//    uint8_t* visData = (uint8_t*)malloc(byteWidth * sizeY * sizeof(uint8_t));
//    float* cPDFs = (float*)(otherResouces->data() + cPDFsHead);
//    float sumPDFValue = 0.0f;
//    for (int y = 0; y < sizeY; ++y) {
//        float parentCoeff = *(pPDF + y);
//        float* cPDF = cPDFs + sizeX * y;
//        for (int x = 0; x < sizeX; ++x) {
//            float PDFValue = parentCoeff * *(cPDF + x);
//            sumPDFValue += PDFValue;
////            printf("%f, ", PDFValue);
//            uint32_t idxBMP = byteWidth * (sizeY - 1 - y) + 3 * x;
//            uint8_t u8Val = (uint8_t)std::min(255.0f, 20 * PDFValue);
//            visData[idxBMP + 0] = u8Val;
//            visData[idxBMP + 1] = u8Val;
//            visData[idxBMP + 2] = u8Val;
//        }
//        printf("\n");
//    }
//    saveBMP("IBL_luminance.bmp", visData, sizeX, sizeY);
//    free(visData);
}


void MaterialCreator::createDiffuseRElem(const char* reflectance, const char* sigma) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t head = align(matData, 4);
    matData->insert(matData->end(), sizeof(DiffuseRElem), 0);
    DiffuseRElem* diffuseInfo = (DiffuseRElem*)&(*matData)[head];
    diffuseInfo->id = MatElem::Diffuse;
    diffuseInfo->idx_R = (cl_uint)scene->idxOfTex(reflectance);
    diffuseInfo->idx_sigma = (cl_uint)scene->idxOfTex(sigma);
}

void MaterialCreator::createSpecularRElem(const char* reflectance, const char* fresnel) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t head = align(matData, 4);
    matData->insert(matData->end(), sizeof(SpecularRElem), 0);
    SpecularRElem* speRInfo = (SpecularRElem*)&(*matData)[head];
    speRInfo->id = MatElem::SpecularReflection;
    speRInfo->idx_R = (cl_uint)scene->idxOfTex(reflectance);
    speRInfo->idx_Fresnel = (cl_uint)scene->idxOfOther(fresnel);
}

void MaterialCreator::createSpecularTElem(const char* transmittance, float etaExt, float etaInt) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t head = align(matData, 4);
    matData->insert(matData->end(), sizeof(SpecularTElem), 0);
    SpecularTElem* speTInfo = (SpecularTElem*)&(*matData)[head];
    speTInfo->id = MatElem::SpecularTransmission;
    speTInfo->idx_T = (cl_uint)scene->idxOfTex(transmittance);
    speTInfo->etaExt = (cl_float)etaExt;
    speTInfo->etaInt = (cl_float)etaInt;
    char fresnelName[256] = "internalFresnelDielectric_";
    strcat(fresnelName, matName);
    createFresnelDielectric(fresnelName, etaExt, etaInt);
    speTInfo->idx_Fresnel = (cl_uint)scene->idxOfOther(fresnelName);
}

void MaterialCreator::createNewWardElem(const char* reflectance, const char* anisoX, const char* anisoY) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t head = align(matData, 4);
    matData->insert(matData->end(), sizeof(NewWardElem), 0);
    NewWardElem* wardInfo = (NewWardElem*)&(*matData)[head];
    wardInfo->id = MatElem::NewWard;
    wardInfo->idx_R = (cl_uint)scene->idxOfTex(reflectance);
    wardInfo->idx_anisoX = (cl_uint)scene->idxOfTex(anisoX);
    wardInfo->idx_anisoY = (cl_uint)scene->idxOfTex(anisoY);
}

void MaterialCreator::createAshikhminSElem(const char* Rs, const char* nu, const char* nv) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t head = align(matData, 4);
    matData->insert(matData->end(), sizeof(AshikhminSElem), 0);
    AshikhminSElem* ashSInfo = (AshikhminSElem*)&(*matData)[head];
    ashSInfo->id = MatElem::AshikhminS;
    ashSInfo->idx_Rs = (cl_uint)scene->idxOfTex(Rs);
    ashSInfo->idx_nu = (cl_uint)scene->idxOfTex(nu);
    ashSInfo->idx_nv = (cl_uint)scene->idxOfTex(nv);
}

void MaterialCreator::createAshikhminDElem(const char* Rd, const char* Rs) {
    ++numBxDFs;
    std::vector<uint8_t>* matData = &scene->materialsData;
    uint64_t head = align(matData, 4);
    matData->insert(matData->end(), sizeof(AshikhminDElem), 0);
    AshikhminDElem* ashDInfo = (AshikhminDElem*)&(*matData)[head];
    ashDInfo->id = MatElem::AshikhminD;
    ashDInfo->idx_Rd = (cl_uint)scene->idxOfTex(Rd);
    ashDInfo->idx_Rs = (cl_uint)scene->idxOfTex(Rs);
}


void MaterialCreator::createMatteMaterial(const char* name, const char* bump, const char* reflectance, const char* sigma) {
    beginMaterial(name, bump);
    createDiffuseRElem(reflectance, sigma);
    endMaterial();
}

void MaterialCreator::createGlassMaterial(const char* name, const char* bump, const char* R, const char* T, float etaExt, float etaInt) {
    beginMaterial(name, bump);
    char fresnelName[256] = "GlassInternalFresnelDielectric_";
    strcat(fresnelName, matName);
    createFresnelDielectric(fresnelName, etaExt, etaInt);
    createSpecularRElem(R, fresnelName);
    createSpecularTElem(T, etaExt, etaInt);
    endMaterial();
}

void MaterialCreator::createMetalMaterial(const char* name, const char* bump, const char* R, float eta_r, float eta_g, float eta_b, float k_r, float k_g, float k_b) {
    beginMaterial(name, bump);
    char fresnelName[256] = "MetalInternalFresnelConductor_";
    strcat(fresnelName, matName);
    createFresnelConductor(fresnelName, eta_r, eta_g, eta_b, k_r, k_g, k_b);
    createSpecularRElem(R, fresnelName);
    endMaterial();
}

void MaterialCreator::createNewWardMaterial(const char* name, const char* bump, const char* reflectance, const char* anisoX, const char* anisoY) {
    beginMaterial(name, bump);
    createNewWardElem(reflectance, anisoX, anisoY);
    endMaterial();
}

void MaterialCreator::createAshikhminMaterial(const char *name, const char* bump, const char* Rs, const char* Rd, const char* nu, const char* nv) {
    beginMaterial(name, bump);
    createAshikhminSElem(Rs, nu, nv);
    createAshikhminDElem(Rd, Rs);
    endMaterial();
}

void MaterialCreator::createMixMaterial(const char* name, const char* mat0, const char* mat1, const char* ratio) {
    
}


void MaterialCreator::createDiffuseLElem(const char* emittance) {
    ++numEEDFs;
    std::vector<uint8_t>* lightPropData = &scene->materialsData;
    uint64_t head = align(lightPropData, 4);
    lightPropData->insert(lightPropData->end(), sizeof(DiffuseLElem), 0);
    DiffuseLElem* diffuseInfo = (DiffuseLElem*)&(*lightPropData)[head];
    diffuseInfo->id = LPElem::DiffuseEmission;
    diffuseInfo->idx_M = (cl_uint)scene->idxOfTex(emittance);
}

void MaterialCreator::createDiffuseLightProperty(const char* name, const char* emittance) {
    beginLightProperty(name);
    createDiffuseLElem(emittance);
    endLightProperty();
}


void MaterialCreator::createImageBasedEnvLElem(const char* radiance, float multiplier) {
    ++numEEDFs;
    std::vector<uint8_t>* lightPropData = &scene->materialsData;
    uint64_t head = align(lightPropData, 4);
    lightPropData->insert(lightPropData->end(), sizeof(ImageBasedEnvLElem), 0);
    ImageBasedEnvLElem* IBEnvInfo = (ImageBasedEnvLElem*)&(*lightPropData)[head];
    IBEnvInfo->id = EnvLPElem::ImageBased;
    IBEnvInfo->idx_Le = (cl_uint)scene->idxOfTex(radiance);
    IBEnvInfo->multiplier = multiplier;
    char dist2DName[256] = "InternalDistribution2D_";
    strcat(dist2DName, radiance);
    createContinuousConsts2D_H_FromImageTexture(dist2DName, radiance, true);
    IBEnvInfo->idx_Dist2D = (cl_uint)scene->idxOfOther(dist2DName);
}

void MaterialCreator::createImageBasedEnvLightPropety(const char* name, const char* radiance, float multiplier) {
    beginLightProperty(name);
    createImageBasedEnvLElem(radiance, multiplier);
    endLightProperty();
}
