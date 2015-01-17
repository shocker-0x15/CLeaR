//
//  light.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef device_light_cl
#define device_light_cl

#include "global.cl"
#include "scene.cl"
#include "texture.cl"
#include "material_structures.cl"

typedef enum {
    EEDF_Diffuse      = 1 << 0,
    EEDF_Varying      = 1 << 1,
    EEDF_Directional  = 1 << 2,
    
    EEDF_Non_Singualar    = EEDF_Diffuse | EEDF_Varying,
    EEDF_All_Types        = EEDF_Diffuse | EEDF_Varying | EEDF_Directional,
} EEDFType;

typedef enum {
    EEDFID_DiffuseEmission = 0,
} EEDFID;

typedef enum {
    EnvEEDFID_EntireSceneEmission = 0,
} EnvEEDFID;


// 12bytes
typedef struct __attribute__((aligned(4))) {
    float uLight;
    float uPos[2];
} LightSample;

// 8bytes
typedef struct __attribute__((aligned(4))) {
//    float uComponent;
    float uDir[2];
} EDFSample;


// 80bytes
typedef struct __attribute__((aligned(16))) {
    DDFHead ddfHead;
    uchar numEEDFs __attribute__((aligned(2)));// 2バイトアラインしなかったら何故か死ぬ。
    ushort offsetsEEDFs[4] __attribute__((aligned(2)));
    vector3 n __attribute__((aligned(16))), s, t, ng;
} EDFHead;

// 8bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    EEDFType eLeType __attribute__((aligned(4)));
} EEDFHead;

// 32bytes
typedef struct __attribute__((aligned(16))) {
    EEDFHead head;
    color M __attribute__((aligned(16)));
} DiffuseEmission;


// 12bytes
typedef struct __attribute__((aligned(2))) {
    DDFHead ddfHead;
    uchar numEnvEEDFs __attribute__((aligned(2)));
    ushort offsetsEnvEEDFs[4] __attribute__((aligned(2)));
} EnvEDFHead;

// 8bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    EEDFType eLeType __attribute__((aligned(4)));
} EnvEEDFHead;

// 32bytes
typedef struct __attribute__((aligned(16))) {
    EnvEEDFHead head;
    color Le __attribute__((aligned(16)));
} EntireSceneEmission;

//------------------------

static inline float l_cosTheta(const vector3* v);
static inline float l_absCosTheta(const vector3* v);
static inline float l_sinTheta2(const vector3* v);
static inline float l_sinTheta(const vector3* v);
static float l_cosPhi(const vector3* v);
static float l_sinPhi(const vector3* v);

void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                    LightPoint* lpos, uchar* EDF, float* areaPDF);
float getAreaPDF(const Scene* scene, const LightPoint* lpos);

void EDFAlloc(const Scene* scene, uint offset, const LightPoint* lpos, uchar* EDF);

static color eLe(const EEDFHead* EEDF, const vector3* vout);
static color env_eLe(const EnvEEDFHead* envEEDF, const vector3* vout);

color Le(const uchar* EDF, const vector3* vout);
inline float absCosNsEDF(const uchar* EDF, const vector3* v);

//------------------------

static inline float l_cosTheta(const vector3* v) {
    return v->z;
}

static inline float l_absCosTheta(const vector3* v) {
    return fabs(v->z);
}

static inline float l_sinTheta2(const vector3* v) {
    return fmax(1.0f - v->z * v->z, 0.0f);
}

static inline float l_sinTheta(const vector3* v) {
    return sqrt(l_sinTheta2(v));
}

static float l_cosPhi(const vector3* v) {
    float sinT = l_sinTheta(v);
    if (sinT == 0.0f) return 1.0f;
    return clamp(v->x / sinT, -1.0f, 1.0f);
}

static float l_sinPhi(const vector3* v) {
    float sinT = l_sinTheta(v);
    if (sinT == 0.0f) return 0.0f;
    return clamp(v->y / sinT, -1.0f, 1.0f);
}


