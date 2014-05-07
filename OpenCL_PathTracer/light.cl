#ifndef light_cl
#define light_cl

#include "global.cl"
#include "rng.cl"
#include "texture.cl"

typedef enum {
    EEDF_Diffuse      = 1 << 0,
    EEDF_Varying      = 1 << 1,
    EEDF_Directional  = 1 << 2,
    
    EEDF_Non_Singualar    = EEDF_Diffuse | EEDF_Varying,
    EEDF_All_Types        = EEDF_Diffuse | EEDF_Varying | EEDF_Directional,
} EEDFType;

typedef struct {
    float uLight;
    float uPos[2];
} LightSample;

typedef struct {
//    float uComponent;
    float uDir[2];
} EDFSample;

typedef struct {
    uchar __attribute__((aligned(4))) id;
    EEDFType eLeType;
} EEDFHead;

typedef struct {
    EEDFHead head;
    color M;
} DiffuseEmission;

typedef struct {
    uchar __attribute__((aligned(2))) numEEDFs;
    ushort __attribute__((aligned(2))) offsetsEEDFs[4];
    vector3 n, s, t, ng;
} EDFHead;

//------------------------

void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                    LightPosition* lpos, float* areaPDF);
inline float absCosNsEDF(const EDFHead* EDF, const vector3* v);
//static inline float cosTheta(const vector3* v);
//static inline float absCosTheta(const vector3* v);
//static inline float sinTheta2(const vector3* v);
//static inline float sinTheta(const vector3* v);
//static inline float cosPhi(const vector3* v);
//static inline float sinPhi(const vector3* v);
void EDFAlloc(const Scene* scene, const global uchar* lightsData, uint offset, const LightPosition* lpos, EDFHead* EDF);
static color eLe(const EEDFHead* EEDF, const vector3* vout);
color Le(const uchar* EDF, const vector3* vout);

//------------------------

void sampleLightPos(const Scene* scene, const LightSample* l_sample, const point3* shdP,
                    LightPosition* lpos, float* areaPDF) {
    lpos->faceID = scene->lights[randUInt(l_sample->uLight, scene->numLights)];
    const global Face* face = scene->faces + lpos->faceID;
    const global point3* p0 = scene->vertices + face->p0;
    const global point3* p1 = scene->vertices + face->p1;
    const global point3* p2 = scene->vertices + face->p2;
    
    float b0, b1, b2;
    uniformSampleTriangle(l_sample->uPos[0], l_sample->uPos[1], &b0, &b1);
    b2 = 1.0f - b0 - b1;
    
    vector3 ng = cross(*p1 - *p0, *p2 - *p0);
    
    float area = 0.5f * length(ng);
    *areaPDF = 1.0f / (area * scene->numLights);
    
    lpos->p = b0 * *p0 + b1 * *p1 + b2 * *p2;
    lpos->ng = normalize(ng);
    
    if (face->uv0 != UINT_MAX && face->uv1 != UINT_MAX && face->uv2 != UINT_MAX) {
        lpos->uv = b0 * *(scene->uvs + face->uv0) + b1 * *(scene->uvs + face->uv1) + b2 * *(scene->uvs + face->uv2);
    }
}

inline float absCosNsEDF(const EDFHead* EDF, const vector3* v) {
    return fabs(dot(EDF->n, *v));
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

void EDFAlloc(const Scene* scene, const global uchar* lightsData, uint offset, const LightPosition* lpos, EDFHead* EDF) {
    EDF->numEEDFs = 1;
    EDF->offsetsEEDFs[0] = EDF->offsetsEEDFs[1] =
    EDF->offsetsEEDFs[2] = EDF->offsetsEEDFs[3] = 0;
    
    const global uchar* lightsData_p = lightsData + offset;
    
    // n
    EDF->n = lpos->ng;
    
    vector3 s_temp, t_temp;
    makeBasis(&lpos->ng, &s_temp, &t_temp);
    // st
    EDF->s = s_temp;
    //    vector3 t = cross(n, s);
    EDF->t = t_temp;
    // ng
    EDF->ng = lpos->ng;
    
    EDF->offsetsEEDFs[0] = sizeof(EDFHead);
    uchar* EDFp = (uchar*)EDF + sizeof(EDFHead);
    uchar EEDFID = *(lightsData_p++);
    EEDFType FType;
    switch (EEDFID) {
        case 0: {
            *(EDFp++) = 0;
            break;
        }
        case 1: {// Diffuse Light
            *(EDFp++) = 1;
            
            FType = EEDF_Diffuse;
            memcpy(AlignPtrAdd(&EDFp, sizeof(EEDFType)), &FType, sizeof(EEDFType));
            
            // Emittance
            color M = evaluateColorTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&lightsData_p, sizeof(uint)), lpos->uv);
            memcpy(AlignPtrAdd(&EDFp, sizeof(color)), &M, sizeof(color));
            break;
        }
        default: {
            break;
        }
    }
}

static color eLe(const EEDFHead* EEDF, const vector3* vout) {
    switch (EEDF->id) {
        case 1: {
            const DiffuseEmission* difEmit = (const DiffuseEmission*)EEDF;
            return vout->z > 0.0f ? difEmit->M / M_PI_F : (color)(0.0f, 0.0f, 0.0f);
        }
        case 0:
        default: {
            break;
        }
    }
    return (color)(0.0f, 0.0f, 0.0f);
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
    color Le = (color)(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < head->numEEDFs; ++i) {
        ushort idxEEDF = head->offsetsEEDFs[i];
        Le += eLe(EDF + idxEEDF, &voutLocal);
    }
    return Le;
}

#endif