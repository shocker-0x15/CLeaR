//
//  main.cpp
//  PathTracer
//
//  Created by 渡部 心 on 2014/04/17.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include <cstdio>
#include <vector>
#include "Matrix4f.hpp"
#include "XORShift.hpp"
#include <fstream>
#define __CL_ENABLE_EXCEPTIONS
#include "cl12.hpp"
#include "clUtility.hpp"
#include "CreateFunctions.hpp"
#include "ModelLoader.hpp"
#include "BVH.hpp"
#include "Face.hpp"

CL_CALLBACK void completeTile(cl_event ev, cl_int exec_status, void* user_data) {
    printf("*");
}

void saveBMP(const char* filename, const void* pixels, uint32_t width, uint32_t height) {
    uint32_t byteWidth = 3 * width + width % 4;
    const uint32_t FILE_HEADER_SIZE = 14;
    const uint32_t INFO_HEADER_SIZE = 40;
    const uint32_t HEADER_SIZE = FILE_HEADER_SIZE + INFO_HEADER_SIZE;
    uint8_t header[HEADER_SIZE];
    uint32_t fileSize = byteWidth * height + HEADER_SIZE;
    uint32_t dataOffset = HEADER_SIZE;
    uint16_t numPlanes = 1;
    uint16_t numBits = 24;
    uint32_t compress = 0;
    uint32_t dataSize = byteWidth * height;
    uint32_t xppm = 1;
    uint32_t yppm = 1;
    
    FILE* fp = fopen(filename, "wb");
    if (fp == nullptr)
        return;
    
    memset(header, 0, HEADER_SIZE);
    header[0] = 'B';
	header[1] = 'M';
	memcpy(header + 2, &fileSize, sizeof(uint32_t));
	memcpy(header + 10, &dataOffset, sizeof(uint32_t));
    
    memcpy(header + 14, &INFO_HEADER_SIZE, sizeof(uint32_t));
	memcpy(header + 18, &width, sizeof(uint32_t));
	memcpy(header + 22, &height, sizeof(uint32_t));
	memcpy(header + 26, &numPlanes, sizeof(uint16_t));
	memcpy(header + 28, &numBits, sizeof(uint16_t));
	memcpy(header + 30, &compress, sizeof(uint32_t));
	memcpy(header + 34, &dataSize, sizeof(uint32_t));
	memcpy(header + 38, &xppm, sizeof(uint32_t));
	memcpy(header + 42, &yppm, sizeof(uint32_t));
    
    fwrite(header, sizeof(uint8_t), HEADER_SIZE, fp);
    fwrite(pixels, sizeof(uint8_t), dataSize, fp);
    
    fclose(fp);
}

XORShiftRandom32 rng{215363872};
const int g_width = 1024;
const int g_height = 1024;
std::vector<cl_float3> vertices{};
std::vector<cl_float3> normals{};
std::vector<cl_float3> binormals{};
std::vector<cl_float2> uvs{};
std::vector<Face> faces{};
std::vector<uint32_t> lights{};
std::vector<uint8_t> materialsData{};
std::vector<uint8_t> lightsData{};
std::vector<uint8_t> texturesData{};
std::vector<uint32_t> g_randStates{};
std::vector<cl_float3> g_pixels{};
BVH bvh;

void addFace(const Face &face) {
    faces.push_back(face);
    if (face.lightPtr != UINT16_MAX)
        lights.push_back((uint32_t)faces.size() - 1);
}