void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                    LightPoint* lpos, uchar* EDF, float* areaPDF) {
    LightInfo lInfo = scene->lights[sampleDiscrete1D(scene->lightPowerDistribution, l_sample->uLight, areaPDF)];
    ushort lightPropPtr = USHRT_MAX;
    if (lInfo.atInfinity) {
        lpos->atInfinity = true;
        
        const global uchar* lightsData_p = scene->materialsData + scene->environment->idx_envLightProperty;
        const global LightPropertyInfo* lpInfo = (const global LightPropertyInfo*)lightsData_p;
        
        // IBLの重ね合わせを考える場合は、複合BSDFからのサンプリングのように1sample MISを行った方が良い？
        // ただ全方位のIBLを重ね合わせたい需要はあんまり無さそうだからMISしなくても良いかも。
        *areaPDF *= 1.0f;// 本当はここで複合IBLから1要素を確率的にサンプリングする。
        uint whichIBL = 0;
        
        float uvPDF;
        lightsData_p += sizeof(LightPropertyInfo);
        for (uint i = 0; i < lpInfo->numEEDFs; ++i) {
            AlignPtrG(&lightsData_p, 4);
            uchar EnvLPElemID = *lightsData_p;
            switch (EnvLPElemID) {
                case EnvLPElem_ImageBased: {
                    const global ImageBasedEnvLElem* llIBEnvElem = (const global ImageBasedEnvLElem*)lightsData_p;
                    
                    if (whichIBL == i) {
                        lpos->uv = sampleContinuousConsts2D_H((const global ContinuousConsts2D_H*)(scene->otherResourcesData + llIBEnvElem->idx_Dist2D),
                                                              l_sample->uPos[0], l_sample->uPos[1], &uvPDF);
                        float theta = lpos->uv.s1 * M_PI_F;
                        float phi = lpos->uv.s0 * 2 * M_PI_F;
                        float sinTheta = sin(theta);
                        lpos->p = (point3)(-sin(phi) * sinTheta, cos(theta), cos(phi) * sinTheta);
                        *areaPDF *= uvPDF / (2 * M_PI_F * M_PI_F * sinTheta);
                        break;
                    }
                    
                    lightsData_p += sizeof(ImageBasedEnvLElem);
                    break;
                }
                default: {
                    break;
                }
            }
        }
        
//        for (uint i = 0; i < lpInfo->numEEDFs; ++i) {
//            // 各IBLが選択される確率の加重平均をとる処理。
//        }
    }
    else {
        lpos->atInfinity = false;
        
        lpos->faceID = lInfo.reference;
        const global Face* face = scene->faces + lpos->faceID;
        const global point3* p0 = scene->vertices + face->p0;
        const global point3* p1 = scene->vertices + face->p1;
        const global point3* p2 = scene->vertices + face->p2;
        
        float b0, b1, b2;
        uniformSampleTriangle(l_sample->uPos[0], l_sample->uPos[1], &b0, &b1);
        b2 = 1.0f - b0 - b1;
        
        vector3 ng = cross(*p1 - *p0, *p2 - *p0);
        
        float area = 0.5f * length(ng);
        *areaPDF *= 1.0f / area;
        
        lpos->p = b0 * *p0 + b1 * *p1 + b2 * *p2;
        lpos->gNormal = normalize(ng);
        
        bool hasVNormal = face->vn0 != UINT_MAX && face->vn1 != UINT_MAX && face->vn2 != UINT_MAX;
        if (hasVNormal)
            lpos->sNormal = normalize(b0 * *(scene->normals + face->vn0) +
                                      b1 * *(scene->normals + face->vn1) +
                                      b2 * *(scene->normals + face->vn2));
        else
            lpos->sNormal = lpos->gNormal;
        
        lpos->hasTangent = face->vt0 != UINT_MAX && face->vt1 != UINT_MAX && face->vt2 != UINT_MAX;
        if (lpos->hasTangent)
            lpos->sTangent = normalize(b0 * *(scene->tangents + face->vt0) +
                                       b1 * *(scene->tangents + face->vt1) +
                                       b2 * *(scene->tangents + face->vt2));
        
        bool hasUV = face->uv0 != UINT_MAX && face->uv1 != UINT_MAX && face->uv2 != UINT_MAX;
        if (hasUV) {
            float2 uv0 = *(scene->uvs + face->uv0);
            float2 uv1 = *(scene->uvs + face->uv1);
            float2 uv2 = *(scene->uvs + face->uv2);
            lpos->uv = b0 * uv0 + b1 * uv1 + b2 * uv2;
            
            float2 dUV0m2 = uv0 - uv2;
            float2 dUV1m2 = uv1 - uv2;
            float3 dP0m2 = *p0 - *p2;
            float3 dP1m2 = *p1 - *p2;
            float invDetUV = 1.0f / (dUV0m2.x * dUV1m2.y - dUV0m2.y * dUV1m2.x);
            float3 uDir = invDetUV * (float3)(dUV1m2.y * dP0m2.x - dUV0m2.y * dP1m2.x,
                                              dUV1m2.y * dP0m2.y - dUV0m2.y * dP1m2.y,
                                              dUV1m2.y * dP0m2.z - dUV0m2.y * dP1m2.z);
            if (hasVNormal)
                lpos->uDir = normalize(cross(cross(lpos->sNormal, uDir), lpos->sNormal));
            else
                lpos->uDir = normalize(uDir);
        }
        
        lightPropPtr = face->lightPtr;
    }
    EDFAlloc(scene, lightPropPtr, lpos, EDF);
}

