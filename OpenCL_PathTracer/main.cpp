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
#include "Scene.hpp"

#include "sim_pathtracer.h"

#define SIMULATION 0

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
Scene scene;
std::vector<uint32_t> g_randStates{};
std::vector<cl_float3> g_pixels{};

void buildScene() {
    addDataAligned<cl_uint>(&scene.camera, g_width);
    addDataAligned<cl_uint>(&scene.camera, g_height);
    cl_float16 f16Val = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 3.999f, 1.0f
    };
    addDataAligned<cl_float16>(&scene.camera, f16Val);// local to world transform
    addDataAligned<cl_uchar>(&scene.camera, 0);// perspective
    float fovY = 0.6435011088f;
    float aspect = 1;
    float near = 1;
    float far = 100;
    addDataAligned<cl_float>(&scene.camera, aspect * powf(tanf(fovY / 2), 2));
    addDataAligned<cl_float>(&scene.camera, 0.05f);// lens radius
    addDataAligned<cl_float>(&scene.camera, 4.0f);// object plane distance
    Matrix4f clipToCamera = Matrix4f(Vector4f(1 / (aspect * tanf(fovY / 2)), 0, 0, 0),
                                     Vector4f(0, 1 / tanf(fovY / 2), 0, 0),
                                     Vector4f(0, 0, -(far + near) / (far - near), -1),
                                     Vector4f(0, 0, -2 * far * near / (far - near), 0)).Invert();
    Matrix4f rasterToScreen = Matrix4f(Vector4f(2.0f / g_width, 0, 0, 0), // raster to screen
                                       Vector4f(0, -2.0f / g_height, 0, 0),
                                       Vector4f(0, 0, 2, 0),
                                       Vector4f(-1, 1, -1, 1));
    Matrix4f rasterToCamera = clipToCamera * rasterToScreen;
    f16Val.s0 = rasterToCamera.m00; f16Val.s4 = rasterToCamera.m01; f16Val.s8 = rasterToCamera.m02; f16Val.sc = rasterToCamera.m03;
    f16Val.s1 = rasterToCamera.m10; f16Val.s5 = rasterToCamera.m11; f16Val.s9 = rasterToCamera.m12; f16Val.sd = rasterToCamera.m13;
    f16Val.s2 = rasterToCamera.m20; f16Val.s6 = rasterToCamera.m21; f16Val.sa = rasterToCamera.m22; f16Val.se = rasterToCamera.m23;
    f16Val.s3 = rasterToCamera.m30; f16Val.s7 = rasterToCamera.m31; f16Val.sb = rasterToCamera.m32; f16Val.sf = rasterToCamera.m33;
    addDataAligned<cl_float16>(&scene.camera, f16Val);
    
    g_randStates.resize(g_width * g_height * 4);
    g_pixels.resize(g_width * g_height);
    
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
    
    MaterialCreator &mc = MaterialCreator::sharedInstance();
    mc.setScene(&scene);
    
    //部屋
    scene.beginObject();
    scene.addVertex(-1.0f, -1.0f, -1.0f);
    scene.addVertex( 1.0f, -1.0f, -1.0f);
    scene.addVertex( 1.0f,  1.0f, -1.0f);
    scene.addVertex(-1.0f,  1.0f, -1.0f);
    scene.addVertex(-1.0f, -1.0f,  1.0f);
    scene.addVertex( 1.0f, -1.0f,  1.0f);
    scene.addVertex( 1.0f,  1.0f,  1.0f);
    scene.addVertex(-1.0f,  1.0f,  1.0f);
    scene.addUV(0.0f, 0.0f);
    scene.addUV(5.0f, 0.0f);
    scene.addUV(5.0f, 5.0f);
    scene.addUV(0.0f, 5.0f);
    scene.addUV(0.0f, 1.0f);
    scene.addUV(1.0f, 1.0f);
    scene.addUV(1.0f, 0.0f);
    scene.addUV(0.0f, 0.0f);
    
    mc.createFloat3ConstantTexture("R_leftWall", 0.75f, 0.25f, 0.25f);
    mc.createFloat3ConstantTexture("R_rightWall", 0.25f, 0.25f, 0.75f);
    mc.createFloat3ConstantTexture("R_otherWalls", 0.75f, 0.75f, 0.75f);
    mc.createFloatConstantTexture("sigma_lambert", 0.0f);
    mc.createCheckerBoardTexture("R_floor", 0.75f, 0.75f, 0.75f, 0.25f, 0.25f, 0.25f);
    mc.createImageTexture("R_backWall", "images/pika_flat.png");
    
    mc.createMatteMaterial("mat_leftWall", scene.idxOfTex("R_leftWall"), scene.idxOfTex("sigma_lambert"));
    mc.createMatteMaterial("mat_rightWall", scene.idxOfTex("R_rightWall"), scene.idxOfTex("sigma_lambert"));
    mc.createMatteMaterial("mat_otherWalls", scene.idxOfTex("R_otherWalls"), scene.idxOfTex("sigma_lambert"));
    mc.createMatteMaterial("mat_floor", scene.idxOfTex("R_floor"), scene.idxOfTex("sigma_lambert"));
    mc.createMatteMaterial("mat_backWall", scene.idxOfTex("R_otherWalls"), scene.idxOfTex("sigma_lambert"));
    
    scene.addFace(Face::make_pt(1, 0, 3, 5, 4, 7, scene.idxOfMat("mat_backWall")));
    scene.addFace(Face::make_pt(1, 3, 2, 5, 7, 6, scene.idxOfMat("mat_backWall")));
    scene.addFace(Face::make_p(0, 4, 7, scene.idxOfMat("mat_leftWall")));
    scene.addFace(Face::make_p(0, 7, 3, scene.idxOfMat("mat_leftWall")));
    scene.addFace(Face::make_p(5, 1, 2, scene.idxOfMat("mat_rightWall")));
    scene.addFace(Face::make_p(5, 2, 6, scene.idxOfMat("mat_rightWall")));
    scene.addFace(Face::make_pt(4, 5, 1, 0, 1, 2, scene.idxOfMat("mat_floor")));
    scene.addFace(Face::make_pt(4, 1, 0, 0, 2, 3, scene.idxOfMat("mat_floor")));
    scene.addFace(Face::make_p(2, 3, 7, scene.idxOfMat("mat_otherWalls")));
    scene.addFace(Face::make_p(2, 7, 6, scene.idxOfMat("mat_otherWalls")));
    scene.endObject();
    
    //光源
    scene.beginObject();
    scene.addVertex(-0.25f, 0.9999f, -0.25f);
    scene.addVertex(0.25f, 0.9999f, -0.25f);
    scene.addVertex(0.25f, 0.9999f, 0.25f);
    scene.addVertex(-0.25f, 0.9999f, 0.25f);
    
    mc.createFloat3ConstantTexture("R_light", 0.9f, 0.9f, 0.9f);
    mc.createFloat3ConstantTexture("M_top", 1500.0f, 1500.0f, 1500.0f);
    
    mc.createMatteMaterial("mat_light", scene.idxOfTex("R_light"), scene.idxOfTex("sigma_lambert"));
    mc.createDiffuseLightProperty("light_top", scene.idxOfTex("M_top"));
    
    scene.addFace(Face::make_p(0, 1, 2, scene.idxOfMat("mat_light"), scene.idxOfLight("light_top")));
    scene.addFace(Face::make_p(0, 2, 3, scene.idxOfMat("mat_light"), scene.idxOfLight("light_top")));
    scene.endObject();
    
    loadModel("models/Pikachu_corrected_high.obj", &scene);
    
    scene.build();
}

