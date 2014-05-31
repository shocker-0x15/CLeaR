#ifndef sim_light_cl
#define sim_light_cl

#include "sim_global.h"
#include "sim_rng.h"
#include "sim_texture.h"

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
    
    //80bytes
    typedef struct {
        vector3 n, s, t, ng;
        uchar numEEDFs; uchar dum0[1];
        ushort offsetsEEDFs[4]; uchar dum1[6];
    } EDFHead;
    
    //------------------------
    
    void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                        LightPosition* lpos, float* areaPDF);
    float getAreaPDF(const Scene* scene, uint faceID, float2 uv);
    
    inline float absCosNsEDF(const uchar* EDF, const vector3* v);
    
    //static inline float cosTheta(const vector3* v);
    //static inline float absCosTheta(const vector3* v);
    //static inline float sinTheta2(const vector3* v);
    //static inline float sinTheta(const vector3* v);
    //static inline float cosPhi(const vector3* v);
    //static inline float sinPhi(const vector3* v);
    
    void EDFAlloc(const Scene* scene, uint offset, const LightPosition* lpos, uchar* EDF);
    
    static color eLe(const EEDFHead* EEDF, const vector3* vout);
    color Le(const uchar* EDF, const vector3* vout);
    
    //------------------------
    
    void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                        LightPosition* lpos, float* areaPDF) {
        lpos->faceID = scene->lights[randUInt(l_sample->uLight, scene->numLights)];
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
        lpos->ng = normalize(ng);
        
        if (face->ns0 != UINT_MAX && face->ns1 != UINT_MAX && face->ns2 != UINT_MAX) {
            lpos->ns = normalize(b0 * *(scene->normals + face->ns0) +
                                 b1 * *(scene->normals + face->ns1) +
                                 b2 * *(scene->normals + face->ns2));
        }
        else {
            lpos->ns = lpos->ng;
        }
        
        if (face->uv0 != UINT_MAX && face->uv1 != UINT_MAX && face->uv2 != UINT_MAX) {
            lpos->uv = b0 * *(scene->uvs + face->uv0) + b1 * *(scene->uvs + face->uv1) + b2 * *(scene->uvs + face->uv2);
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
    
    inline float absCosNsEDF(const uchar* EDF, const vector3* v) {
        return fabsf(dot(((const EDFHead*)EDF)->n, *v));
    }
    
    //static inline float cosTheta(const vector3* v) {
    //    return v->z;
    //}
    //
    //static inline float absCosTheta(const vector3* v) {
    //    return fabs(v->z);
    //}
    //
    //static inline float sinTheta2(const vector3* v) {
    //    return fmax(1.0f - v->z * v->z, 0.0f);
    //}
    //
    //static inline float sinTheta(const vector3* v) {
    //    return sqrt(sinTheta2(v));
    //}
    //
    //static inline float cosPhi(const vector3* v) {
    //    float sinT = sinTheta(v);
    //    if (sinT == 0.0f) return 1.0f;
    //    return clamp(v->x / sinT, -1.0f, 1.0f);
    //}
    //
    //static inline float sinPhi(const vector3* v) {
    //    float sinT = sinTheta(v);
    //    if (sinT == 0.0f) return 0.0f;
    //    return clamp(v->y / sinT, -1.0f, 1.0f);
    //}
    
    void EDFAlloc(const Scene* scene, uint offset, const LightPosition* lpos, uchar* EDF) {
        EDFHead* LeHead = (EDFHead*)EDF;
        const uchar* lightsData_p = scene->lightsData + offset;
        
        LeHead->numEEDFs = *(lightsData_p++);
        LeHead->offsetsEEDFs[0] = LeHead->offsetsEEDFs[1] =
        LeHead->offsetsEEDFs[2] = LeHead->offsetsEEDFs[3] = 0;
        
        // n
        LeHead->n = lpos->ns;
        
        vector3 s_temp, t_temp;
        makeBasis(&lpos->ns, &s_temp, &t_temp);
        // st
        LeHead->s = s_temp;
        //    vector3 t = cross(n, s);
        LeHead->t = t_temp;
        // ng
        LeHead->ng = lpos->ng;
        
        uchar* EDFp = EDF + sizeof(EDFHead);
        EEDFType FType;
        for (int i = 0; i < LeHead->numEEDFs; ++i) {
            AlignPtr(&EDFp, 16);
            LeHead->offsetsEEDFs[i] = (ushort)((uintptr_t)EDFp - (uintptr_t)EDF);
            uchar EEDFID = *(lightsData_p++);
            *(EDFp++) = EEDFID;
            switch (EEDFID) {
                case EEDFID_DiffuseEmission: {
                    FType = EEDF_Diffuse;
                    memcpy(AlignPtrAdd(&EDFp, sizeof(EEDFType)), &FType, sizeof(EEDFType));
                    
                    // Emittance
                    color M = evaluateColorTexture(scene->texturesData + *(uint*)AlignPtrAddG(&lightsData_p, sizeof(uint)), lpos->uv);
                    memcpy(AlignPtrAdd(&EDFp, sizeof(color)), &M, sizeof(color));
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
        
        //    if (dot(*vin, *ng) * dot(*vout, *ng) > 0) // ignore BTDFs
        //        flags = BxDFType(flags & ~BxDF_Transmission);
        //    else // ignore BRDFs
        //        flags = BxDFType(flags & ~BxDF_Transmission);
        color Le = colorZero;
        for (int i = 0; i < head->numEEDFs; ++i) {
            ushort idxEEDF = head->offsetsEEDFs[i];
            Le += eLe((EEDFHead*)(EDF + idxEEDF), &voutLocal);
        }
        return Le;
    }
}

#endif