void buildScene() {
    std::vector<uint16_t> matIdx;
    std::vector<uint16_t> lgtIdx;
    
    g_randStates.resize(g_width * g_height * 4);
    g_pixels.resize(g_width * g_height);
    vertices.reserve(64);
    normals.reserve(64);
    binormals.reserve(64);
    uvs.reserve(64);
    faces.reserve(64);
    lights.reserve(64);
    
    materialsData.reserve(64);
    lightsData.reserve(64);
    
    for (int i = 0; i < g_height; ++i) {
        for (int j = 0; j < g_width; ++j) {
            uint32_t idxBase = i * g_width + j;
            g_pixels[idxBase].s0 = g_pixels[idxBase].s1 = g_pixels[idxBase].s2 = 0.0f;
            
            uint32_t seed = rng.getUInt();
            for (uint32_t k = 0; k < 4; ++k) {
                g_randStates[idxBase * 4 + k] = seed = 1812433253U * (seed ^ (seed >> 30)) + k;
            }
        }
    }
    
    //部屋
    vertices.push_back(makeCLfloat3(-1.0f, -1.0f, -1.0f));
    vertices.push_back(makeCLfloat3( 1.0f, -1.0f, -1.0f));
    vertices.push_back(makeCLfloat3( 1.0f,  1.0f, -1.0f));
    vertices.push_back(makeCLfloat3(-1.0f,  1.0f, -1.0f));
    vertices.push_back(makeCLfloat3(-1.0f, -1.0f,  1.0f));
    vertices.push_back(makeCLfloat3( 1.0f, -1.0f,  1.0f));
    vertices.push_back(makeCLfloat3( 1.0f,  1.0f,  1.0f));
    vertices.push_back(makeCLfloat3(-1.0f,  1.0f,  1.0f));
    
    uvs.push_back(makeCLfloat2(0.0f, 0.0f));
    uvs.push_back(makeCLfloat2(5.0f, 0.0f));
    uvs.push_back(makeCLfloat2(5.0f, 5.0f));
    uvs.push_back(makeCLfloat2(0.0f, 5.0f));
    
    uvs.push_back(makeCLfloat2(0.0f, 1.0f));
    uvs.push_back(makeCLfloat2(1.0f, 1.0f));
    uvs.push_back(makeCLfloat2(1.0f, 0.0f));
    uvs.push_back(makeCLfloat2(0.0f, 0.0f));
    
    {
        uint64_t matHead;
        
        //mat0
        matHead = addDataAligned<cl_uchar>(&materialsData, 1);//Diffuse
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloat3ConstantTexture(&texturesData, 0.75f, 0.25f, 0.25f));//Reflectance
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.0f));//Roughness
        matIdx.push_back((uint16_t)matHead);
        
        //mat1
        matHead = addDataAligned<cl_uchar>(&materialsData, 1);
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloat3ConstantTexture(&texturesData, 0.25f, 0.25f, 0.75f));
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.0f));
        matIdx.push_back((uint16_t)matHead);
        
        //mat2
        matHead = addDataAligned<cl_uchar>(&materialsData, 1);
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloat3ConstantTexture(&texturesData, 0.75f, 0.75f, 0.75f));
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.0f));
        matIdx.push_back((uint16_t)matHead);
        
        //mat3
        //        matHead = addDataAligned<cl_uchar>(&materialsData, 1);
        //        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloat3ConstantTexture(&texturesData, 0.75f, 0.75f, 0.75f));
        //        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.0f));
        //        matIdx.push_back((uint16_t)matHead);
        
        //        matHead = addDataAligned<cl_uchar>(&materialsData, 2);//Specular
        //        addDataAligned<cl_uchar>(&materialsData, 2);//procedural texture
        //        addDataAligned<cl_uchar>(&materialsData, 0);//checker board
        //        f3Val.s0 = 0.75f; f3Val.s1 = 0.75f; f3Val.s2 = 0.75f;
        //        addDataAligned<cl_float3>(&materialsData, f3Val);
        //        f3Val.s0 = 0.25f; f3Val.s1 = 0.25f; f3Val.s2 = 0.25f;
        //        addDataAligned<cl_float3>(&materialsData, f3Val);
        //        addDataAligned<cl_uchar>(&materialsData, 0);//NoOp
        //        matIdx.push_back((uint16_t)matHead);
        
        matHead = addDataAligned<cl_uchar>(&materialsData, 4);//New Ward
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createCheckerBoardTexture(&texturesData, 0.75f, 0.75f, 0.75f, 0.25f, 0.25f, 0.25f));
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.25f));//Anisotropy X
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.25f));//Anisotropy Y
        matIdx.push_back((uint16_t)matHead);
        
        //mat4
        matHead = addDataAligned<cl_uchar>(&materialsData, 1);
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createImageTexture(&texturesData, "images/pika_flat.png"));
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.0f));
        matIdx.push_back((uint16_t)matHead);
    }
    
    addFace(Face::make_pt(1, 0, 3, 5, 4, 7, matIdx[4])); addFace(Face::make_pt(1, 3, 2, 5, 7, 6, matIdx[4]));
    //    addFace(Face(4, 5, 6, matIdx[3])); addFace(Face(4, 6, 7, matIdx[3]));
    addFace(Face::make_p(0, 4, 7, matIdx[0])); addFace(Face::make_p(0, 7, 3, matIdx[0]));
    addFace(Face::make_p(5, 1, 2, matIdx[1])); addFace(Face::make_p(5, 2, 6, matIdx[1]));
    //    addFace(Face::make_p(4, 5, 1, matIdx[2])); addFace(Face::make_p(4, 1, 0, matIdx[2]));
    addFace(Face::make_pt(4, 5, 1, 0, 1, 2, matIdx[3])); addFace(Face::make_pt(4, 1, 0, 0, 2, 3, matIdx[3]));
    addFace(Face::make_p(2, 3, 7, matIdx[2])); addFace(Face::make_p(2, 7, 6, matIdx[2]));
    
    
    //光源
    vertices.push_back(makeCLfloat3(-0.25f, 0.9999f, -0.25f));
    vertices.push_back(makeCLfloat3(0.25f, 0.9999f, -0.25f));
    vertices.push_back(makeCLfloat3(0.25f, 0.9999f, 0.25f));
    vertices.push_back(makeCLfloat3(-0.25f, 0.9999f, 0.25f));
    
    {
        uint64_t matHead;
        
        //mat5
        matHead = addDataAligned<cl_uchar>(&materialsData, 1);
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloat3ConstantTexture(&texturesData, 0.9f, 0.9f, 0.9f));
        addDataAligned<cl_uint>(&materialsData, (cl_uint)createFloatConstantTexture(&texturesData, 0.0f));
        matIdx.push_back((uint16_t)matHead);
        
        //light0
        matHead = addDataAligned<cl_uchar>(&lightsData, 1);//1: Perfect Diffuse
        addDataAligned<cl_uint>(&lightsData, (cl_uint)createFloat3ConstantTexture(&texturesData, 30.0f, 30.0f, 30.0f));
        lgtIdx.push_back((uint16_t)matHead);
    }
    
    addFace(Face::make_p(8, 9, 10, matIdx[5], lgtIdx[0]));
    addFace(Face::make_p(8, 10, 11, matIdx[5], lgtIdx[0]));
    
    for (int i = 0; i < faces.size(); ++i) {
        BBox bb{vertices[faces[i].p0]};
        bb.unionP(vertices[faces[i].p1]);
        bb.unionP(vertices[faces[i].p2]);
        bvh.addLeaf(bb);
    }
    bvh.build();
}