int main(int argc, const char * argv[]) {
    const uint32_t iterations = 256;
    
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
        
        cl::Buffer buf_vertices{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numVertices() * sizeof(cl_float3), scene.rawVertices(), nullptr};
        cl::Buffer buf_normals;
        if (scene.numNormals() == 0)
            buf_normals = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float3), nullptr, nullptr);
        else
            buf_normals = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numNormals() * sizeof(cl_float3), scene.rawNormals(), nullptr);
        cl::Buffer buf_tangents;
        if (scene.numTangents() == 0)
            buf_tangents = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float3), nullptr, nullptr);
        else
            buf_tangents = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numTangents() * sizeof(cl_float3), scene.rawTangents(), nullptr);
        cl::Buffer buf_uvs;
        if (scene.numUVs() == 0)
            buf_uvs = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float2), nullptr, nullptr);
        else
            buf_uvs = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numUVs() * sizeof(cl_float2), scene.rawUVs(), nullptr);
        cl::Buffer buf_faces{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numFaces() * sizeof(Face), scene.rawFaces(), nullptr};
        cl::Buffer buf_lights{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numLights() * sizeof(uint32_t), scene.rawLights(), nullptr};
        cl::Buffer buf_materialsData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfMaterialsData(), scene.rawMaterialsData(), nullptr};
        cl::Buffer buf_lightsData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfLightPropsData(), scene.rawLightPropsData(), nullptr};
        cl::Buffer buf_texturesData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfTexturesData(), scene.rawTexturesData(), nullptr};
        cl::Buffer buf_BVHnodes{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfBVHNodes(), scene.rawBVHNodes(), nullptr};
        cl::Buffer buf_Camera{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfCamera(), scene.rawCamera(), nullptr};
        cl::Buffer buf_randStates{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_randStates.size() * sizeof(uint32_t), (void*)g_randStates.data(), nullptr};
        cl::Buffer buf_pixels{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_pixels.size() * sizeof(cl_float3), (void*)g_pixels.data(), nullptr};
        
        cl::Kernel kernelRendering{programRendering, "pathtracing"};
        kernelRendering.setArg(0, buf_vertices);
        kernelRendering.setArg(1, buf_normals);
        kernelRendering.setArg(2, buf_tangents);
        kernelRendering.setArg(3, buf_uvs);
        kernelRendering.setArg(4, buf_faces);
        kernelRendering.setArg(5, buf_lights);
        kernelRendering.setArg(6, (uint32_t)scene.numLights());
        kernelRendering.setArg(7, buf_materialsData);
        kernelRendering.setArg(8, buf_lightsData);
        kernelRendering.setArg(9, buf_texturesData);
        kernelRendering.setArg(10, buf_BVHnodes);
        kernelRendering.setArg(11, buf_Camera);
        kernelRendering.setArg(12, buf_randStates);
        kernelRendering.setArg(13, buf_pixels);
        
        std::vector<cl::Event> eventList;
        cl::Event computeEvent;
        cl::Event readEvent;
        
        cl::CommandQueue queue{context, device};
        const int numTilesX = 8, numTilesY = 8;
        const int numTiles = numTilesX * numTilesY;
        cl::NDRange tile{g_width / numTilesX, g_height / numTilesY};
        cl::NDRange localSize{32, 32};
