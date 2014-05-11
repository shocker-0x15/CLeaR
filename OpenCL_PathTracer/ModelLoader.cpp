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
    
    createFloat3ConstantTexture(scene, "objfile_default_reflectance", 0.5f, 0.5f, 0.5f);
    createFloatConstantTexture(scene, "objfile_default_sigma", 0.0f);
    createDiffuseMaterial(scene, "objfile_default_material",
                          scene->idxOfTex("objfile_default_reflectance"),
                          scene->idxOfTex("objfile_default_sigma"));
    
    bool smoothShading = false;
    while (ifs && std::getline(ifs, sLine)) {
        //最初の空白を削除。
        sLine.erase(0, sLine.find_first_not_of("　 ¥t"));

        switch (sLine[0]) {
            case 'o':
                scene->endObject();
                scene->beginObject();
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
                    scene->addFace(Face::make_pnt(p0, p1, p2, ns0, ns1, ns2, uv0, uv1, uv2, scene->idxOfMat("objfile_default_material")));
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