int main(int argc, const char * argv[]) {
    const uint32_t sppOnce = 16;
    const uint32_t iterations = 2;
    
    buildScene();
    
    cl_int ret = CL_SUCCESS;
    try {
        std::ifstream ifs;
        cl::Platform platform;
        cl::Platform::get(&platform);
        
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        cl::Device device = devices[1];
        std::string deviceName;
        device.getInfo(CL_DEVICE_NAME, &deviceName);
        printf("%s\n", deviceName.c_str());
        
        cl_context_properties ctx_props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform(), 0};
        cl::Context context{device, ctx_props};
        
        ifs.open("pathtracer.cl");
        ifs.clear();
        ifs.seekg(0, std::ios::beg);
        std::string rawStrRendering{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        cl::Program::Sources srcRendering{1, std::make_pair(rawStrRendering.c_str(), rawStrRendering.length())};
        ifs.close();
        
        cl::Program programRendering{context, srcRendering};
        programRendering.build();
        std::string buildLog;
        programRendering.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("%s\n", buildLog.c_str());
        
        cl::Buffer buf_vertices{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, vertices.size() * sizeof(cl_float3), (void*)vertices.data(), nullptr};
        cl::Buffer buf_normals;
        if (normals.empty())
            buf_normals = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float3), nullptr, nullptr);
        else
            buf_normals = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, normals.size() * sizeof(cl_float3), (void*)normals.data(), nullptr);
        cl::Buffer buf_binormals;
        if (binormals.empty())
            buf_binormals = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float3), nullptr, nullptr);
        else
            buf_binormals = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, binormals.size() * sizeof(cl_float3), (void*)binormals.data(), nullptr);
        cl::Buffer buf_uvs;
        if (uvs.empty())
            buf_uvs = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float2), nullptr, nullptr);
        else
            buf_uvs = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, uvs.size() * sizeof(cl_float2), (void*)uvs.data(), nullptr);
        cl::Buffer buf_faces{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, faces.size() * sizeof(Face), (void*)faces.data(), nullptr};
        cl::Buffer buf_lights{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, lights.size() * sizeof(uint32_t), (void*)lights.data(), nullptr};
        cl::Buffer buf_materialsData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, materialsData.size() * sizeof(uint8_t), (void*)materialsData.data(), nullptr};
        cl::Buffer buf_lightsData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, lightsData.size() * sizeof(uint8_t), (void*)lightsData.data(), nullptr};
        cl::Buffer buf_texturesData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, texturesData.size() * sizeof(uint8_t), (void*)texturesData.data(), nullptr};
        cl::Buffer buf_BVHnodes{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, bvh.nodes.size() * sizeof(BVHNode), (void*)bvh.nodes.data(), nullptr};
        cl::Buffer buf_randStates{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_randStates.size() * sizeof(uint32_t), (void*)g_randStates.data(), nullptr};
        cl::Buffer buf_pixels{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_pixels.size() * sizeof(cl_float3), (void*)g_pixels.data(), nullptr};
        
        cl::Kernel kernelRendering{programRendering, "pathtracing"};
        kernelRendering.setArg(0, buf_vertices);
        kernelRendering.setArg(1, buf_normals);
        kernelRendering.setArg(2, buf_binormals);
        kernelRendering.setArg(3, buf_uvs);
        kernelRendering.setArg(4, buf_faces);
        kernelRendering.setArg(5, buf_lights);
        kernelRendering.setArg(6, (uint32_t)lights.size());
        kernelRendering.setArg(7, buf_materialsData);
        kernelRendering.setArg(8, buf_lightsData);
        kernelRendering.setArg(9, buf_texturesData);
        kernelRendering.setArg(10, buf_BVHnodes);
        kernelRendering.setArg(11, buf_randStates);
        kernelRendering.setArg(12, g_width);
        kernelRendering.setArg(13, g_height);
        kernelRendering.setArg(14, sppOnce);
        kernelRendering.setArg(15, buf_pixels);
        
        std::vector<cl::Event> eventList;
        cl::Event computeEvent;
        cl::Event readEvent;
        
        cl::CommandQueue queue{context, device};
        const int numTilesX = 8, numTilesY = 8;
        const int numTiles = numTilesX * numTilesY;
        cl::NDRange tile{g_width / numTilesX, g_height / numTilesY};
        cl::NDRange localSize{32, 32};
        for (int i = 0; i < iterations; ++i) {
            printf("[ %d ]", i);
            for (int j = 0; j < numTiles; ++j) {
                cl::NDRange offset{*tile * (j % numTilesX), *(tile + 1) * (j / numTilesX)};
                cl::Event ev;
                queue.enqueueNDRangeKernel(kernelRendering, offset, tile, localSize, nullptr, &ev);
//                ev.setCallback(CL_COMPLETE, completeTile);
            }
            queue.finish();
            printf("\n");
        }
        
