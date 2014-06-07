#ifndef sim_camera_h
#define sim_camera_h

#include "sim_global.h"
#include "sim_rng.h"
#include "sim_matrix.h"

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
    
    //64bytes
    typedef struct __attribute__((aligned(16))) {
        vector3 n, s, t, ng;
    } IDFHead;
    
    //------------------------
    
    void sampleLensPos(const Scene* scene, const CameraSample* sample, LensPosition* lpos, float* areaPDF);
    
    inline float absCosNsIDF(const uchar* IDF, const vector3* v);
    
    //static inline float cosTheta(const vector3* v);
    //static inline float absCosTheta(const vector3* v);
    //static inline float sinTheta2(const vector3* v);
    //static inline float sinTheta(const vector3* v);
    //static inline float cosPhi(const vector3* v);
    //static inline float sinPhi(const vector3* v);
    
    void IDFAlloc(const Scene* scene, const LensPosition* lpos, uchar* IDF);
    
    color sample_We(const uchar* IDF, const IDFSample* sample, vector3* vin, float* dirPDF);
    //color We(const uchar* IDF, const vector3* vin);
    
    //------------------------
    
    void sampleLensPos(const Scene* scene, const CameraSample* sample, LensPosition* lpos, float* areaPDF) {
        const CameraHead* head = (const CameraHead*)scene->camera;
        
        float ux, uy;
        concentricSampleDisk(sample->uLens[0], sample->uLens[1], &ux, &uy);
        
        vector3 lCamDir = vector3(0.0f, 0.0f, 1.0f);
        mulMat4x4G_V3(&head->localToWorld, &lCamDir, &lpos->n);
        
        const PerspectiveInfo* info = (const PerspectiveInfo*)head;
        
        point3 lensPosLocal = point3(ux * info->lensRadius, uy * info->lensRadius, 0.0f);
        mulMat4x4G_P3(&head->localToWorld, &lensPosLocal, &lpos->p);
        
        *areaPDF = 1.0f / (M_PI_F * info->lensRadius * info->lensRadius);
        lpos->uv = sw2(lensPosLocal, x, y);
    }
    
    inline float absCosNsIDF(const uchar* IDF, const vector3* v) {
        return fabs(dot(((IDFHead*)IDF)->n, *v));
    }
    
    void IDFAlloc(const Scene* scene, const LensPosition* lpos, uchar* IDF) {
        IDFHead* WeHead = (IDFHead*)IDF;
        
        WeHead->n = lpos->n;
        makeTangent(&WeHead->n, &WeHead->s);
        WeHead->t = cross(WeHead->n, WeHead->s);
        WeHead->ng = WeHead->n;
        
        PerspectiveIDF* perspective = (PerspectiveIDF*)(IDF + sizeof(IDFHead));
        perspective->localOrigin = point3(lpos->uv, 0.0f);
        perspective->info = (const PerspectiveInfo*)scene->camera;
    }
    
    color sample_We(const uchar* IDF, const IDFSample* sample, vector3* vin, float* dirPDF) {
        const IDFHead* head = (const IDFHead*)IDF;
        const vector3* s = &head->s;
        const vector3* t = &head->t;
        const vector3* n = &head->n;
        const vector3* ng = &head->ng;
        
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
}

#endif