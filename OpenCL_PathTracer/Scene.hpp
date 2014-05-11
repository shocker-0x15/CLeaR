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

class Scene {
public:
    std::vector<cl_float3> vertices{};
    std::vector<cl_float3> normals{};
    std::vector<cl_float3> tangents{};
    std::vector<cl_float2> uvs{};
    std::vector<Face> faces{};
    std::vector<uint32_t> lights{};
    std::vector<uint8_t> materialsData{};
    std::map<std::string, size_t> materialsRef;
    std::vector<uint8_t> lightPropsData{};
    std::map<std::string, size_t> lightPropsRef;
    std::vector<uint8_t> texturesData{};
    std::map<std::string, size_t> texturesRef;
    BVH bvh;
    bool immediateMode;
    size_t idxBaseVertices;
    size_t idxBaseNormals;
    size_t idxBaseTangents;
    size_t idxBaseUVs;
    
    Scene() {
        immediateMode = false;
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
            if (face.ns0 != UINT32_MAX) {
                trueFace.ns0 += idxBaseNormals;
                trueFace.ns1 += idxBaseNormals;
                trueFace.ns2 += idxBaseNormals;
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
        if (face.lightPtr != UINT16_MAX)
            lights.push_back((uint32_t)faces.size() - 1);
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
    void* rawLights() {
        return lights.data();
    }
    void* rawMaterialsData() {
        return materialsData.data();
    }
    void* rawLightPropsData() {
        return lightPropsData.data();
    }
    void* rawTexturesData() {
        return texturesData.data();
    }
    void* rawBVHNodes() {
        return bvh.nodes.data();
    }
    
    size_t numVertices() {
        return vertices.size();
    }
    size_t numNormals() {
        return normals.size();
    }
    size_t numTangents() {
        return tangents.size();
    }
    size_t numUVs() {
        return uvs.size();
    }
    size_t numFaces() {
        return faces.size();
    }
    size_t numLights() {
        return lights.size();
    }
    size_t sizeOfMaterialsData() {
        return materialsData.size() * sizeof(uint8_t);
    }
    size_t sizeOfLightPropsData() {
        return lightPropsData.size() * sizeof(uint8_t);
    }
    size_t sizeOfTexturesData() {
        return texturesData.size() * sizeof(uint8_t);
    }
    size_t sizeOfBVHNodes() {
        return bvh.nodes.size() * sizeof(BVHNode);
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
    
    void build() {
        for (int i = 0; i < faces.size(); ++i) {
            BBox bb{vertices[faces[i].p0]};
            bb.unionP(vertices[faces[i].p1]);
            bb.unionP(vertices[faces[i].p2]);
            bvh.addLeaf(bb);
        }
        bvh.build();
    }
};

#endif
