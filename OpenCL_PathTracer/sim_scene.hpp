#ifndef sim_scene_cl
#define sim_scene_cl

#include "sim_global.hpp"
#include "sim_distribution.hpp"

namespace sim {
    struct BBox;//typedef struct BBox BBox;
    struct BVHNode;//typedef struct BVHNode BVHNode;
    
    //56bytes
    typedef struct {
        uint p0, p1, p2;
        uint vn0, vn1, vn2;
        uint vt0, vt1, vt2;
        uint uv0, uv1, uv2;
        uint alphaTexPtr;
        ushort matPtr, lightPtr;
    } Face;
    
    //8bytes
    typedef struct {
        uchar atInfinity; uchar dum0[3];
        uint reference;
    } LightInfo;
    
    //128bytes
    typedef struct {
        uint width, height; uchar dum0[56];
        mat4x4 localToWorld;
    } CameraHead;
    
    //4bytes
    typedef struct {
        uint offsetEnvLightProperty;
    } EnvironmentHead;
    
    typedef struct {
        point3* vertices;
        vector3* normals;
        vector3* tangents;
        float2* uvs;
        Face* faces;
        LightInfo* lights;
        uchar* materialsData;
        uchar* texturesData;
        uchar* otherResoucesData;
        BVHNode* BVHNodes;
        CameraHead* camera;
        EnvironmentHead* environment;
        Discrete1D* lightPowerDistribution;
    } Scene;
}

#endif
