#ifndef sim_light_cl
#define sim_light_cl

#include "sim_global.hpp"
#include "sim_rng.hpp"
#include "sim_texture.hpp"
#include "sim_materials.hpp"

namespace sim {
    typedef enum {
        EEDFID_DiffuseEmission = 0,
    } EEDFID;
    
    typedef enum {
        EEDF_Diffuse      = 1 << 0,
        EEDF_Varying      = 1 << 1,
        EEDF_Directional  = 1 << 2,
        
        EEDF_Non_Singualar    = EEDF_Diffuse | EEDF_Varying,
        EEDF_All_Types        = EEDF_Diffuse | EEDF_Varying | EEDF_Directional,
    } EEDFType;
    
    typedef enum {
        EnvID_LatitudeLongitude = 0,
    } EnvID;
    
    //12bytes
    typedef struct {
        float uLight;
        float uPos[2];
    } LightSample;
    
    //8bytes
    typedef struct {
        //    float uComponent;
        float uDir[2];
    } EDFSample;
    
    
    //80bytes
    typedef struct {
        DDFHead ddfHead; uchar dum0;
        uchar numEEDFs; uchar dum1;//2バイトアラインしなかったら何故か死ぬ。
        ushort offsetsEEDFs[4]; uchar dum2[4];
        vector3 n, s, t, ng;
    } EDFHead;
    
    //8bytes
    typedef struct {
        uchar id; uchar dum[3];
        EEDFType eLeType;
    } EEDFHead;
    
    //32bytes
    typedef struct {
        EEDFHead head; uchar dum[8];
        color M;
    } DiffuseEmission;
    
    //------------------------
    
    static inline float l_cosTheta(const vector3* v);
    static inline float l_absCosTheta(const vector3* v);
    static inline float l_sinTheta2(const vector3* v);
    static inline float l_sinTheta(const vector3* v);
    static float l_cosPhi(const vector3* v);
    static float l_sinPhi(const vector3* v);
    
    void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                        LightPosition* lpos, uchar* EDF, float* areaPDF);
    float getAreaPDF(const Scene* scene, uint faceID, float2 uv);
    
    void EDFAlloc(const Scene* scene, uint offset, const LightPosition* lpos, uchar* EDF);
    
    static color eLe(const EEDFHead* EEDF, const vector3* vout);
    
    color Le(const uchar* EDF, const vector3* vout);
    inline float absCosNsEDF(const uchar* EDF, const vector3* v);
    
    //------------------------
    
    static inline float l_cosTheta(const vector3* v) {
        return v->z;
    }
    
    static inline float l_absCosTheta(const vector3* v) {
        return fabsf(v->z);
    }
    
    static inline float l_sinTheta2(const vector3* v) {
        return fmaxf(1.0f - v->z * v->z, 0.0f);
    }
    
    static inline float l_sinTheta(const vector3* v) {
        return sqrtf(l_sinTheta2(v));
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
                        LightPosition* lpos, uchar* EDF, float* areaPDF) {
        LightInfo lInfo = scene->lights[sampleDiscrete1D(scene->lightPowerCDF, l_sample->uLight, areaPDF)];
        if (lInfo.atInfinity) {
            
        }
        else {
            lpos->faceID = lInfo.reference;
            const Face* face = scene->faces + lpos->faceID;
            const point3* p0 = scene->vertices + face->p0;
            const point3* p1 = scene->vertices + face->p1;
            const point3* p2 = scene->vertices + face->p2;
            
            float b0, b1, b2;
            uniformSampleTriangle(l_sample->uPos[0], l_sample->uPos[1], &b0, &b1);
            b2 = 1.0f - b0 - b1;
            
            vector3 ng = cross(*p1 - *p0, *p2 - *p0);
            
            float area = 0.5f * length(ng);
            *areaPDF = 1.0f / (area * scene->numLights);
            
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
                float3 uDir = invDetUV * float3(dUV1m2.y * dP0m2.x - dUV0m2.y * dP1m2.x,
                                                dUV1m2.y * dP0m2.y - dUV0m2.y * dP1m2.y,
                                                dUV1m2.y * dP0m2.z - dUV0m2.y * dP1m2.z);
                if (hasVNormal)
                    lpos->uDir = normalize(cross(cross(lpos->sNormal, uDir), lpos->sNormal));
                else
                    lpos->uDir = normalize(uDir);
            }
            
            EDFAlloc(scene, face->lightPtr, lpos, EDF);
        }
    }
    
    float getAreaPDF(const Scene* scene, uint faceID, float2 uv) {
        const Face* face = scene->faces + faceID;
        const point3* p0 = scene->vertices + face->p0;
        const point3* p1 = scene->vertices + face->p1;
        const point3* p2 = scene->vertices + face->p2;
        
        vector3 ng = cross(*p1 - *p0, *p2 - *p0);
        
        float area = 0.5f * length(ng);
        return 1.0f / (area * scene->numLights);
    }
    
    
    void EDFAlloc(const Scene* scene, uint offset, const LightPosition* lpos, uchar* EDF) {
        EDFHead* LeHead = (EDFHead*)EDF;
        LeHead->ddfHead._type = DDFType_EDF;
        const uchar* lightsData_p = scene->materialsData + offset;
        const LightPropertyInfo* lpInfo = (const LightPropertyInfo*)lightsData_p;
        
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
            uchar EEDFID = *lightsData_p;
            switch (EEDFID) {
                case EEDFID_DiffuseEmission: {
                    AlignPtr(&EDFp, 16);
                    LeHead->offsetsEEDFs[i] = (ushort)((uintptr_t)EDFp - (uintptr_t)EDF);
                    
                    DiffuseEmission* diffuse = (DiffuseEmission*)EDFp;
                    DiffuseEDFInfo* diffuseInfo = (DiffuseEDFInfo*)lightsData_p;
                    
                    diffuse->head.id = EEDFID;
                    diffuse->head.eLeType = EEDF_Diffuse;
                    diffuse->M = evaluateColorTexture(scene->texturesData + diffuseInfo->idx_M, lpos->uv);
                    
                    EDFp += sizeof(DiffuseEmission);
                    lightsData_p += sizeof(DiffuseEDFInfo);
                    break;
                }
                default: {
                    break;
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
    
    
    color Le(const uchar* EDF, const vector3* vout) {
        const EDFHead* head = (const EDFHead*)EDF;
        const vector3* s = &head->s;
        const vector3* t = &head->t;
        const vector3* n = &head->n;
        const vector3* ng = &head->ng;
        vector3 voutLocal = worldToLocal(s, t, n, vout);
        
        color Le = colorZero;
        for (int i = 0; i < head->numEEDFs; ++i) {
            ushort idxEEDF = head->offsetsEEDFs[i];
            Le += eLe((EEDFHead*)(EDF + idxEEDF), &voutLocal);
        }
        return Le;
    }
    
    inline float absCosNsEDF(const uchar* EDF, const vector3* v) {
        return fabsf(dot(((const EDFHead*)EDF)->n, *v));
    }
}

#endif