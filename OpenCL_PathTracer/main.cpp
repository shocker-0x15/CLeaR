//
//  main.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/04/17.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include <cstdio>
#include <vector>
#include "LinearAlgebra.h"
#include "XORShift.hpp"
#include <fstream>
//#define __CL_ENABLE_EXCEPTIONS
#include "cl12.hpp"
#include "clUtility.hpp"
#include "CreateFunctions.hpp"
#include "ModelLoader.hpp"
#include "BVH.hpp"
#include "Scene.h"
#include <chrono>
#include "BMPExporter.h"

#include "sim_pathtracer.hpp"

void CL_CALLBACK completeTile(cl_event ev, cl_int exec_status, void* user_data) {
    printf("*");
}

XORShiftRandom32 rng{215363872};
const int g_width = 2048;
const int g_height = 1152;
Scene scene;
std::vector<uint32_t> g_randStates{};
std::vector<cl_float3> g_pixels{};

void HSVtoRGB(float h, float s, float v, float* r, float* g, float* b) {
    int Hi = (int)(floorf(fmodf(6.0f * h, 6)));
    float f = 6.0f * h - Hi;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    switch (Hi) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
            break;
        default:
            break;
    }
}

void buildScene() {
    namespace LA = LinearAlgebra;
    
    std::vector<uint8_t>* refOthers = &scene.otherResouces;
    
    //128bytes
    struct CameraHead {
        uint width, height; uint8_t dum0[56];
        cl_float16 localToWorld;
    };
    //256bytes
    struct PerspectiveInfo {
        CameraHead head;
        uint8_t id; uint8_t dum0[3];
        float virtualPlaneArea;
        float lensRadius;
        float objPDistance; uint8_t dum1[48];
        cl_float16 rasterToCamera;
    };
    PerspectiveInfo perspectiveCamera;
    perspectiveCamera.head.width = g_width;
    perspectiveCamera.head.height = g_height;
    Matrix4f camLocalToWorld = LA::LookAt(-2.0f, 0.5f, 10.0f, 0.0f, 3.0f, 0.0f, 0, 1, 0).Invert();
    memcpy(&perspectiveCamera.head.localToWorld, &camLocalToWorld, sizeof(cl_float16));
    perspectiveCamera.id = 0;// perspective
    float fovY = 50.0f * M_PI / 180.0f;
    float aspect = (float)g_width / g_height;
    float near = 1;
    float far = 100;
    perspectiveCamera.virtualPlaneArea = aspect * powf(tanf(fovY / 2), 2);
    perspectiveCamera.lensRadius = 0.01f;
    perspectiveCamera.objPDistance = 1.041633333f * 10;
    Matrix4f clipToCamera = Matrix4f(Vector4f(1 / (aspect * tanf(fovY / 2)), 0, 0, 0),
                                     Vector4f(0, 1 / tanf(fovY / 2), 0, 0),
                                     Vector4f(0, 0, -(far + near) / (far - near), -1),
                                     Vector4f(0, 0, -2 * far * near / (far - near), 0)).Invert();
    Matrix4f rasterToScreen = Matrix4f(Vector4f(2.0f / g_width, 0, 0, 0), // raster to screen
                                       Vector4f(0, -2.0f / g_height, 0, 0),
                                       Vector4f(0, 0, 2, 0),
                                       Vector4f(-1, 1, -1, 1));
    Matrix4f rasterToCamera = clipToCamera * rasterToScreen;
    cl_float16 f16Val;
    memcpy(&f16Val, &rasterToCamera, sizeof(cl_float16));
    perspectiveCamera.rasterToCamera = f16Val;
    scene.setCamera(addDataAligned(refOthers, perspectiveCamera, 64));
    
    
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
    
    mc.createImageTexture("IBLSource", "images/moonlight_night.exr");
    mc.createImageBasedEnvLightPropety("IBL", "IBLSource", 10.0f);
    struct EnvironmentHead {
        uint32_t offsetEnvLightProperty;
    };
    EnvironmentHead envHead;
    envHead.offsetEnvLightProperty = (uint32_t)scene.idxOfLight("IBL");
    scene.setEnvironment(addDataAligned(refOthers, envHead, 4));
    
    
    scene.localToWorld = LA::ScaleMatrix(10.0f, 10.0f, 10.0f);
    
    
    scene.localToWorld.push();
    {
        scene.localToWorld = scene.localToWorld.top() * LA::TranslateMatrix(-5, 0, -5);
        
        mc.createNormalMapTexture("floor_bump", "images/RayHH_assets/Metal-3166-bump-map.png");
        mc.createFloat3ConstantTexture("floor_Rs", 0.75f, 0.75f, 0.75f);
        mc.createFloat3ConstantTexture("floor_Rd", 0.5f, 0.5f, 0.5f);
        mc.createFloatConstantTexture("floor_nu", 100.0f);
        mc.createFloatConstantTexture("floor_nv", 100.0f);
        mc.createAshikhminMaterial("floor_panel", "floor_bump", "floor_Rs", "floor_Rd", "floor_nu", "floor_nv");
//        mc.createFloat3ConstantTexture("floor_R", 0.75f, 0.75f, 0.75f);
//        mc.createFloatConstantTexture("floor_sigma", 0.0f);
//        mc.createMatteMaterial("floor_panel", nullptr, "floor_R", "floor_sigma");
//        mc.createFloat3ConstantTexture("floor_R", 0.99f, 0.99f, 0.99f);
//        mc.createMetalMaterial("floor_panel", nullptr, "floor_R",
//                               0.14221f, 0.12643f, 0.15837f, 4.5230e+0f, 3.3071e+0f, 2.3359e+0f);
        
        for (int i = 0; i < 100; ++i) {
            scene.localToWorld.push();
            scene.localToWorld = scene.localToWorld.top() * LA::TranslateMatrix(float(i % 10), 0.0f, float(i / 10));
            
            scene.beginObject();
            {
                scene.addVertex(0.0f, 0.0f, 0.0f);
                scene.addVertex(0.0f, 0.0f, 1.0f);
                scene.addVertex(1.0f, 0.0f, 1.0f);
                scene.addVertex(1.0f, 0.0f, 0.0f);
                
                scene.addUV(0.0f, 0.0f);
                scene.addUV(0.0f, 5.0f);
                scene.addUV(5.0f, 5.0f);
                scene.addUV(5.0f, 0.0f);
                
                scene.addFace(Face::make_P_UV(0, 1, 2, 0, 1, 2, scene.idxOfMat("floor_panel")));
                scene.addFace(Face::make_P_UV(0, 2, 3, 0, 2, 3, scene.idxOfMat("floor_panel")));
            }
            scene.endObject();
            
            scene.localToWorld.pop();
        }
    }
    scene.localToWorld.pop();
    
    
    scene.localToWorld.push();
    {
        scene.localToWorld = scene.localToWorld.top() *
        LA::RotateMatrix(-M_PI / 9, 0, 1, 0) * LA::ScaleMatrix(0.1f, 0.1f, 0.1f) * LA::ScaleMatrix(0.75f, 2.25f, 0.75f) * LA::TranslateMatrix(0, 1, 0);
        
        mc.createFloat3ConstantTexture("box_light_R", 0.9f, 0.9f, 0.9f);
        mc.createFloatConstantTexture("box_light_sigma", 0.0f);
        mc.createMatteMaterial("box_light", nullptr, "box_light_R", "box_light_sigma");
        mc.createFloat3ConstantTexture("box_light_M", 1200.0f, 366.0f, 66.0f);
        mc.createDiffuseLightProperty("box_light", "box_light_M");
        
        loadModel("models/box.obj", &scene, scene.idxOfMat("box_light"), scene.idxOfLight("box_light"));
    }
    scene.localToWorld.pop();
    
    
    scene.localToWorld.push();
    {
        scene.localToWorld = scene.localToWorld.top() *
        LA::TranslateMatrix(0, 0.6f, 0) * LA::RotateMatrix(-M_PI / 9, 0, 1, 0) * LA::ScaleMatrix(0.1f, 0.1f, 0.1f);
        
        //Water
        mc.createFloat3ConstantTexture("water_R", 0.95f, 0.95f, 0.95f);
        mc.createFloat3ConstantTexture("water_T", 0.85f, 0.85f, 0.85f);
        mc.createGlassMaterial("monkey", nullptr,
                               "water_R",
                               "water_T", 1.0f, 1.333f);
        
        loadModel("models/monkey.obj", &scene, scene.idxOfMat("monkey"));
    }
    scene.localToWorld.pop();
    

//    scene.localToWorld.push();
//    {
//        scene.localToWorld = scene.localToWorld.top() *
//        LA::ScaleMatrix(0.25f, 0.1f, 0.25f) * LA::TranslateMatrix(0, -0.14f, 0.0f);
//        
//        mc.createFloat3ConstantTexture("fluid_R", 1.0f, 1.0f, 1.0f);
//        mc.createMetalMaterial("fluid", nullptr, "fluid_R",
//                               3.0568f, 3.15007f, 2.22288f, 3.3785e+0f, 3.3278e+0, 3.0647e+0f);
//        
//        loadModel("models/fluid.obj", &scene, scene.idxOfMat("fluid"));
//    }
//    scene.localToWorld.pop();
    
    
    scene.localToWorld.push();
    {
        for (int i = 0; i < 5; ++i) {
            int numAround = (i + 1) * 10;
            for (int j = 0; j < numAround; ++j) {
                if (i == 0 && j == 3)
                    continue;
                float sX = 0.3f + 0.2f * rng.getFloat0cTo1o();
                float sY = 1.0f + 0.2f * rng.getFloat0cTo1o();
                float sZ = 0.3f + 0.2f * rng.getFloat0cTo1o();
                float angle = 2 * M_PI * rng.getFloat0cTo1o();
                
                float posAngle = (2 * M_PI * j + 0.1f * rng.getFloat0cTo1o()) / numAround;
                float posRadius = 0.25f * (i + 0.5f * rng.getFloat0cTo1o()) + 0.7f;
                float scale = (0.03f + (5 - i) * 0.015f) + 0.03f * rng.getFloat0cTo1o();
                
                float r, g, b;
                char suffix[30];
                sprintf(suffix, "_%d_%d", i, j);
                std::string matName = "rndmat";
                matName += suffix;
                HSVtoRGB(rng.getFloat0cTo1o(), 0.3f + 0.7f * rng.getFloat0cTo1o(), 0.5f + 0.5f * rng.getFloat0cTo1o(), &r, &g, &b);
                mc.createFloat3ConstantTexture((matName + "_R").c_str(), r, g, b);
                mc.createFloatConstantTexture((matName + "_sigma").c_str(), rng.getFloat0cTo1o() * 0.3f);
                mc.createMatteMaterial(matName.c_str(), nullptr, (matName + "_R").c_str(), (matName + "_sigma").c_str());
                
                scene.localToWorld.push();
                {
                    scene.localToWorld = scene.localToWorld.top() *
                    LA::TranslateMatrix(posRadius * cosf(posAngle), 0.0f, posRadius * sinf(posAngle)) *
                    LA::RotateMatrix(angle, 0, 1, 0) * LA::ScaleMatrix(scale * sX, scale * sY, scale * sZ) * LA::TranslateMatrix(0, 1, 0);
                    
                    loadModel("models/beveled_box.obj", &scene, scene.idxOfMat(matName.c_str()));
                }
                scene.localToWorld.pop();
            }
        }
        
//        scene.localToWorld = scene.localToWorld.top() *
        
    }
    scene.localToWorld.pop();
    
    scene.build();
}

