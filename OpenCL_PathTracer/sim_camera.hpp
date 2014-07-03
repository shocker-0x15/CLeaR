#ifndef sim_camera_h
#define sim_camera_h

#include "sim_global.hpp"
#include "sim_rng.hpp"
#include "sim_matrix.hpp"

namespace sim {
    //8bytes
    typedef struct {
        //    float uCamera;
        float uLens[2];
    } CameraSample;
    
    //8bytes
    typedef struct {
        //    float uComponent;
        float uDir[2];
    } IDFSample;
    
    //256bytes
    typedef struct {
        CameraHead head;
        uchar id; uchar dum0[3];
        float virtualPlaneArea;
        float lensRadius;
        float objPDistance; uchar dum1[48];
        mat4x4 rasterToCamera;
    } PerspectiveInfo;
    
    //32bytes
    typedef struct {
        point3 localOrigin;
        const PerspectiveInfo* info; uchar dum[8];
    } PerspectiveIDF;
    
    //32bytes
    typedef struct {
        DDFHead ddfHead; uchar dum0[15];
        vector3 dir;
    } IDFHead;
    
    //------------------------
    
    void sampleLensPos(const Scene* scene, const CameraSample* sample, LensPosition* lpos, uchar* IDF, float* areaPDF);
    
    void IDFAlloc(const Scene* scene, const LensPosition* lpos, uchar* IDF);
    
    color sample_We(const uchar* IDF, const IDFSample* sample, vector3* vin, float* dirPDF);
    inline float absCosNsIDF(const uchar* IDF, const vector3* v);
    
    //------------------------
    
    void sampleLensPos(const Scene* scene, const CameraSample* sample, LensPosition* lpos, uchar* IDF, float* areaPDF) {
        const CameraHead* head = (const CameraHead*)scene->camera;
        
        float ux, uy;
        concentricSampleDisk(sample->uLens[0], sample->uLens[1], &ux, &uy);
        
        vector3 lCamDir = vector3(0.0f, 0.0f, 1.0f);
        mulMat4x4G_V3(&head->localToWorld, &lCamDir, &lpos->dir);
        
        const PerspectiveInfo* info = (const PerspectiveInfo*)head;
        
        point3 lensPosLocal = point3(ux * info->lensRadius, uy * info->lensRadius, 0.0f);
        mulMat4x4G_P3(&head->localToWorld, &lensPosLocal, &lpos->p);
        
        *areaPDF = 1.0f / (M_PI_F * info->lensRadius * info->lensRadius);
        lpos->uv = sw2(lensPosLocal, x, y);
        
        IDFAlloc(scene, lpos, IDF);
    }
    
    
    void IDFAlloc(const Scene* scene, const LensPosition* lpos, uchar* IDF) {
        IDFHead* WeHead = (IDFHead*)IDF;
        WeHead->ddfHead._type = DDFType_PerspectiveIDF;
        
        WeHead->dir = lpos->dir;
        
        PerspectiveIDF* perspective = (PerspectiveIDF*)(IDF + sizeof(IDFHead));
        perspective->localOrigin = point3(lpos->uv, 0.0f);
        perspective->info = (const PerspectiveInfo*)scene->camera;
    }
    
    color sample_We(const uchar* IDF, const IDFSample* sample, vector3* vin, float* dirPDF) {
        const IDFHead* head = (const IDFHead*)IDF;
        
        const PerspectiveIDF* pIDF = (const PerspectiveIDF*)(head + 1);
        const PerspectiveInfo* info = pIDF->info;
        
        point4 pRas = point4(sample->uDir[0], sample->uDir[1], 0, 1);
        point4 pCamera;
        mulMat4x4G_P4(&info->rasterToCamera, &pRas, &pCamera);
        vector3 dir = normalize(sw3(pCamera, x, y, z));
        
        if (info->lensRadius > 0.0f) {
            float ft = -info->objPDistance / dir.z;
            point3 pFocus = point3(0, 0, 0) + ft * dir;
            
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
        return fabsf(dot(((IDFHead*)IDF)->dir, *v));
    }
}

#endif