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
#include <map>
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

namespace OBJ {
    struct Material {
        std::string name;
        float Kd[3];
        float Ks[3];
        float Ka[3];
        float Ns;
        std::string mapKd;
        std::string mapKs;
        std::string mapKa;
        std::string mapBump;
        
        Material() :
        name{""},
        Kd{0, 0, 0}, Ks{0, 0, 0}, Ka{0, 0, 0},
        Ns(100),
        mapKd{""}, mapKs{""}, mapKa{""}, mapBump{""} { };
    };
    
    struct Object {
        struct Group {
            enum Format {
                VTN,
                VN,
            };
            
            std::string name;
            std::string material;
            Format format;
            uint32_t smooth;
            std::vector<uint32_t> posIndices;
            std::vector<uint32_t> nrmIndices;
            std::vector<uint32_t> uvIndices;
        };
        
        std::string name;
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> uvs;
        std::vector<Group> groups;
    };
    
    bool loadMtl(const char* fileName, std::vector<Material>* materials, std::map<std::string, uint32_t>* indicesMat) {
        std::ifstream ifs;
        ifs.open(fileName);
        if (ifs.fail()) {
            printf("failed to open a .mtl file.\n");
            return false;
        }
        
        std::string sFileName = fileName;
        std::string directory = sFileName.substr(0, sFileName.find_last_of("/") + 1);
        std::string sLine(256, '0');
        
        Material* curMat = nullptr;
        while (ifs && std::getline(ifs, sLine)) {
            //最初の空白を削除。
            sLine.erase(0, sLine.find_first_not_of("　 ¥t"));
            
            switch (sLine[0]) {
                case 'K': {
                    if (sLine.length() < 2)
                        break;
                    char formatStr[128];
                    sprintf(formatStr, "K%c %%f %%f %%f", sLine[1]);
                    float r, g, b;
                    if (sscanf(sLine.c_str(), formatStr, &r, &g, &b) != 3)
                        r = g = b = 0.0f;
                    if (sLine[1] == 'd') {
                        curMat->Kd[0] = r; curMat->Kd[1] = g; curMat->Kd[2] = b;
                    }
                    else if (sLine[1] == 's') {
                        curMat->Ks[0] = r; curMat->Ks[1] = g; curMat->Ks[2] = b;
                    }
                    else if (sLine[1] == 'a') {
                        curMat->Ka[0] = r; curMat->Ka[1] = g; curMat->Ka[2] = b;
                    }
                    break;
                }
                case 'm': {
                    char texName[256];
                    if (sscanf(sLine.c_str(), "map_Kd %s", texName) == 1) {
                        curMat->mapKd = directory + texName;
                    }
                    else if (sscanf(sLine.c_str(), "map_Ks %s", texName) == 1) {
                        curMat->mapKs = directory + texName;
                    }
                    else if (sscanf(sLine.c_str(), "map_Ka %s", texName) == 1) {
                        curMat->mapKa = directory + texName;
                    }
                    else if (sscanf(sLine.c_str(), "map_bump %s", texName) == 1) {
                        curMat->mapKd = directory + texName;
                    }
                    break;
                }
                case 'n': {
                    char mtlName[256];
                    if (sscanf(sLine.c_str(), "newmtl %s", mtlName) != 1)
                        break;
                    indicesMat->insert(std::pair<std::string, uint32_t>(mtlName, (uint32_t)materials->size()));
                    materials->emplace_back();
                    curMat = &materials->back();
                    curMat->name = mtlName;
                    break;
                }
                default:
                    break;
            }
        }
        
        ifs.close();
        
        return true;
    }
    
