//
//  Scene.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/05/09.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Scene_h
#define OpenCL_PathTracer_Scene_h

#include <vector>
#include <string>
#include <map>
#include "Face.hpp"
#include "BVH.hpp"
#include "clUtility.hpp"
#include <cassert>

//8bytes
typedef struct {
    uint8_t atInfinity; uint8_t dum0[3];
    uint32_t reference;
} LightInfo;

class Scene {
public:
    std::vector<cl_float3> vertices;
    std::vector<cl_float3> normals;
    std::vector<cl_float3> tangents;
    std::vector<cl_float2> uvs;
    std::vector<Face> faces;
    std::vector<LightInfo> lightInfos;
    std::vector<float> lightPowers;
    std::vector<uint8_t> materialsData;
    std::map<std::string, size_t> materialsRef;
    std::map<std::string, size_t> lightPropsRef;
    std::vector<uint8_t> texturesData;
    std::map<std::string, size_t> texturesRef;
    std::vector<uint8_t> otherResouces;
    std::map<std::string, size_t> othersRef;
    BVH bvh;
    
    bool immediateMode;
    size_t idxBaseVertices;
    size_t idxBaseNormals;
    size_t idxBaseTangents;
    size_t idxBaseUVs;
    
    Scene() {
        immediateMode = false;
        //idx of Camera, Environment, LightPowerCDF
        otherResouces.insert(otherResouces.end(), sizeof(uint32_t) * 3, 0);
    }
    
    void addVertex(float x, float y, float z) {
        cl_float3 f3Val{x, y, z};
        vertices.push_back(f3Val);
    }
    void addNormal(float x, float y, float z) {
        cl_float3 f3Val{x, y, z};
        normals.push_back(f3Val);
    }
    void addTangent(float x, float y, float z) {
        cl_float3 f3Val{x, y, z};
        tangents.push_back(f3Val);
    }
    void addUV(float u, float v) {
        cl_float2 f2Val{u, v};
        uvs.push_back(f2Val);
    }
    void addFace(const Face &face) {
        if (immediateMode) {
            Face trueFace = face;
            trueFace.p0 += idxBaseVertices;
            trueFace.p1 += idxBaseVertices;
            trueFace.p2 += idxBaseVertices;
            if (face.vn0 != UINT32_MAX) {
                trueFace.vn0 += idxBaseNormals;
                trueFace.vn1 += idxBaseNormals;
                trueFace.vn2 += idxBaseNormals;
            }
            if (face.vt0 != UINT32_MAX) {
                trueFace.vt0 += idxBaseTangents;
                trueFace.vt1 += idxBaseTangents;
                trueFace.vt2 += idxBaseTangents;
            }
            if (face.uv0 != UINT32_MAX) {
                trueFace.uv0 += idxBaseUVs;
                trueFace.uv1 += idxBaseUVs;
                trueFace.uv2 += idxBaseUVs;
            }
            faces.push_back(trueFace);
        }
        else {
            faces.push_back(face);
        }
        if (face.lightPtr != UINT16_MAX) {
            LightInfo lInfo;
            lInfo.atInfinity = 0;
            lInfo.reference = (uint32_t)faces.size() - 1;
            lightInfos.push_back(lInfo);
            
            lightPowers.push_back(1.0f);// 適当。ライトの出力に比例した値にすべき。
        }
    }
    bool addMaterial(size_t idx, const char* name) {
        std::pair<std::map<std::string, size_t>::iterator, bool> ret = materialsRef.insert(std::pair<std::string, size_t>(name, idx));
        return ret.second;
    }
    bool addLightProp(size_t idx, const char* name) {
        std::pair<std::map<std::string, size_t>::iterator, bool> ret = lightPropsRef.insert(std::pair<std::string, size_t>(name, idx));
        return ret.second;
    }
    bool addTexture(size_t idx, const char* name) {
        std::pair<std::map<std::string, size_t>::iterator, bool> ret = texturesRef.insert(std::pair<std::string, size_t>(name, idx));
        return ret.second;
    }
    bool addOtherResouce(size_t idx, const char* name) {
        std::pair<std::map<std::string, size_t>::iterator, bool> ret = othersRef.insert(std::pair<std::string, size_t>(name, idx));
        return ret.second;
    }
    bool setCamera(size_t idx) {
        std::pair<std::map<std::string, size_t>::iterator, bool> ret = othersRef.insert(std::pair<std::string, size_t>("Camera", idx));
        return ret.second;
    }
    bool setEnvironment(size_t idx) {
        std::pair<std::map<std::string, size_t>::iterator, bool> ret = othersRef.insert(std::pair<std::string, size_t>("Environment", idx));
        return ret.second;
    }
    