float getAreaPDF(const Scene* scene, const LightPoint* lpos) {
    if (lpos->atInfinity) {
        const global uchar* lightsData_p = scene->materialsData + scene->environment->idx_envLightProperty;
        const global LightPropertyInfo* lpInfo = (const global LightPropertyInfo*)lightsData_p;
        
        float areaPDF = 0.0f;
        float uvPDF;
        lightsData_p += sizeof(LightPropertyInfo);
        for (uint i = 0; i < lpInfo->numEEDFs; ++i) {
            AlignPtrG(&lightsData_p, 4);
            uchar EnvLPElemID = *lightsData_p;
            switch (EnvLPElemID) {
                case EnvLPElem_ImageBased: {
                    const global ImageBasedEnvLElem* llIBEnvElem = (const global ImageBasedEnvLElem*)lightsData_p;
                    
                    uvPDF = PDFContinuousConsts2D_H((const global ContinuousConsts2D_H*)(scene->otherResourcesData + llIBEnvElem->idx_Dist2D), &lpos->uv);
                    areaPDF += uvPDF / (2 * M_PI_F * M_PI_F * sin(lpos->uv.s1 * M_PI_F)) * 1.0f;// 本来はこのIBLが選ばれる確率をかける必要がある。
                    
                    lightsData_p += sizeof(ImageBasedEnvLElem);
                    break;
                }
                default: {
                    break;
                }
            }
        }
        
        for (uint i = 0; i < scene->lightPowerDistribution->numItems; ++i)
            if (scene->lights[i].atInfinity)
                return areaPDF * probDiscrete1D(scene->lightPowerDistribution, i);
    }
    else {
        const global Face* face = scene->faces + lpos->faceID;
        const global point3* p0 = scene->vertices + face->p0;
        const global point3* p1 = scene->vertices + face->p1;
        const global point3* p2 = scene->vertices + face->p2;
        
        vector3 ng = cross(*p1 - *p0, *p2 - *p0);
        
        float areaPDF = 1.0f / (0.5f * length(ng));
        for (uint i = 0; i < scene->lightPowerDistribution->numItems; ++i)
            if (scene->lights[i].reference == lpos->faceID)
                return areaPDF * probDiscrete1D(scene->lightPowerDistribution, i);
    }
    return 0.0f;
}