#if SIMULATION
        sim::global_sizes[0] = (sim::uint)*tile;
        sim::global_sizes[1] = (sim::uint)*(tile + 1);
        for (int i = 0; i < iterations; ++i) {
            printf("[ %d ]", i);
            for (int j = 0; j < numTiles; ++j) {
                sim::global_offsets[0] = (sim::uint)*tile * (j % numTilesX);
                sim::global_offsets[1] = (sim::uint)*(tile + 1) * (j / numTilesX);
                for (int ty = 0; ty < *(tile + 1); ++ty) {
                    for (int tx = 0; tx < *tile; ++tx) {
                        sim::global_ids[0] = sim::global_offsets[0] + tx;
                        sim::global_ids[1] = sim::global_offsets[1] + ty;
                        sim::pathtracing((sim::float3*)scene.rawVertices(), (sim::float3*)scene.rawNormals(), (sim::float3*)scene.rawTangents(), (sim::float2*)scene.rawUVs(),
                                         (sim::uchar*)scene.rawFaces(), (sim::uint*)scene.rawLights(), (sim::uint)scene.numLights(),
                                         (sim::uchar*)scene.rawMaterialsData(), (sim::uchar*)scene.rawLightPropsData(), (sim::uchar*)scene.rawTexturesData(),
                                         (sim::uchar*)scene.rawBVHNodes(), g_randStates.data(), g_width, g_height, sppOnce, (sim::float3*)g_pixels.data());
                    }
                }
                printf("*");
            }
            printf("\n");
        }
        buf_pixels = {context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_pixels.size() * sizeof(cl_float3), (void*)g_pixels.data(), nullptr};
#else
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
#endif
        
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
        kernelToneMapping.setArg(3, iterations);
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