    bool load(const char* fileName, Scene* scene) {
        std::ifstream ifs;
        ifs.open(fileName);
        if (ifs.fail()) {
            printf("failed to open an .obj file.\n");
            return false;
        }
        
        std::string sFileName = fileName;
        std::string directory = sFileName.substr(0, sFileName.find_last_of("/") + 1);
        std::string sLine(256, '0');
        
        MaterialCreator &mc = MaterialCreator::sharedInstance();
        mc.setScene(scene);
        
//        //Diffuse
//        mc.createFloat3ConstantTexture("objfile_default_reflectance", 0.85f, 0.85f, 0.15f);
//        mc.createFloatConstantTexture("objfile_default_sigma", 0.0f);
//        mc.createMatteMaterial("objfile_default_material", nullptr,
//                               "objfile_default_reflectance",
//                               "objfile_default_sigma");
//        //Ward
//        mc.createFloat3ConstantTexture("objfile_default_reflectance", 0.5f, 0.5f, 0.5f);
//        mc.createFloatConstantTexture("objfile_default_roughX", 0.15f);
//        mc.createFloatConstantTexture("objfile_default_roughY", 0.15f);
//        mc.createNewWardMaterial("objfile_default_material", nullptr,
//                                 "objfile_default_reflectance",
//                                 "objfile_default_roughX",
//                                 "objfile_default_roughY");
//        //Ashikhmin
//        mc.createFloat3ConstantTexture("objfile_default_Ashikhmin_Rs", 0.1f, 0.1f, 0.1f);
//        mc.createFloat3ConstantTexture("objfile_default_Ashikhmin_Rd", 0.85f, 0.85f, 0.15f);
//        mc.createFloatConstantTexture("objfile_default_Ashikhmin_nu", 2000.0f);
//        mc.createFloatConstantTexture("objfile_default_Ashikhmin_nv", 2000.0f);
//        mc.createAshikhminMaterial("objfile_default_material", nullptr,
//                                   "objfile_default_Ashikhmin_Rs", "objfile_default_Ashikhmin_Rd",
//                                   "objfile_default_Ashikhmin_nu", "objfile_default_Ashikhmin_nv");
        //Water
        mc.createFloat3ConstantTexture("objfile_default_reflectance", 0.95f, 0.95f, 0.95f);
        mc.createFloat3ConstantTexture("objfile_default_transmittance", 0.95f, 0.95f, 0.95f);
        mc.createGlassMaterial("objfile_default_material", nullptr, 
                               "objfile_default_reflectance",
                               "objfile_default_transmittance", 1.0f, 1.333f);
//        mc.createFloat3ConstantTexture("objfile_default_reflectance", 1.0f, 1.0f, 1.0f);
//        //Gold
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               0.16111f, 0.37457f, 1.58671f, 3.9521e+0f, 2.6371e+0f, 1.9205e+0f);
//        //Silver
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               0.14221f, 0.12643f, 0.15837f, 4.5230e+0f, 3.3071e+0f, 2.3359e+0f);
//        //Copper
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               0.21359f, 0.977984f, 1.17237f, 4.1618e+0f, 2.5929e+0f, 2.3244e+0f);
//        //Chromium
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               3.0568f, 3.15007f, 2.22288f, 3.3785e+0f, 3.3278e+0, 3.0647e+0f);
//        //Aluminium
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               1.92139f, 0.9986353f, 0.58738f, 8.1420e+0f, 6.5823e+0f, 5.2806e+0f);
//        //Titanium
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               2.40904f, 1.87618f, 1.65813f, 3.1459e+0f, 2.5920e+0f, 2.2321e+0f);
//        //Platinum
//        mc.createMetalMaterial("objfile_default_material", nullptr, "objfile_default_reflectance",
//                               2.54319f, 2.1202f, 1.8117f, 4.4853e+0f, 3.6989e+0f, 3.0588e+0f);
        
        
        std::vector<Object> objects;
        std::vector<Material> materials;
        std::map<std::string, uint32_t> matIdx;
        Object* curObject = nullptr;
        Object::Group* curGroup = nullptr;
        std::string curGroupName;
        uint32_t smooth = 0;
        std::string curMatName = "";
        bool phaseF = false;
        while (ifs && std::getline(ifs, sLine)) {
            //最初の空白を削除。
            sLine.erase(0, sLine.find_first_not_of("　 ¥t"));
            
            if (phaseF && sLine[0] != 'f')
                phaseF = false;
            
            switch (sLine[0]) {
                case 'o': {
                    char name[256];
                    if (sscanf(sLine.c_str(), "o %s", name) != 1)
                        break;
                    
                    objects.emplace_back();
                    curObject = &objects.back();
                    curObject->name = name;
                    break;
                }
                case 'g': {
                    char name[256];
                    if (sscanf(sLine.c_str(), "g %s", name) != 1)
                        break;
                    
                    curGroupName = name;
                    break;
                }
                case 's': {
                    if (sLine.compare(0, 5, "s off") == 0) {
                        smooth = 0;
                        break;
                    }
                    uint32_t s;
                    if (sscanf(sLine.c_str(), "s %u", &s) != 1)
                        break;
                    smooth = s;
                        
                    break;
                }
                case 'v':
                    switch (sLine[1]) {
                        case ' ':
                            float px, py, pz;
                            sscanf(sLine.c_str(), "v %f %f %f", &px, &py, &pz);
                            curObject->positions.push_back(px);
                            curObject->positions.push_back(py);
                            curObject->positions.push_back(pz);
                            break;
                        case 't':
                            float u, v;
                            sscanf(sLine.c_str(), "vt %f %f", &u, &v);
                            curObject->uvs.push_back(u);
                            curObject->uvs.push_back(1.0f - v);
                            break;
                        case 'n':
                            float nx, ny, nz;
                            sscanf(sLine.c_str(), "vn %f %f %f", &nx, &ny, &nz);
                            curObject->normals.push_back(nx);
                            curObject->normals.push_back(ny);
                            curObject->normals.push_back(nz);
                            break;
                        default:
                            break;
                    }
                    break;
                case 'f': {
                    uint32_t p0, p1, p2;
                    uint32_t uv0, uv1, uv2;
                    uint32_t ns0, ns1, ns2;
                    if (phaseF == false) {
                        phaseF = true;
                        
                        curObject->groups.emplace_back();
                        curGroup = &curObject->groups.back();
                        curGroup->name = curGroupName;
                        curGroup->smooth = smooth;
                        curGroup->material = curMatName;
                        
                        if (sscanf(sLine.c_str(), "f %u/%u/%u %u/%u/%u %u/%u/%u",
                                   &p0, &uv0, &ns0, &p1, &uv1, &ns1, &p2, &uv2, &ns2) == 9)
                            curGroup->format = Object::Group::Format::VTN;
                        else if (sscanf(sLine.c_str(), "f %u//%u %u//%u %u//%u",
                                        &p0, &ns0, &p1, &ns1, &p2, &ns2) == 6)
                            curGroup->format = Object::Group::Format::VN;
                    }
                    switch (curGroup->format) {
                        case Object::Group::Format::VTN:
                            sscanf(sLine.c_str(), "f %u/%u/%u %u/%u/%u %u/%u/%u",
                                   &p0, &uv0, &ns0, &p1, &uv1, &ns1, &p2, &uv2, &ns2);
                            p0 -= 1; p1 -= 1; p2 -= 1;
                            uv0 -= 1; uv1 -= 1; uv2 -= 1;
                            ns0 -= 1; ns1 -= 1; ns2 -= 1;
                            curGroup->posIndices.push_back(p0); curGroup->posIndices.push_back(p1); curGroup->posIndices.push_back(p2);
                            curGroup->uvIndices.push_back(uv0); curGroup->uvIndices.push_back(uv1); curGroup->uvIndices.push_back(uv2);
                            curGroup->nrmIndices.push_back(ns0); curGroup->nrmIndices.push_back(ns1); curGroup->nrmIndices.push_back(ns2);
                            break;
                        case Object::Group::Format::VN:
                            sscanf(sLine.c_str(), "f %u//%u %u//%u %u//%u",
                                   &p0, &ns0, &p1, &ns1, &p2, &ns2);
                            p0 -= 1; p1 -= 1; p2 -= 1;
                            ns0 -= 1; ns1 -= 1; ns2 -= 1;
                            curGroup->posIndices.push_back(p0); curGroup->posIndices.push_back(p1); curGroup->posIndices.push_back(p2);
                            curGroup->nrmIndices.push_back(ns0); curGroup->nrmIndices.push_back(ns1); curGroup->nrmIndices.push_back(ns2);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case 'm': {
                    if (sLine.compare(0, 6, "mtllib") == 0) {
                        char mtlFileName[256];
                        if (sscanf(sLine.c_str(), "mtllib %s", mtlFileName) != 1)
                            break;
                        std::string mtlFullPath = directory + mtlFileName;
                        loadMtl(mtlFullPath.c_str(), &materials, &matIdx);
                    }
                    break;
                }
                case 'u': {
                    if (sLine.compare(0, 6, "usemtl") == 0) {
                        char mtlName[256];
                        if (sscanf(sLine.c_str(), "usemtl %s", mtlName) != 1)
                            break;
                        curMatName = mtlName;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        
        ifs.close();
        
//        for (int i = 0; i < objects.size(); ++i) {
//            Object &obj = objects[i];
//            printf("o %s\n", obj.name.c_str());
//            for (int j = 0; j < obj.positions.size() / 3; ++j) {
//                printf("v %.6f %.6f %.6f\n", obj.positions[j * 3 + 0], obj.positions[j * 3 + 1], obj.positions[j * 3 + 2]);
//            }
//            for (int j = 0; j < obj.uvs.size() / 2; ++j) {
//                printf("vt %.6f %.6f\n", obj.uvs[j * 2 + 0], obj.uvs[j * 2 + 1]);
//            }
//            for (int j = 0; j < obj.normals.size() / 3; ++j) {
//                printf("vn %.6f %.6f %.6f\n", obj.normals[j * 3 + 0], obj.normals[j * 3 + 1], obj.normals[j * 3 + 2]);
//            }
//            for (int j = 0; j < obj.groups.size(); ++j) {
//                Object::Group &g = obj.groups[j];
//                printf("g %s\n", g.name.c_str());
//                printf("usemtl %s\n", g.material.c_str());
//                printf("s ");
//                if (g.smooth == 0)
//                    printf("off\n");
//                else
//                    printf("%u\n", g.smooth);
//                
//                for (int k = 0; k < g.posIndices.size() / 3; ++k) {
//                    if (g.format == Object::Group::Format::VTN) {
//                        printf("f %u/%u/%u %u/%u/%u %u/%u/%u\n",
//                               g.posIndices[k * 3 + 0], g.uvIndices[k * 3 + 0], g.nrmIndices[k * 3 + 0],
//                               g.posIndices[k * 3 + 1], g.uvIndices[k * 3 + 1], g.nrmIndices[k * 3 + 1],
//                               g.posIndices[k * 3 + 2], g.uvIndices[k * 3 + 2], g.nrmIndices[k * 3 + 2]);
//                    }
//                    else if (g.format == Object::Group::Format::VN) {
//                        printf("f %u//%u %u//%u %u//%u\n",
//                               g.posIndices[k * 3 + 0], g.uvIndices[k * 3 + 0], g.nrmIndices[k * 3 + 0],
//                               g.posIndices[k * 3 + 2], g.uvIndices[k * 3 + 2], g.nrmIndices[k * 3 + 2]);
//                    }
//                }
//            }
//            printf("\n");
//        }
        
        scene->beginObject();
        
        for (int i = 0; i < objects.size(); ++i) {
            Object &obj = objects[i];
            for (int j = 0; j < obj.positions.size() / 3; ++j)
                scene->addVertex(obj.positions[j * 3 + 0], obj.positions[j * 3 + 1], obj.positions[j * 3 + 2]);
            for (int j = 0; j < obj.uvs.size() / 2; ++j)
                scene->addUV(obj.uvs[j * 2 + 0], obj.uvs[j * 2 + 1]);
            for (int j = 0; j < obj.normals.size() / 3; ++j)
                scene->addNormal(obj.normals[j * 3 + 0], obj.normals[j * 3 + 1], obj.normals[j * 3 + 2]);
            
            for (int j = 0; j < obj.groups.size(); ++j) {
                Object::Group &g = obj.groups[j];
                Material &mat = materials[matIdx[g.material]];
                std::string diffuseTexName = g.name + "_" + mat.name + "_Kd";
                bool hasAlpha = false;
                if (mat.mapKd != "")
                    mc.createImageTexture(diffuseTexName.c_str(), mat.mapKd.c_str(), &hasAlpha);
                else
                    mc.createFloat3ConstantTexture(diffuseTexName.c_str(), mat.Kd[0], mat.Kd[1], mat.Kd[2]);
                
                std::string sigmaTexName = g.name + "_" + mat.name + "_Kd_sigma";
                mc.createFloatConstantTexture(sigmaTexName.c_str(), 0.0f);
                
                std::string matteMatName = g.name + "_" + mat.name;
                mc.createMatteMaterial(matteMatName.c_str(), nullptr, diffuseTexName.c_str(), sigmaTexName.c_str());
                
                for (int k = 0; k < g.posIndices.size() / 3; ++k) {
                    if (g.format == Object::Group::Format::VTN) {
                        scene->addFace(Face::make_P_N_UV(g.posIndices[k * 3 + 0], g.posIndices[k * 3 + 1], g.posIndices[k * 3 + 2],
                                                         g.nrmIndices[k * 3 + 0], g.nrmIndices[k * 3 + 1], g.nrmIndices[k * 3 + 2],
                                                         g.uvIndices[k * 3 + 0], g.uvIndices[k * 3 + 1], g.uvIndices[k * 3 + 2],
                                                         scene->idxOfMat(matteMatName.c_str()), UINT16_MAX, UINT32_MAX));
                    }
                    else if (g.format == Object::Group::Format::VN) {
                        scene->addFace(Face::make_P_N(g.posIndices[k * 3 + 0], g.posIndices[k * 3 + 1], g.posIndices[k * 3 + 2],
                                                      g.nrmIndices[k * 3 + 0], g.nrmIndices[k * 3 + 1], g.nrmIndices[k * 3 + 2],
                                                      scene->idxOfMat(matteMatName.c_str()), UINT16_MAX, UINT32_MAX));
                    }
                }
            }
        }
        
        scene->endObject();
        
        return true;
    }
}

bool loadModel(const char* fileName, Scene* scene) {
    std::string sFileName = fileName;
    
    size_t extPos = sFileName.find_first_of(".");
    if (extPos == std::string::npos)
        return false;
    
    std::string sExt = sFileName.substr(extPos + 1);
    if (!sExt.compare("obj")) {
        OBJ::load(fileName, scene);
    }
    
    return false;
}
