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
#include "Distribution.hpp"
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
    std::map<std::string, uint64_t> materialsRef;
    std::map<std::string, uint64_t> lightPropsRef;
    std::vector<uint8_t> texturesData;
    std::map<std::string, uint64_t> texturesRef;
    std::map<std::string, uint64_t> texFilesDatabase;
    std::vector<uint8_t> otherResouces;
    std::map<std::string, uint64_t> othersRef;
    BVH bvh;
    
    bool immediateMode;
    uint64_t idxBaseVertices;
    uint64_t idxBaseNormals;
    uint64_t idxBaseTangents;
    uint64_t idxBaseUVs;
    
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
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = materialsRef.insert(std::pair<std::string, uint64_t>(name, idx));
        return ret.second;
    }
    bool addLightProp(size_t idx, const char* name) {
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = lightPropsRef.insert(std::pair<std::string, uint64_t>(name, idx));
        return ret.second;
    }
    bool addTexture(size_t idx, const char* name) {
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = texturesRef.insert(std::pair<std::string, uint64_t>(name, idx));
        return ret.second;
    }
    bool addTexFileToDB(size_t idx, const char* filename) {
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = texFilesDatabase.insert(std::pair<std::string, uint64_t>(filename, idx));
        return ret.second;
    }
    bool addOtherResouce(size_t idx, const char* name) {
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = othersRef.insert(std::pair<std::string, uint64_t>(name, idx));
        return ret.second;
    }
    bool setCamera(size_t idx) {
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = othersRef.insert(std::pair<std::string, uint64_t>("Camera", idx));
        return ret.second;
    }
    bool setEnvironment(size_t idx) {
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret = othersRef.insert(std::pair<std::string, uint64_t>("Environment", idx));
        LightInfo lInfo;
        lInfo.atInfinity = 1;
        lInfo.reference = UINT32_MAX;
        lightInfos.push_back(lInfo);
        lightPowers.push_back(1.0f);// 適当。ライトの出力に比例した値にすべき。
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
    
    uint64_t numVertices() const {
        return vertices.size();
    }
    uint64_t numNormals() const {
        return normals.size();
    }
    uint64_t numTangents() const {
        return tangents.size();
    }
    uint64_t numUVs() const {
        return uvs.size();
    }
    uint64_t numFaces() const {
        return faces.size();
    }
    uint64_t numLights() const {
        return lightInfos.size();
    }
    uint64_t sizeOfMaterialsData() const {
        return materialsData.size() * sizeof(uint8_t);
    }
    uint64_t sizeOfTexturesData() const {
        return texturesData.size() * sizeof(uint8_t);
    }
    uint64_t sizeOfBVHNodes() const {
        return bvh.nodes.size() * sizeof(BVHNode);
    }
    uint64_t sizeOfOtherResouces() const {
        return otherResouces.size();
    }
    
    uint64_t idxOfMat(const std::string &name) {
        assert(materialsRef.count(name) == 1);
        return materialsRef[name];
    }
    uint64_t idxOfLight(const std::string &name) {
        assert(lightPropsRef.count(name) == 1);
        return lightPropsRef[name];
    }
    uint64_t idxOfTex(const std::string &name) {
        assert(texturesRef.count(name) == 1);
        return texturesRef[name];
    }
    bool texFilehasLoaded(const std::string &filename, uint64_t* idx) {
        bool ret = texFilesDatabase.count(filename) == 1;
        *idx = ret ? texFilesDatabase[filename] : 0;
        return ret;
    }
    uint64_t idxOfOther(const std::string &name) {
        assert(othersRef.count(name) == 1);
        return othersRef[name];
    }
    
    void calcLightPowerDistribution() {
        auto refOthers = &otherResouces;
        uint64_t lpDistHead = fillZerosAligned(refOthers, sizeof(Discrete1D), 4);
        Discrete1D dist;
        dist.head._type = DistributionType::Discrete1D;
        dist.numItems = (cl_uint)lightPowers.size();
        
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
        uint64_t lpCDFHead = addDataAligned(refOthers, CDF.data(), sizeof(float) * CDF.size(), sizeof(float));
        uint64_t lpPMFHead = addDataAligned(refOthers, PMF.data(), sizeof(float) * PMF.size(), sizeof(float));
        dist.offsetCDF = (int32_t)lpCDFHead - (int32_t)lpDistHead;
        dist.offsetPMF = (int32_t)lpPMFHead - (int32_t)lpDistHead;
        memcpy(refOthers->data() + lpDistHead, &dist, sizeof(Discrete1D));
        
        addOtherResouce(lpDistHead, "LightPowerDistribution");
    }
    
    void build();
};

#endif