void EDFAlloc(const Scene* scene, uint offset, const LightPoint* lpos, uchar* EDF) {
    if (lpos->atInfinity) {
        EnvEDFHead* envEDFHead = (EnvEDFHead*)EDF;
        envEDFHead->ddfHead._type = DDFType_EnvEDF;
        const global uchar* lightsData_p = scene->materialsData + scene->environment->idx_envLightProperty;
        const global LightPropertyInfo* lpInfo = (const global LightPropertyInfo*)lightsData_p;
        
        envEDFHead->numEnvEEDFs = lpInfo->numEEDFs;
        envEDFHead->offsetsEnvEEDFs[0] = envEDFHead->offsetsEnvEEDFs[1] =
        envEDFHead->offsetsEnvEEDFs[2] = envEDFHead->offsetsEnvEEDFs[3] = 0;
        
        uchar* EDFp = EDF + sizeof(EnvEDFHead);
        lightsData_p += sizeof(LightPropertyInfo);
        for (int i = 0; i < envEDFHead->numEnvEEDFs; ++i) {
            AlignPtrG(&lightsData_p, 4);
            uchar EnvLPElemID = *lightsData_p;
            switch (EnvLPElemID) {
                case EnvLPElem_ImageBased: {
                    AlignPtr(&EDFp, 16);
                    envEDFHead->offsetsEnvEEDFs[i] = (ushort)((uintptr_t)EDFp - (uintptr_t)EDF);
                    
                    EntireSceneEmission* entire = (EntireSceneEmission*)EDFp;
                    const global ImageBasedEnvLElem* llIBEnvElem = (const global ImageBasedEnvLElem*)lightsData_p;
                    
                    entire->head.id = EnvEEDFID_EntireSceneEmission;
                    entire->head.eLeType = EEDF_Diffuse;
                    entire->Le = evaluateColorTexture(scene->texturesData + llIBEnvElem->idx_Le, lpos->uv) * llIBEnvElem->multiplier;
                    
                    EDFp += sizeof(EntireSceneEmission);
                    lightsData_p += sizeof(ImageBasedEnvLElem);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    else {
        EDFHead* LeHead = (EDFHead*)EDF;
        LeHead->ddfHead._type = DDFType_EDF;
        const global uchar* lightsData_p = scene->materialsData + offset;
        const global LightPropertyInfo* lpInfo = (const global LightPropertyInfo*)lightsData_p;
        
        LeHead->numEEDFs = lpInfo->numEEDFs;
        LeHead->offsetsEEDFs[0] = LeHead->offsetsEEDFs[1] =
        LeHead->offsetsEEDFs[2] = LeHead->offsetsEEDFs[3] = 0;
        
        LeHead->n = lpos->sNormal;
        if (lpos->hasTangent)
            LeHead->s = lpos->sTangent;
        else
            makeTangent(&lpos->sNormal, &LeHead->s);
        
        LeHead->t = cross(LeHead->n, LeHead->s);
        LeHead->ng = lpos->gNormal;
        
        uchar* EDFp = EDF + sizeof(EDFHead);
        lightsData_p += sizeof(LightPropertyInfo);
        for (int i = 0; i < LeHead->numEEDFs; ++i) {
            AlignPtrG(&lightsData_p, 4);
            uchar LPElemID = *lightsData_p;
            switch (LPElemID) {
                case LPElem_DiffuseEmission: {
                    AlignPtr(&EDFp, 16);
                    LeHead->offsetsEEDFs[i] = (ushort)((uintptr_t)EDFp - (uintptr_t)EDF);
                    
                    DiffuseEmission* diffuse = (DiffuseEmission*)EDFp;
                    const global DiffuseLElem* diffuseElem = (const global DiffuseLElem*)lightsData_p;
                    
                    diffuse->head.id = EEDFID_DiffuseEmission;
                    diffuse->head.eLeType = EEDF_Diffuse;
                    diffuse->M = evaluateColorTexture(scene->texturesData + diffuseElem->idx_M, lpos->uv);
                    
                    EDFp += sizeof(DiffuseEmission);
                    lightsData_p += sizeof(DiffuseLElem);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
}


static color eLe(const EEDFHead* EEDF, const vector3* vout) {
    switch (EEDF->id) {
        case EEDFID_DiffuseEmission: {
            const DiffuseEmission* difEmit = (const DiffuseEmission*)EEDF;
            return vout->z > 0.0f ? difEmit->M / M_PI_F : colorZero;
        }
        default: {
            break;
        }
    }
    return colorZero;
}

static color env_eLe(const EnvEEDFHead* envEEDF, const vector3* vout) {
    switch (envEEDF->id) {
        case EnvEEDFID_EntireSceneEmission: {
            const EntireSceneEmission* entire = (const EntireSceneEmission*)envEEDF;
            return entire->Le;
        }
        default: {
            break;
        }
    }
    return colorZero;
}


color Le(const uchar* EDF, const vector3* vout) {
    const DDFHead* ddfHead = (const DDFHead*)EDF;
    if (ddfHead->_type == DDFType_EDF) {
        const EDFHead* head = (const EDFHead*)EDF;
        const vector3* s = &head->s;
        const vector3* t = &head->t;
        const vector3* n = &head->n;
        //const vector3* ng = &head->ng;
        vector3 voutLocal = worldToLocal(s, t, n, vout);
        
        color Le = colorZero;
        for (int i = 0; i < head->numEEDFs; ++i) {
            const EEDFHead* iLe = (const EEDFHead*)(EDF + head->offsetsEEDFs[i]);
            Le += eLe(iLe, &voutLocal);
        }
        return Le;
    }
    else {
        const EnvEDFHead* head = (const EnvEDFHead*)EDF;
        
        color Le = colorZero;
        for (int i = 0; i < head->numEnvEEDFs; ++i) {
            const EnvEEDFHead* iLe = (const EnvEEDFHead*)(EDF + head->offsetsEnvEEDFs[i]);
            Le += env_eLe(iLe, vout);
        }
        return Le;
    }
}

inline float absCosNsEDF(const uchar* EDF, const vector3* v) {
    return ((const DDFHead*)EDF)->_type == DDFType_EDF ? fabs(dot(((const EDFHead*)EDF)->n, *v)) : 1.0f;
}

#endif