//        queue.enqueueReadBuffer(buf_pixels, CL_TRUE, 0, g_height * g_width * sizeof(cl_float3), g_pixels.data(), &eventList, &readEvent);
        printf("rendering done!\n");
        
        
        ifs.open("tonemapping.cl");
        ifs.clear();
        ifs.seekg(0, std::ios::beg);
        std::string rawStrToneMapping{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        cl::Program::Sources srcToneMapping{1, std::make_pair(rawStrToneMapping.c_str(), rawStrToneMapping.length())};
        ifs.close();
        
        cl::Program programToneMapping{context, srcToneMapping};
        programToneMapping.build();
        
        uint32_t byteWidth = 3 * g_width + g_width % 4;
        uint8_t* LDRPixels = (uint8_t*)malloc(byteWidth * g_height);
        cl::Buffer buf_image{context, CL_MEM_WRITE_ONLY, (size_t)(byteWidth * g_height)};
        
        cl::Kernel kernelToneMapping{programToneMapping, "tonemapping16B"};
        kernelToneMapping.setArg(0, g_width);
        kernelToneMapping.setArg(1, g_height);
        kernelToneMapping.setArg(2, byteWidth);
        kernelToneMapping.setArg(3, sppOnce * iterations);
        kernelToneMapping.setArg(4, buf_pixels);
        kernelToneMapping.setArg(5, buf_image);
        
        queue.enqueueNDRangeKernel(kernelToneMapping, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueReadBuffer(buf_image, CL_TRUE, 0, byteWidth * g_height, LDRPixels, nullptr, nullptr);
        saveBMP("output.bmp", LDRPixels, g_width, g_height);
        free(LDRPixels);
    } catch (cl::Error error) {
        char err_str[64];
        printErrorFromCode(error.err(), err_str);
        fprintf(stderr, "ERROR: %s @ ", err_str);
        fprintf(stderr, "%s\n", error.what());
    }
    
    return 0;
}

