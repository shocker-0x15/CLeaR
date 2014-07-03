#ifndef device_camera_cl
#define device_camera_cl

#include "global.cl"
#include "rng.cl"
#include "matrix.cl"

//8bytes
typedef struct __attribute__((aligned(4))) {
//    float uCamera;
    float uLens[2];
} CameraSample;

//8bytes
typedef struct __attribute__((aligned(4))) {
//    float uComponent;
    float uDir[2];
} IDFSample;

//256bytes
typedef struct __attribute__((aligned(64))) {
    CameraHead head;
    uchar id;
    float virtualPlaneArea __attribute__((aligned(4)));
    float lensRadius;
    float objPDistance;
    mat4x4 rasterToCamera __attribute__((aligned(64)));
} PerspectiveInfo;

//32bytes
typedef struct __attribute__((aligned(16))) {
    point3 localOrigin;
    const global PerspectiveInfo* info;
} PerspectiveIDF;

//32bytes
typedef struct __attribute__((aligned(16))) {
    DDFHead ddfHead;
    vector3 dir __attribute__((aligned(16)));
} IDFHead;

//------------------------

void sampleLensPos(const Scene* scene, const CameraSample* sample, LensPosition* lpos, uchar* IDF, float* areaPDF);

void IDFAlloc(const Scene* scene, const LensPosition* lpos, uchar* IDF);

color sample_We(const uchar* IDF, const IDFSample* sample, vector3* vin, float* dirPDF);
inline float absCosNsIDF(const uchar* IDF, const vector3* v);

//------------------------

void sampleLensPos(const Scene* scene, const CameraSample* sample, LensPosition* lpos, uchar* IDF, float* areaPDF) {
    const global CameraHead* head = (const global CameraHead*)scene->camera;
    
    float ux, uy;
    concentricSampleDisk(sample->uLens[0], sample->uLens[1], &ux, &uy);
    
    vector3 lCamDir = (vector3)(0.0f, 0.0f, 1.0f);
    mulMat4x4G_V3(&head->localToWorld, &lCamDir, &lpos->dir);
    
    const global PerspectiveInfo* info = (const global PerspectiveInfo*)head;
    
    point3 lensPosLocal = (point3)(ux * info->lensRadius, uy * info->lensRadius, 0.0f);
    mulMat4x4G_P3(&head->localToWorld, &lensPosLocal, &lpos->p);
    
    *areaPDF = 1.0f / (M_PI_F * info->lensRadius * info->lensRadius);
    lpos->uv = lensPosLocal.xy;
    
    IDFAlloc(scene, lpos, IDF);
}


void IDFAlloc(const Scene* scene, const LensPosition* lpos, uchar* IDF) {
    IDFHead* WeHead = (IDFHead*)IDF;
    WeHead->ddfHead._type = DDFType_PerspectiveIDF;
    
    WeHead->dir = lpos->dir;
    
    PerspectiveIDF* perspective = (PerspectiveIDF*)(IDF + sizeof(IDFHead));
    perspective->localOrigin = (point3)(lpos->uv, 0.0f);
    perspective->info = (const global PerspectiveInfo*)scene->camera;
}


color sample_We(const uchar* IDF, const IDFSample* sample, vector3* vin, float* dirPDF) {
    const IDFHead* head = (const IDFHead*)IDF;
    
    const PerspectiveIDF* pIDF = (const PerspectiveIDF*)(head + 1);
    const global PerspectiveInfo* info = pIDF->info;
    
    point4 pRas = (point4)(sample->uDir[0], sample->uDir[1], 0, 1);
    point4 pCamera;
    mulMat4x4G_P4(&info->rasterToCamera, &pRas, &pCamera);
    vector3 dir = normalize(pCamera.xyz);
    
    if (info->lensRadius > 0.0f) {
        float ft = -info->objPDistance / dir.z;
        point3 pFocus = (point3)(0, 0, 0) + ft * dir;
        
        dir = normalize(pFocus - pIDF->localOrigin);
    }
    
    mulMat4x4G_V3(&info->head.localToWorld, &dir, vin);
    *dirPDF = 1.0f / (pow(-dir.z, 3) * info->virtualPlaneArea);
    
    return colorOne;
}

//color We(const uchar* IDF, const vector3* vin) {
//
//}

inline float absCosNsIDF(const uchar* IDF, const vector3* v) {
    return fabs(dot(((IDFHead*)IDF)->dir, *v));
}

#endif
