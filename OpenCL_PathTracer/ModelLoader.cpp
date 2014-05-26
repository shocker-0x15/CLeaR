//
//  ModelLoader.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "ModelLoader.hpp"
#include <string>
#include <fstream>
#include "CreateFunctions.hpp"

inline void makeTangent(float nx, float ny, float nz, float* s) {
    if (fabsf(nx) > fabsf(ny)) {
        float invLen = 1.0f / sqrtf(nx * nx + nz * nz);
        s[0] = -nz * invLen;
        s[1] = 0.0f;
        s[2] = nx * invLen;
    }
    else {
        float invLen = 1.0f / sqrtf(ny * ny + nz * nz);
        s[0] = 0.0f;
        s[1] = nz * invLen;
        s[2] = -ny * invLen;
    }
}

bool loadObj(const char* fileName, Scene* scene) {
    std::ifstream ifs;
    ifs.open(fileName);
    if (ifs.fail()) {
        printf("failed to open.¥n");
        return false;
    }
    
    std::string sLine(256, '0');
    
    MaterialCreator &mc = MaterialCreator::sharedInstance();
    mc.setScene(scene);
    
//    //Diffuse
//    mc.createFloat3ConstantTexture("objfile_default_reflectance", 0.85f, 0.85f, 0.15f);
//    mc.createFloatConstantTexture("objfile_default_sigma", 0.0f);
//    mc.createMatteMaterial("objfile_default_material",
//                           scene->idxOfTex("objfile_default_reflectance"),
//                           scene->idxOfTex("objfile_default_sigma"));
//    //Ward
//    mc.createFloat3ConstantTexture("objfile_default_reflectance", 0.5f, 0.5f, 0.5f);
//    mc.createFloatConstantTexture("objfile_default_roughX", 0.15f);
//    mc.createFloatConstantTexture("objfile_default_roughY", 0.15f);
//    mc.createWardMaterial("objfile_default_material",
//                          scene->idxOfTex("objfile_default_reflectance"),
//                          scene->idxOfTex("objfile_default_roughX"),
//                          scene->idxOfTex("objfile_default_roughX"));
    //Ashikhmin
    mc.createFloat3ConstantTexture("objfile_default_Ashikhmin_Rs", 0.1f, 0.1f, 0.1f);
    mc.createFloat3ConstantTexture("objfile_default_Ashikhmin_Rd", 0.85f, 0.85f, 0.15f);
    mc.createFloatConstantTexture("objfile_default_Ashikhmin_nu", 2000.0f);
    mc.createFloatConstantTexture("objfile_default_Ashikhmin_nv", 2000.0f);
    mc.createAshikhminMaterial("objfile_default_material",
                               scene->idxOfTex("objfile_default_Ashikhmin_Rs"), scene->idxOfTex("objfile_default_Ashikhmin_Rd"),
                               scene->idxOfTex("objfile_default_Ashikhmin_nu"), scene->idxOfTex("objfile_default_Ashikhmin_nv"));
//    //Water
//    mc.createFloat3ConstantTexture("objfile_default_reflectance", 0.95f, 0.95f, 0.95f);
//    mc.createFloat3ConstantTexture("objfile_default_transmittance", 0.95f, 0.95f, 0.95f);
//    mc.createGlassMaterial("objfile_default_material",
//                           scene->idxOfTex("objfile_default_reflectance"),
//                           scene->idxOfTex("objfile_default_transmittance"), 1.0f, 1.333f);
//    mc.createFloat3ConstantTexture("objfile_default_reflectance", 1.0f, 1.0f, 1.0f);
//    //Gold
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           0.16111f, 0.37457f, 1.58671f, 3.9521e+0f, 2.6371e+0f, 1.9205e+0f);
//    //Silver
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           0.14221f, 0.12643f, 0.15837f, 4.5230e+0f, 3.3071e+0f, 2.3359e+0f);
//    //Copper
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           0.21359f, 0.977984f, 1.17237f, 4.1618e+0f, 2.5929e+0f, 2.3244e+0f);
//    //Chromium
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           3.0568f, 3.15007f, 2.22288f, 3.3785e+0f, 3.3278e+0, 3.0647e+0f);
//    //Aluminium
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           1.92139f, 0.9986353f, 0.58738f, 8.1420e+0f, 6.5823e+0f, 5.2806e+0f);
//    //Titanium
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           2.40904f, 1.87618f, 1.65813f, 3.1459e+0f, 2.5920e+0f, 2.2321e+0f);
//    //Platinum
//    mc.createMetalMaterial("objfile_default_material", scene->idxOfTex("objfile_default_reflectance"),
//                           2.54319f, 2.1202f, 1.8117f, 4.4853e+0f, 3.6989e+0f, 3.0588e+0f);
    
    scene->beginObject();
    
    bool smoothShading = false;
    while (ifs && std::getline(ifs, sLine)) {
        //最初の空白を削除。
        sLine.erase(0, sLine.find_first_not_of("　 ¥t"));

        switch (sLine[0]) {
            case 'o':
                break;
            case 'g':
                break;
            case 's':
                smoothShading = sLine.compare("s off") != 0;
                break;
            case 'v':
                switch (sLine[1]) {
                    case ' ':
                        float px, py, pz;
                        sscanf(sLine.c_str(), "v %f %f %f", &px, &py, &pz);
                        scene->addVertex(px, py, pz);
                        break;
                    case 't':
                        float u, v;
                        sscanf(sLine.c_str(), "vt %f %f", &u, &v);
                        scene->addUV(u, v);
                        break;
                    case 'n':
                        float nx, ny, nz;
                        sscanf(sLine.c_str(), "vn %f %f %f", &nx, &ny, &nz);
                        scene->addNormal(nx, ny, nz);
                        float tangent[3];
                        makeTangent(nx, ny, nz, tangent);
                        scene->addTangent(tangent[0], tangent[1], tangent[2]);
                        break;
                }
                break;
            case 'f': {
                uint32_t p0, p1, p2;
                uint32_t uv0, uv1, uv2;
                uint32_t ns0, ns1, ns2;
                Face face;
                if (sscanf(sLine.c_str(), "f %u/%u/%u %u/%u/%u %u/%u/%u",
                           &p0, &uv0, &ns0, &p1, &uv1, &ns1, &p2, &uv2, &ns2) == 9) {
                    --p0; --p1; --p2;
                    --uv0; --uv1; --uv2;
                    --ns0; --ns1; --ns2;
                    scene->addFace(Face::make_pn(p0, p1, p2, ns0, ns1, ns2, scene->idxOfMat("objfile_default_material")));
                }
                else if (sscanf(sLine.c_str(), "f %u//%u %u//%u %u//%u",
                                &p0, &ns0, &p1, &ns1, &p2, &ns2) == 6) {
                    --p0; --p1; --p2;
                    --ns0; --ns1; --ns2;
                    scene->addFace(Face::make_pn(p0, p1, p2, ns0, ns1, ns2, scene->idxOfMat("objfile_default_material")));
                }
                
                break;
            }
            default:
                break;
        }
    }
    
    scene->endObject();
    
    ifs.close();
    
    return true;
}

bool loadModel(const char* fileName, Scene* scene) {
    std::string sFileName = fileName;
    
    size_t extPos = sFileName.find_first_of(".");
    if (extPos == std::string::npos)
        return false;
    
    std::string sExt = sFileName.substr(extPos + 1);
    if (!sExt.compare("obj")) {
        loadObj(fileName, scene);
    }
    
    return false;
}