int main(int argc, const char * argv[]) {
    using namespace std::chrono;
    
    system_clock::time_point programStartTimePoint, startTimePoint, endTimePoint;
    system_clock::duration overallTime, buildTime, renderingKernelSetupTime, postProcessKernelSetupTime, renderingTime, postProcessTime;
    
    programStartTimePoint = system_clock::now();
    std::time_t ctimeLaunch = system_clock::to_time_t(programStartTimePoint);
    printf("%s\n", std::ctime(&ctimeLaunch));
    
#define SIMULATION 0
    const uint32_t iterations = 16;
    
    buildScene();
    
    buildTime = system_clock::now() - programStartTimePoint;
    printf("build time: %lldmsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(buildTime).count());
    
    cl_int ret = CL_SUCCESS;
//    try {
        std::ifstream ifs;
        cl::Platform platform;
        cl::Platform::get(&platform);
        
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        cl::Device device = devices[1];
        std::string deviceName;
        device.getInfo(CL_DEVICE_NAME, &deviceName);
        printf("--------------------------------\n");
        printf("%s\n\n", deviceName.c_str());
        
        cl_context_properties ctx_props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform(), 0};
        cl::Context context{device, ctx_props};
        
        
        //------------------------------------------------
        //レンダリングカーネルの生成
        startTimePoint = system_clock::now();
        
        ifs.open("pathtracer.cl");
        ifs.clear();
        ifs.seekg(0, std::ios::beg);
        std::string rawStrRendering{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        cl::Program::Sources srcRendering{1, std::make_pair(rawStrRendering.c_str(), rawStrRendering.length())};
        ifs.close();
        
        cl::Program programRendering{context, srcRendering};
        programRendering.build("");
        std::string buildLog;
        programRendering.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("rendering kernel build log: \n");
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
        cl::Buffer buf_lightInfos{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numLights() * sizeof(LightInfo), scene.rawLightInfos(), nullptr};
        cl::Buffer buf_materialsData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfMaterialsData(), scene.rawMaterialsData(), nullptr};
        cl::Buffer buf_texturesData{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfTexturesData(), scene.rawTexturesData(), nullptr};
        cl::Buffer buf_otherResources{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfOtherResouces(), scene.rawOtherResources(), nullptr};
        cl::Buffer buf_BVHnodes{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.sizeOfBVHNodes(), scene.rawBVHNodes(), nullptr};
        cl::Buffer buf_randStates{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_randStates.size() * sizeof(uint32_t), (void*)g_randStates.data(), nullptr};
        cl::Buffer buf_pixels{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_pixels.size() * sizeof(cl_float3), (void*)g_pixels.data(), nullptr};
        
        cl::Kernel kernelRendering{programRendering, "pathtracing"};
        kernelRendering.setArg(0, buf_vertices);
        kernelRendering.setArg(1, buf_normals);
        kernelRendering.setArg(2, buf_tangents);
        kernelRendering.setArg(3, buf_uvs);
        kernelRendering.setArg(4, buf_faces);
        kernelRendering.setArg(5, buf_lightInfos);
        kernelRendering.setArg(6, buf_materialsData);
        kernelRendering.setArg(7, buf_texturesData);
        kernelRendering.setArg(8, buf_otherResources);
        kernelRendering.setArg(9, buf_BVHnodes);
        kernelRendering.setArg(10, buf_randStates);
        kernelRendering.setArg(11, buf_pixels);
        
        std::vector<cl::Event> eventList;
        cl::Event computeEvent;
        cl::Event readEvent;
        
        cl::CommandQueue queue{context, device};
        const int numTilesX = 16, numTilesY = 16;
        const int numTiles = numTilesX * numTilesY;
        cl::NDRange tile{g_width / numTilesX, g_height / numTilesY};
        cl::NDRange localSize{8, 8};
        
        renderingKernelSetupTime =
        system_clock::now() - startTimePoint;
        printf("rendering kernel setup time: %lldmsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(renderingKernelSetupTime).count());
        printf("\n");
        //------------------------------------------------
        
        
        //------------------------------------------------
        //ポストプロセスカーネルの生成
        startTimePoint = system_clock::now();
        
        ifs.open("post_processing.cl");
        ifs.clear();
        ifs.seekg(0, std::ios::beg);
        std::string rawStrPostProcessing{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        cl::Program::Sources srcPostProcessing{1, std::make_pair(rawStrPostProcessing.c_str(), rawStrPostProcessing.length())};
        ifs.close();
        
        cl::Program programPostProcessing{context, srcPostProcessing};
        programPostProcessing.build("");
        programPostProcessing.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("post-process kernel build log: \n");
        printf("%s\n", buildLog.c_str());
        
        
        cl::Buffer buf_intermediate0{context, CL_MEM_READ_WRITE, g_pixels.size() * sizeof(cl_float3)};
        
        uint32_t byteWidth = 3 * g_width + g_width % 4;
        uint8_t* LDRPixels = (uint8_t*)malloc(byteWidth * g_height);
        cl::Buffer buf_image{context, CL_MEM_WRITE_ONLY, (size_t)(byteWidth * g_height)};
        
        
        cl::Kernel kernelClear{programPostProcessing, "clear"};
        kernelClear.setArg(0, g_width);
        kernelClear.setArg(1, g_height);
        kernelClear.setArg(2, buf_intermediate0);
        
        cl::Kernel kernelPostProcess0{programPostProcessing, "scaling"};
        kernelPostProcess0.setArg(0, g_width);
        kernelPostProcess0.setArg(1, g_height);
        kernelPostProcess0.setArg(2, iterations);
        kernelPostProcess0.setArg(3, buf_pixels);
        kernelPostProcess0.setArg(4, buf_intermediate0);
        
        cl::Kernel kernelToneMappng{programPostProcessing, "toneMapping"};
        kernelToneMappng.setArg(0, g_width);
        kernelToneMappng.setArg(1, g_height);
        kernelToneMappng.setArg(2, byteWidth);
        kernelToneMappng.setArg(3, buf_intermediate0);
        kernelToneMappng.setArg(4, buf_image);
        
        postProcessKernelSetupTime = system_clock::now() - startTimePoint;
        printf("post-process kernel setup time: %lldmsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(postProcessKernelSetupTime).count());
        printf("--------------------------------\n");
        //------------------------------------------------
        
        
        //------------------------------------------------
        //レンダリング開始
        system_clock::time_point renderingStartTimePoint;
        renderingStartTimePoint = std::chrono::system_clock::now();
        uint32_t k10mins = 1;
#if SIMULATION
        printf("CPU equivalent code:\n");
        sim::global_sizes[0] = (sim::uint)*tile;
        sim::global_sizes[1] = (sim::uint)*(tile + 1);
        for (int i = 0; i < iterations; ++i) {
            printf("[ %d ]", i);
            
            startTimePoint = std::chrono::system_clock::now();
            for (int j = 0; j < numTiles; ++j) {
                sim::global_offsets[0] = (sim::uint)*tile * (j % numTilesX);
                sim::global_offsets[1] = (sim::uint)*(tile + 1) * (j / numTilesX);
                for (int ty = 0; ty < *(tile + 1); ++ty) {
                    for (int tx = 0; tx < *tile; ++tx) {
                        sim::global_ids[0] = sim::global_offsets[0] + tx;
                        sim::global_ids[1] = sim::global_offsets[1] + ty;
                        sim::pathtracing((sim::float3*)scene.rawVertices(), (sim::float3*)scene.rawNormals(), (sim::float3*)scene.rawTangents(), (sim::float2*)scene.rawUVs(),
                                         (sim::uchar*)scene.rawFaces(), (sim::uint*)scene.rawLightInfos(),
                                         (sim::uchar*)scene.rawMaterialsData(), (sim::uchar*)scene.rawTexturesData(), (sim::uchar*)scene.rawOtherResources(),
                                         (sim::uchar*)scene.rawBVHNodes(), g_randStates.data(), (sim::float3*)g_pixels.data());
                    }
                }
                printf("*");
            }
            std::chrono::system_clock::duration passTime = std::chrono::system_clock::now() - startTimePoint;
            printf(" %fsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(passTime).count() * 0.001f);
        }
        buf_pixels = cl::Buffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, g_pixels.size() * sizeof(cl_float3), (void*)g_pixels.data(), nullptr);
        kernelPostProcess0.setArg(3, buf_pixels);
#else
        for (int i = 0; i < iterations; ++i) {
            printf("[ %d ]", i);
            
            startTimePoint = std::chrono::system_clock::now();
            for (int j = 0; j < numTiles; ++j) {
                cl::NDRange offset{*tile * (j % numTilesX), *(tile + 1) * (j / numTilesX)};
                cl::Event ev;
                queue.enqueueNDRangeKernel(kernelRendering, offset, tile, localSize, nullptr, &ev);
//                ev.setCallback(CL_COMPLETE, completeTile);
            }
            queue.finish();
            
            std::chrono::system_clock::duration passTime = std::chrono::system_clock::now() - startTimePoint;
            printf(" %fsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(passTime).count() * 0.001f);
            
            overallTime = system_clock::now() - programStartTimePoint;
            int64_t runtime = std::chrono::duration_cast<std::chrono::seconds>(overallTime).count();
            if (runtime > 60 * 10 * k10mins) {
                std::time_t cTimeElapsed = system_clock::to_time_t(system_clock::now());
                printf("%u x 10 minutes.\n", k10mins);
                printf("%s", std::ctime(&cTimeElapsed));
                ++k10mins;
            }
        }
#endif
        renderingTime = std::chrono::system_clock::now() - renderingStartTimePoint;
        printf("rendering done! ... time: %fsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(renderingTime).count() * 0.001f);
        //------------------------------------------------
        
        
        //------------------------------------------------
        //ポストプロセッシング開始
        startTimePoint = std::chrono::system_clock::now();
    
        queue.enqueueNDRangeKernel(kernelClear, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueNDRangeKernel(kernelPostProcess0, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueNDRangeKernel(kernelToneMappng, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueReadBuffer(buf_image, CL_TRUE, 0, byteWidth * g_height, LDRPixels, nullptr, nullptr);
        
        postProcessTime = std::chrono::system_clock::now() - startTimePoint;
        printf("post-process done! ... time: %fsec\n", std::chrono::duration_cast<std::chrono::milliseconds>(postProcessTime).count() * 0.001f);
        //------------------------------------------------
        
        saveBMP("output.bmp", LDRPixels, g_width, g_height);
        free(LDRPixels);
//    } catch (cl::Error error) {
//        char err_str[64];
//        printErrorFromCode(error.err(), err_str);
//        fprintf(stderr, "ERROR: %s @ ", err_str);
//        fprintf(stderr, "%s\n", error.what());
//    }

    return 0;
}
