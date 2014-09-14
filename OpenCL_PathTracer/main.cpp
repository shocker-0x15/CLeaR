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
#define __CL_ENABLE_EXCEPTIONS
#include "cl12.hpp"
#include "clUtility.hpp"
#include "CreateFunctions.hpp"
#include "ModelLoader.hpp"
#include "BVH.hpp"
#include "Scene.h"
#include "StopWatch.hpp"
#include "BMPExporter.h"

#include "sim_pathtracer.hpp"

void CL_CALLBACK completeTile(cl_event ev, cl_int exec_status, void* user_data) {
    printf("*");
}

XORShiftRandom32 rng{215363872};
const int g_width = 1024;
const int g_height = 1024;
Scene scene;
std::vector<uint32_t> g_randStates{};
std::vector<cl_float3> g_pixels{};

std::string stringFromFile(const char* filename) {
    std::ifstream ifs;
    ifs.open(filename);
    ifs.clear();
    ifs.seekg(0, std::ios::beg);
    return std::string{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
}

void buildScene(StopWatch &sw) {
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
    Matrix4f camLocalToWorld = LA::RotateMatrix(-M_PI * 0.3f, 0, 1, 0) * LA::LookAt(-0.6f, 0.5f, 5.5f, 0.0f, 0.0f, 0.0f, 0, 1, 0).Invert();
    memcpy(&perspectiveCamera.head.localToWorld, &camLocalToWorld, sizeof(cl_float16));
    perspectiveCamera.id = 0;// perspective
    float fovY = 30.0f * M_PI / 180.0f;
    float aspect = (float)g_width / g_height;
    float near = 1;
    float far = 100;
    perspectiveCamera.virtualPlaneArea = aspect * powf(tanf(fovY / 2), 2);
    perspectiveCamera.lensRadius = 0.05f;
    perspectiveCamera.objPDistance = 5.3f;
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
    
    mc.createImageTexture("IBLSource", "images/Alexs_Apt_2k.exr");
    mc.createImageBasedEnvLightPropety("IBL", "IBLSource", 50.0f);
    struct EnvironmentHead {
        uint32_t offsetEnvLightProperty;
    };
    EnvironmentHead envHead;
    envHead.offsetEnvLightProperty = (uint32_t)scene.idxOfLight("IBL");
    scene.setEnvironment(addDataAligned(refOthers, envHead, 4));
    
    scene.localToWorld = LA::RotateMatrix(-M_PI * 0.3f, 0, 1, 0);
    scene.localToWorld.push();
    
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
    mc.createFloat3CheckerBoardTexture("R_floor", 0.75f, 0.75f, 0.75f, 0.25f, 0.25f, 0.25f);
    mc.createFloat3CheckerBoardBumpTexture("bump_floor", 0.05f, false);
    mc.createNormalMapTexture("bump_backWall", "images/paper_bump.png");
    mc.createImageTexture("R_backWall", "images/Kirby.png");
    
    //    mc.createImageTexture("R_floor", "images/stone_wall__.png");
    //    mc.createNormalMapTexture("bump_floor", "images/stone_wall_normal_map__.png");
    //    mc.createFloat3ConstantTexture("Rs_floor", 0.15f, 0.15f, 0.15f);
    //    mc.createFloatConstantTexture("nu_floor", 150);
    //    mc.createFloatConstantTexture("nv_floor", 150);
    //    mc.createAshikhminMaterial("mat_floor", "bump_floor", "Rs_floor", "R_floor", "nu_floor", "nv_floor");
    
    mc.createMatteMaterial("mat_leftWall", nullptr, "R_leftWall", "sigma_lambert");
    mc.createMatteMaterial("mat_rightWall", nullptr, "R_rightWall", "sigma_lambert");
    mc.createMatteMaterial("mat_otherWalls", nullptr, "R_otherWalls", "sigma_lambert");
    mc.createMatteMaterial("mat_floor", "bump_floor", "R_floor", "sigma_lambert");
    mc.createMatteMaterial("mat_backWall", "bump_backWall", "R_backWall", "sigma_lambert");
    
    scene.addFace(Face::make_P_UV(1, 0, 3, 5, 4, 7, scene.idxOfMat("mat_backWall"), UINT16_MAX, (uint32_t)scene.idxOfTex("R_backWall")));
    scene.addFace(Face::make_P_UV(1, 3, 2, 5, 7, 6, scene.idxOfMat("mat_backWall"), UINT16_MAX, (uint32_t)scene.idxOfTex("R_backWall")));
    scene.addFace(Face::make_P(0, 4, 7, scene.idxOfMat("mat_leftWall")));
    scene.addFace(Face::make_P(0, 7, 3, scene.idxOfMat("mat_leftWall")));
    scene.addFace(Face::make_P(5, 1, 2, scene.idxOfMat("mat_rightWall")));
    scene.addFace(Face::make_P(5, 2, 6, scene.idxOfMat("mat_rightWall")));
    scene.addFace(Face::make_P_UV(4, 5, 1, 0, 1, 2, scene.idxOfMat("mat_floor")));
    scene.addFace(Face::make_P_UV(4, 1, 0, 0, 2, 3, scene.idxOfMat("mat_floor")));
    scene.addFace(Face::make_P(2, 3, 7, scene.idxOfMat("mat_otherWalls")));
    scene.addFace(Face::make_P(2, 7, 6, scene.idxOfMat("mat_otherWalls")));
    scene.endObject();
    
    //光源
    scene.beginObject();
    scene.addVertex(-0.25f, 0.9999f, -0.25f);
    scene.addVertex(0.25f, 0.9999f, -0.25f);
    scene.addVertex(0.25f, 0.9999f, 0.25f);
    scene.addVertex(-0.25f, 0.9999f, 0.25f);
    
    mc.createFloat3ConstantTexture("R_light", 0.9f, 0.9f, 0.9f);
    mc.createFloat3ConstantTexture("M_top", 1500.0f, 1500.0f, 1500.0f);
    
    mc.createMatteMaterial("mat_light", nullptr, "R_light", "sigma_lambert");
    mc.createDiffuseLightProperty("light_top", "M_top");
    
    //    scene.addFace(Face::make_P(0, 1, 2, scene.idxOfMat("mat_light"), scene.idxOfLight("light_top")));
    //    scene.addFace(Face::make_P(0, 2, 3, scene.idxOfMat("mat_light"), scene.idxOfLight("light_top")));
    scene.endObject();
    
    scene.localToWorld.push();
    {
        scene.localToWorld *= LA::TranslateMatrix(0, -0.999f, 0) * LA::RotateMatrix(M_PI / 6, 0, 1, 0) * LA::ScaleMatrix(0.25f, 0.25f, 0.25f);
        loadModel("models/Pikachu.obj", &scene);
    }
    scene.localToWorld.pop();
    
    scene.localToWorld.push();
    {
        scene.localToWorld *= LA::TranslateMatrix(-0.6f, -0.999f, 0.4f) * LA::RotateMatrix(-M_PI / 8, 0, 1, 0) * LA::ScaleMatrix(0.15f, 0.25f, 0.15f) * LA::TranslateMatrix(0, 1, 0);
        
        mc.createFloat3ConstantTexture("box_light_R", 0.95f, 0.95f, 0.95f);
        mc.createFloatConstantTexture("box_light_sigma", 0.0f);
        mc.createMatteMaterial("box_light", nullptr, "box_light_R", "box_light_sigma");
        
        mc.createFloat3ConstantTexture("box_light_M", 300.0f, 300.0f, 270.0f);
        mc.createDiffuseLightProperty("box_light", "box_light_M");
        
        loadModel("models/box.obj", &scene, scene.idxOfMat("box_light"), scene.idxOfLight("box_light"));
    }
    scene.localToWorld.pop();
    
    for (int i = 0; i < 100; ++i) {
        float scale = 0.25f * (0.5f + rng.getFloat0cTo1o());
        float angle = 2 * M_PI * rng.getFloat0cTo1o();
        float tx = -5 + 10 * rng.getFloat0cTo1o();
        float ty = -5 + 10 * rng.getFloat0cTo1o();
        float tz = -5 + 10 * rng.getFloat0cTo1o();
        Vector3f axis{rng.getFloat0cTo1o(), rng.getFloat0cTo1o(), rng.getFloat0cTo1o()};
        axis = axis.normalize();
        
        scene.localToWorld.push();
        {
            scene.localToWorld *= LA::TranslateMatrix(tx, ty, tz) * LA::RotateMatrix(angle, axis.x, axis.y, axis.z) * LA::ScaleMatrix(scale, scale, scale);
            loadModel("models/Pikachu.obj", &scene);
        }
        scene.localToWorld.pop();
    }
    
    scene.localToWorld.pop();
    
    sw.start();
    scene.build();
    printf("building BVH done! ... time: %fmsec\n", sw.stop(StopWatch::Microseconds) * 0.001f);
}

int main(int argc, const char * argv[]) {
    using namespace std::chrono;
    
    StopWatch stopwatch;
    StopWatchHiRes stopwatchHiRes;
    
    std::time_t ctimeLaunch = system_clock::to_time_t(stopwatch.start());
    printf("%s\n", std::ctime(&ctimeLaunch));
    
    initCLUtility();
    
#define SIMULATION 0
    const uint32_t iterations = 16;
    
    stopwatch.start();
    buildScene(stopwatch);
    printf("build time: %fsec\n", stopwatch.stop() * 0.001f);
    
    cl_int ret = CL_SUCCESS;
#ifdef __CL_ENABLE_EXCEPTIONS
    try {
#endif
        cl::Platform platform;
        cl::Platform::get(&platform);
        
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        cl::Device device = devices[1];
        
        printf("--------------------------------\n");
        printDeviceInfo(device, CL_DEVICE_NAME);
        printf("\n");
        
//        for (uint32_t info = 0x1000; info <= 0x104B; ++info) {
//            printDeviceInfo(device, info);
//        }
        
        cl_context_properties ctx_props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform(), 0};
        cl::Context context{device, ctx_props};
        const bool profiling = true;
        cl::CommandQueue queue{context, device, static_cast<cl_command_queue_properties>(profiling ? CL_QUEUE_PROFILING_ENABLE : 0)};
        std::vector<cl::Event> events{50};
        uint32_t evIdx;
        cl_ulong tpCmdStart, tpCmdEnd, tpCmdSubmit;
        
        std::string buildLog;
        
        //------------------------------------------------
        // 空間分割プログラムの生成
        stopwatch.start();
        
        std::string rawStrBuildAccel = stringFromFile("bvh_construction.cl");
        cl::Program::Sources srcBuildAccel{1, std::make_pair(rawStrBuildAccel.c_str(), rawStrBuildAccel.length())};
        
        cl::Program programBuildAccel{context, srcBuildAccel};
        programBuildAccel.build("");
        programBuildAccel.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("build accel kernel build log: \n");
        printf("%s\n", buildLog.c_str());
        
        printf("BVH kernel setup time: %lldmsec\n", stopwatch.stop());
        printf("\n");
        //------------------------------------------------
        
        
        //------------------------------------------------
        // レンダリングプログラムの生成
        stopwatch.start();
        
        std::string rawStrRendering = stringFromFile("pathtracer.cl");
        cl::Program::Sources srcRendering{1, std::make_pair(rawStrRendering.c_str(), rawStrRendering.length())};
        
        cl::Program programRendering{context, srcRendering};
//        programRendering.build("");
//        programRendering.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
//        printf("rendering kernel build log: \n");
//        printf("%s\n", buildLog.c_str());
        
        printf("rendering kernel setup time: %lldmsec\n", stopwatch.stop());
        printf("\n");
        //------------------------------------------------
        
        
        //------------------------------------------------
        // ポストプロセスプログラムの生成
        stopwatch.start();
        
        std::string rawStrPostProcessing = stringFromFile("post_processing.cl");
        cl::Program::Sources srcPostProcessing{1, std::make_pair(rawStrPostProcessing.c_str(), rawStrPostProcessing.length())};
        
        cl::Program programPostProcessing{context, srcPostProcessing};
        programPostProcessing.build("");
        programPostProcessing.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
        printf("post-process kernel build log: \n");
        printf("%s\n", buildLog.c_str());
        
        printf("post-process kernel setup time: %lldmsec\n", stopwatch.stop());
        printf("--------------------------------\n");
        //------------------------------------------------
        
        
        
        //------------------------------------------------
        // 空間分割開始
        stopwatchHiRes.start();
        
        struct AABB {
            cl_float3 min;
            cl_float3 max;
            cl_float3 center;
        };
        
        // 各三角形のAABBを並列に求める。
        cl::Kernel kernelCalcAABBs{programBuildAccel, "calcAABBs"};
        cl::Buffer buf_vertices{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numVertices() * sizeof(cl_float3), scene.rawVertices(), nullptr};
        cl::Buffer buf_faces{context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, scene.numFaces() * sizeof(Face), scene.rawFaces(), nullptr};
        cl::Buffer buf_AABBs{context, CL_MEM_READ_WRITE, scene.numFaces() * sizeof(AABB), nullptr, nullptr};
        {
            kernelCalcAABBs.setArg(0, buf_vertices);
            kernelCalcAABBs.setArg(1, buf_faces);
            kernelCalcAABBs.setArg(2, scene.numFaces());
            kernelCalcAABBs.setArg(3, buf_AABBs);
            
            uint32_t localSize = 128;
            uint32_t workSize = (((uint32_t)scene.numFaces() + (localSize - 1)) / localSize) * localSize;
            queue.enqueueNDRangeKernel(kernelCalcAABBs, cl::NullRange, cl::NDRange{workSize}, cl::NDRange{localSize}, nullptr, &events[0]);
        }
        cl::finish();
        if (profiling) {
            events[0].wait();
            getProfilingInfo(events[0], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            printf("calculating each AABB done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, (tpCmdEnd - tpCmdSubmit) * 0.001f);
        }
        
        // 全体を囲むAABBを求める。
        cl::Kernel kernelUnifyAABBs{programBuildAccel, "unifyAABBs"};
        cl::Buffer buf_srcAABBs = buf_AABBs;
        cl::Buffer buf_unifiedAABBs;
        {
            uint32_t numAABBs = (uint32_t)scene.numFaces();
            uint32_t localSize = 128;
            evIdx = 0;
            while (true) {
                uint32_t numMerged = (numAABBs + (localSize - 1)) / localSize;
                uint32_t workSize = numMerged * localSize;
                buf_unifiedAABBs = cl::Buffer(context, CL_MEM_READ_WRITE, numMerged * sizeof(AABB), nullptr, nullptr);
                
                kernelUnifyAABBs.setArg(0, buf_srcAABBs);
                kernelUnifyAABBs.setArg(1, numAABBs);
                kernelUnifyAABBs.setArg(2, buf_unifiedAABBs);
                
                queue.enqueueNDRangeKernel(kernelUnifyAABBs, cl::NullRange, cl::NDRange{workSize}, cl::NDRange{localSize}, nullptr, &events[evIdx++]);
                queue.enqueueBarrierWithWaitList();
                
                if (numMerged == 1)
                    break;
                
                buf_srcAABBs = buf_unifiedAABBs;
                numAABBs = numMerged;
            }
        }
        cl::finish();
        if (profiling) {
            cl_ulong sumTimeUnion = 0, sumTimeUnionFromSubmit = 0;
            for (uint32_t i = 0; i < evIdx; ++i) {
                events[i].wait();
                getProfilingInfo(events[i], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
                sumTimeUnion += tpCmdEnd - tpCmdStart;
                sumTimeUnionFromSubmit += tpCmdEnd - tpCmdSubmit;
            }
            printf("unifying AABBs done! ... time: %fusec (%fusec)\n", sumTimeUnion * 0.001f, sumTimeUnionFromSubmit * 0.001f);
        }
        AABB entireAABB;
        queue.enqueueReadBuffer(buf_unifiedAABBs, CL_TRUE, 0, sizeof(AABB), &entireAABB);
        
        // モートンコードを求める。
        cl::Kernel kernelCalcMortonCodes{programBuildAccel, "calcMortonCodes"};
        cl::Buffer buf_MortonCodes{context, CL_MEM_READ_WRITE, scene.numFaces() * sizeof(cl_uint3), nullptr, nullptr};
        cl::Buffer buf_Indices{context, CL_MEM_READ_WRITE, scene.numFaces() * sizeof(cl_uint), nullptr, nullptr};
        {
            cl_float3 sizeEntireAABB = {
                entireAABB.max.s0 - entireAABB.min.s0,
                entireAABB.max.s1 - entireAABB.min.s1,
                entireAABB.max.s2 - entireAABB.min.s2
            };
            
            kernelCalcMortonCodes.setArg(0, buf_AABBs);
            kernelCalcMortonCodes.setArg(1, scene.numFaces());
            kernelCalcMortonCodes.setArg(2, entireAABB.min);
            kernelCalcMortonCodes.setArg(3, sizeEntireAABB);
            kernelCalcMortonCodes.setArg(4, 10);
            kernelCalcMortonCodes.setArg(5, buf_MortonCodes);
            kernelCalcMortonCodes.setArg(6, buf_Indices);
            
            uint32_t localSize = 128;
            uint32_t workSize = (((uint32_t)scene.numFaces() + (localSize - 1)) / localSize) * localSize;
            queue.enqueueNDRangeKernel(kernelCalcMortonCodes, cl::NullRange, cl::NDRange{workSize}, cl::NDRange{localSize}, nullptr, &events[0]);
        }
        cl::finish();
        if (profiling) {
            events[0].wait();
            getProfilingInfo(events[0], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            printf("calculating each Morton code done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, (tpCmdEnd - tpCmdSubmit) * 0.001f);
        }
        
        cl::Kernel kernelBlockwiseSort{programBuildAccel, "blockwiseSort"};
        {
            kernelBlockwiseSort.setArg(0, buf_MortonCodes);
            kernelBlockwiseSort.setArg(1, scene.numFaces());
            kernelBlockwiseSort.setArg(2, 0);
            kernelBlockwiseSort.setArg(3, buf_Indices);
            
            uint32_t localSize = 128;
            uint32_t workSize = (((uint32_t)scene.numFaces() + (localSize - 1)) / localSize) * localSize;
            queue.enqueueNDRangeKernel(kernelBlockwiseSort, cl::NullRange, cl::NDRange{workSize}, cl::NDRange{localSize}, nullptr, &events[0]);
        }
        cl::finish();
        if (profiling) {
            events[0].wait();
            getProfilingInfo(events[0], &tpCmdStart, &tpCmdEnd, &tpCmdSubmit);
            printf("blockwise sorting done! ... time: %fusec (%fusec)\n", (tpCmdEnd - tpCmdStart) * 0.001f, (tpCmdEnd - tpCmdSubmit) * 0.001f);
        }
        
        printf("spatial splitting done! ... time: %llumsec\n", stopwatchHiRes.stop());
        //------------------------------------------------
        
        
        //------------------------------------------------
        //レンダリング開始
        stopwatch.start();
        
        const int numTilesX = 16, numTilesY = 16;
        const int numTiles = numTilesX * numTilesY;
        cl::NDRange tile{g_width / numTilesX, g_height / numTilesY};
        cl::NDRange localSize{8, 8};
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
        
        for (int i = 0; i < iterations; ++i) {
            printf("[ %d ]", i);
            
            stopwatch.start();
            for (int j = 0; j < numTiles; ++j) {
                cl::NDRange offset{*tile * (j % numTilesX), *(tile + 1) * (j / numTilesX)};
                cl::Event ev;
                queue.enqueueNDRangeKernel(kernelRendering, offset, tile, localSize, nullptr, &ev);
                //                ev.setCallback(CL_COMPLETE, completeTile);
            }
            queue.finish();
            
            printf(" %fsec\n", stopwatch.stop() * 0.001f);
            
            uint64_t runtime = stopwatch.elapsedFromRoot(StopWatch::Seconds);
            if (runtime > 60 * 10 * k10mins) {
                std::time_t cTimeElapsed = system_clock::to_time_t(system_clock::now());
                printf("%u x 10 minutes.\n", k10mins);
                printf("%s", std::ctime(&cTimeElapsed));
                ++k10mins;
            }
        }
#endif
        printf("rendering done! ... time: %fsec\n", stopwatch.stop() * 0.001f);
        //------------------------------------------------
        
        
        //------------------------------------------------
        //ポストプロセッシング開始
        stopwatch.start();
        
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
        
        queue.enqueueNDRangeKernel(kernelClear, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueNDRangeKernel(kernelPostProcess0, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueNDRangeKernel(kernelToneMappng, cl::NullRange, cl::NDRange{g_width, g_height}, cl::NullRange, nullptr, nullptr);
        queue.finish();
        queue.enqueueReadBuffer(buf_image, CL_TRUE, 0, byteWidth * g_height, LDRPixels, nullptr, nullptr);
        
        printf("post-process done! ... time: %fsec\n", stopwatch.stop() * 0.001f);
        //------------------------------------------------
        
        saveBMP("output.bmp", LDRPixels, g_width, g_height);
        free(LDRPixels);
#ifdef __CL_ENABLE_EXCEPTIONS
    } catch (cl::Error error) {
        char err_str[64];
        printErrorFromCode(error.err(), err_str);
        fprintf(stderr, "ERROR: %s @ ", err_str);
        fprintf(stderr, "%s\n", error.what());
    }
#endif
    
    return 0;
}