    void beginObject() {
        immediateMode = true;
        idxBaseVertices = vertices.size();
        idxBaseNormals = normals.size();
        idxBaseTangents = tangents.size();
        idxBaseUVs = uvs.size();
    }
    void endObject() {
        immediateMode = false;
    }
    
    void* rawVertices() {
        return vertices.data();
    }
    void* rawNormals() {
        return normals.data();
    }
    void* rawTangents() {
        return tangents.data();
    }
    void* rawUVs() {
        return uvs.data();
    }
    void* rawFaces() {
        return faces.data();
    }
    void* rawLightInfos() {
        return lightInfos.data();
    }
    void* rawMaterialsData() {
        return materialsData.data();
    }
    void* rawTexturesData() {
        return texturesData.data();
    }
    void* rawBVHNodes() {
        return bvh.nodes.data();
    }
    void* rawOtherResources() {
        return otherResouces.data();
    }
    
    size_t numVertices() const {
        return vertices.size();
    }
    size_t numNormals() const {
        return normals.size();
    }
    size_t numTangents() const {
        return tangents.size();
    }
    size_t numUVs() const {
        return uvs.size();
    }
    size_t numFaces() const {
        return faces.size();
    }
    size_t numLights() const {
        return lightInfos.size();
    }
    size_t sizeOfMaterialsData() const {
        return materialsData.size() * sizeof(uint8_t);
    }
    size_t sizeOfTexturesData() const {
        return texturesData.size() * sizeof(uint8_t);
    }
    size_t sizeOfBVHNodes() const {
        return bvh.nodes.size() * sizeof(BVHNode);
    }
    size_t sizeOfOtherResouces() const {
        return otherResouces.size();
    }
    
    size_t idxOfMat(const std::string &name) {
        assert(materialsRef.count(name) == 1);
        return materialsRef[name];
    }
    size_t idxOfLight(const std::string &name) {
        assert(lightPropsRef.count(name) == 1);
        return lightPropsRef[name];
    }
    size_t idxOfTex(const std::string &name) {
        assert(texturesRef.count(name) == 1);
        return texturesRef[name];
    }
    size_t idxOfOther(const std::string &name) {
        assert(othersRef.count(name) == 1);
        return othersRef[name];
    }
    
    void calcLightPowerCDF() {
        auto refOthers = &otherResouces;
        uint64_t lpCDFHead = addDataAligned<cl_uint>(refOthers, (cl_uint)lightPowers.size());
        
        std::vector<float> PMF(lightPowers.size());
        std::vector<float> CDF(lightPowers.size());
        CDF[0] = lightPowers[0];
        for (int i = 1; i < lightPowers.size(); ++i)
            CDF[i] = CDF[i - 1] + lightPowers[i];
        float sum = CDF.back();
        for (int i = 0; i < CDF.size(); ++i) {
            CDF[i] /= sum;
            PMF[i] = lightPowers[i] / sum;
        }
        addDataAligned(refOthers, CDF.data(), sizeof(float) * CDF.size(), sizeof(float));
        addDataAligned(refOthers, PMF.data(), sizeof(float) * PMF.size(), sizeof(float));
        
        addOtherResouce(lpCDFHead, "LightPowerDistribution");
    }
    
    void build() {
        calcLightPowerCDF();
        
        for (int i = 0; i < faces.size(); ++i) {
            BBox bb{vertices[faces[i].p0]};
            bb.unionP(vertices[faces[i].p1]);
            bb.unionP(vertices[faces[i].p2]);
            bvh.addLeaf(bb);
        }
        bvh.build();
        
        *(uint32_t*)&otherResouces[0] = (uint32_t)idxOfOther("Camera");
        *(uint32_t*)&otherResouces[4] = (uint32_t)idxOfOther("Environment");
        *(uint32_t*)&otherResouces[8] = (uint32_t)idxOfOther("LightPowerDistribution");
    }
};

#endif
