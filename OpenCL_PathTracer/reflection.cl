#ifndef reflection_cl
#define reflection_cl

#include "global.cl"
#include "rng.cl"
#include "texture.cl"

typedef enum {
    BxDF_Reflection   = 1 << 0,
    BxDF_Transmission = 1 << 1,
    
    BxDF_Diffuse      = 1 << 2,
    BxDF_Glossy       = 1 << 3,
    BxDF_Specular     = 1 << 4,
    
    BxDF_Non_Singualar    = BxDF_Diffuse | BxDF_Glossy,
    BxDF_All_Types        = BxDF_Diffuse | BxDF_Glossy | BxDF_Specular,
    BxDF_All_Reflection   = BxDF_Reflection | BxDF_All_Types,
    BxDF_All_Transmission = BxDF_Transmission | BxDF_All_Types,
    BxDF_All              = BxDF_All_Reflection | BxDF_All_Transmission
} BxDFType;

typedef struct {
    float uComponent;
    float uDir[2];
} BSDFSample;

typedef struct {
    uchar __attribute__((aligned(4))) id;
    BxDFType fxType;
} BxDFHead;

typedef struct {
    BxDFHead head;
    color R;
    float A, B;
} Diffuse;

typedef struct {
    BxDFHead head;
    color R;
} SpecularReflection;

typedef struct {
    BxDFHead head;
    color T;
} SpecularTransmission;

typedef struct {
    BxDFHead head;
    color R;
    float ax, ay;
} Ward;

typedef struct {
    uchar __attribute__((aligned(2))) numBxDFs;
    ushort __attribute__((aligned(2))) offsetsBxDFs[4];
    vector3 n, s, t, ng;
} BSDFHead;

//------------------------

inline bool hasNonSpecular(const BSDFHead* BSDF);
inline float absCosNsBSDF(const BSDFHead* BSDF, const vector3* v);
static inline float cosTheta(const vector3* v);
static inline float absCosTheta(const vector3* v);
static inline float sinTheta2(const vector3* v);
static inline float sinTheta(const vector3* v);
static inline float cosPhi(const vector3* v);
static inline float sinPhi(const vector3* v);
void BSDFAlloc(const Scene* scene, const global uchar* matsData, uint offset, const Intersection* isect, BSDFHead* BSDF);
static color sample_fx(const BxDFHead* BSDF, const vector3* vout, const BSDFSample* sample,
                       vector3* vin, float* dirPDF);
static color fx(const BxDFHead* BxDF, const vector3* vout, const vector3* vin);
color sample_fs(const uchar* BSDF, const vector3* vout, const BSDFSample* sample,
                vector3* vin, float* dirPDF, BxDFType* sampledType);
color fs(const uchar* BSDF, const vector3* vout, const vector3* vin);

//------------------------

inline bool hasNonSpecular(const BSDFHead* BSDF) {
    for (int i = 0; i < BSDF->numBxDFs; ++i) {
        BxDFHead* bxdf = (const uchar*)BSDF + BSDF->offsetsBxDFs[i];
        if((bool)(bxdf->fxType & BxDF_Non_Singualar))
            return true;
    }
    return false;
}

inline float absCosNsBSDF(const BSDFHead* BSDF, const vector3* v) {
    return fabs(dot(BSDF->n, *v));
}

static inline float cosTheta(const vector3* v) {
    return v->z;
}

static inline float absCosTheta(const vector3* v) {
    return fabs(v->z);
}

static inline float sinTheta2(const vector3* v) {
    return fmax(1.0f - v->z * v->z, 0.0f);
}

static inline float sinTheta(const vector3* v) {
    return sqrt(sinTheta2(v));
}

static inline float cosPhi(const vector3* v) {
    float sinT = sinTheta(v);
    if (sinT == 0.0f) return 1.0f;
    return clamp(v->x / sinT, -1.0f, 1.0f);
}

static inline float sinPhi(const vector3* v) {
    float sinT = sinTheta(v);
    if (sinT == 0.0f) return 0.0f;
    return clamp(v->y / sinT, -1.0f, 1.0f);
}

void BSDFAlloc(const Scene* scene, const global uchar* matsData, uint offset, const Intersection* isect, BSDFHead* BSDF) {
    BSDF->numBxDFs = 1;
    BSDF->offsetsBxDFs[0] = BSDF->offsetsBxDFs[1] =
    BSDF->offsetsBxDFs[2] = BSDF->offsetsBxDFs[3] = 0;
    
    const global uchar* matsData_p = matsData + offset;
    
    // n
    BSDF->n = isect->ng;
    
    vector3 s_temp, t_temp;
    makeBasis(&isect->ng, &s_temp, &t_temp);
    // st
    BSDF->s = s_temp;
//    vector3 t = cross(n, s);
    BSDF->t = t_temp;
    // ng
    BSDF->ng = isect->ng;
    
    BSDF->offsetsBxDFs[0] = sizeof(BSDFHead);
    uchar* BSDFp = (uchar*)BSDF + sizeof(BSDFHead);
    uchar BxDFID = *(matsData_p++);
    BxDFType FType;
    switch (BxDFID) {
        case 0: {
            *(BSDFp++) = 0;
            break;
        }
        case 1: {// Diffuse Reflection
            *(BSDFp++) = 1;
            
            FType = BxDF_Reflection | BxDF_Diffuse;
            memcpy(AlignPtrAdd(&BSDFp, sizeof(BxDFType)), &FType, sizeof(BxDFType));
            
            //Reflectance
            color R = evaluateColorTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&matsData_p, sizeof(uint)), isect->uv);
            memcpy(AlignPtrAdd(&BSDFp, sizeof(color)), &R, sizeof(color));
            
            //Roughness
            float sigma2 = evaluateFloatTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&matsData_p, sizeof(uint)), isect->uv);
            sigma2 *= sigma2;
            
            float A = 1.0f - (sigma2 / (2.0f * (sigma2 + 0.33f)));
            float B = 0.45f * sigma2 / (sigma2 + 0.09f);
            memcpy(AlignPtrAdd(&BSDFp, sizeof(float)), &A, sizeof(float));
            memcpy(AlignPtrAdd(&BSDFp, sizeof(float)), &B, sizeof(float));
            break;
        }
        case 2: {// Specular Reflection
            *(BSDFp++) = 2;
            
            FType = BxDF_Reflection | BxDF_Specular;
            memcpy(AlignPtrAdd(&BSDFp, sizeof(BxDFType)), &FType, sizeof(BxDFType));
            
            //Reflectance
            color R = evaluateColorTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&matsData_p, sizeof(uint)), isect->uv);
            memcpy(AlignPtrAdd(&BSDFp, sizeof(color)), &R, sizeof(color));
            
            //Fresnel
            uchar fresnelType = *(matsData_p++);
            if (fresnelType == 0) {
                
            }
            else if (fresnelType == 1) {
                
            }
            else {
                
            }
            break;
        }
        case 3: {// Specular Transmission
            *(BSDFp++) = 3;
        }
        case 4: {// New Ward
            *(BSDFp++) = 4;
            
            FType = BxDF_Reflection | BxDF_Glossy;
            memcpy(AlignPtrAdd(&BSDFp, sizeof(BxDFType)), &FType, sizeof(BxDFType));
            
            //Reflectance
            color R = evaluateColorTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&matsData_p, sizeof(uint)), isect->uv);
            memcpy(AlignPtrAdd(&BSDFp, sizeof(color)), &R, sizeof(color));
            
            //Roughness
            float ax = evaluateFloatTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&matsData_p, sizeof(uint)), isect->uv);
            memcpy(AlignPtrAdd(&BSDFp, sizeof(float)), &ax, sizeof(float));
            float ay = evaluateFloatTexture(scene->texturesData + *(global uint*)AlignPtrAddG(&matsData_p, sizeof(uint)), isect->uv);
            memcpy(AlignPtrAdd(&BSDFp, sizeof(float)), &ay, sizeof(float));
            break;
        }
        default: {
            break;
        }
    }
}

static color sample_fx(const BxDFHead* BxDF, const vector3* vout, const BSDFSample* sample,
                       vector3* vin, float* dirPDF) {
    switch (BxDF->id) {
        case 1: {// Diffuse
            *vin = cosineSampleHemisphere(sample->uDir[0], sample->uDir[1]);
            if (vout->z < 0.0f)
                vin->z *= -1;
            *dirPDF = absCosTheta(vin) / M_PI_F;

            return fx(BxDF, vout, vin);
        }
        case 2: {// Specular Reflection
            const SpecularReflection* speR = (const SpecularReflection*)BxDF;
            
            *vin = (vector3)(-vout->x, -vout->y, vout->z);
            *dirPDF = 1.0f;
            
            uchar fresnelType = 0;
            if (fresnelType == 0) {
                return speR->R / absCosTheta(vin);
            }
            else if (fresnelType == 1) {
                
            }
            else {
                
            }
        }
        case 3: {
            break;
        }
        case 4: {
            const Ward* ward = (const Ward*)BxDF;
            
            float quad = 2 * M_PI_F * sample->uDir[1];
            float phi_h = atan2(ward->ay * sin(quad), ward->ax * cos(quad));
            float cosphi_ax = cos(phi_h) / ward->ax;
            float sinphi_ay = sin(phi_h) / ward->ay;
            float theta_h = atan(sqrt(-log(1 - sample->uDir[0]) / (cosphi_ax * cosphi_ax + sinphi_ay * sinphi_ay)));
            if (vout->z < 0)
                theta_h = M_PI_F - theta_h;
            vector3 halfv = (vector3)(sin(theta_h) * cos(phi_h), sin(theta_h) * sin(phi_h), cos(theta_h));
            *vin = 2 * dot(*vout, halfv) * halfv - *vout;
            if (vin->z * vout->z <= 0) {
                *dirPDF = 0.0f;
                return (color)(0.0f, 0.0f, 0.0f);
            }
            
            float hx_ax = halfv.x / ward->ax;
            float hy_ay = halfv.y / ward->ay;
            float dotHN = absCosTheta(&halfv);
            float dotHI = dot(halfv, *vin);
            float numerator = exp(-(hx_ax * hx_ax + hy_ay * hy_ay) / (dotHN * dotHN));
            float commonDenom = 4 * M_PI_F * ward->ax * ward->ay * dotHI * dotHN * dotHN * dotHN;
            
            *dirPDF = numerator / commonDenom;
            return ward->R * (numerator / (commonDenom * dotHI * dotHN));
        }
        case 0:
        default: {
            break;
        }
    }
    return (color)(0.0f, 0.0f, 0.0f);
}

static color fx(const BxDFHead* BxDF, const vector3* vout, const vector3* vin) {
    switch (BxDF->id) {
        case 1: {
            const Diffuse* diffuse = (const Diffuse*)BxDF;
            
            if (diffuse->A == 1.0f) {
                return diffuse->R / M_PI_F;
            }
            else {
                float sinTheta_i = sinTheta(vin);
                float sinTheta_o = sinTheta(vout);
                
                float maxCos = 0.0f;
                if (sinTheta_i > 1e-4f && sinTheta_o > 1e-4f) {
                    float sinPhi_i = sinPhi(vin), cosPhi_i = cosPhi(vin);
                    float sinPhi_o = sinPhi(vout), cosPhi_o = cosPhi(vout);
                    float dcos = cosPhi_i * cosPhi_o + sinPhi_i * sinPhi_o;
                    maxCos = fmax(0.0f, dcos);
                }
                
                float sinAlpha, tanBeta;
                if (absCosTheta(vin) > absCosTheta(vout)) {
                    sinAlpha = sinTheta_o;
                    tanBeta = sinTheta_i / absCosTheta(vin);
                }
                else {
                    sinAlpha = sinTheta_i;
                    tanBeta = sinTheta_o / absCosTheta(vout);
                }
                return diffuse->R / M_PI_F * (diffuse->A + diffuse->B * maxCos * sinAlpha * tanBeta);
            }
        }
        case 2: {
            return (color)(0.0f, 0.0f, 0.0f);
        }
        case 3: {
            return (color)(0.0f, 0.0f, 0.0f);
        }
        case 4: {
            const Ward* ward = (const Ward*)BxDF;
            
            if (vin->z * vout->z <= 0)
                return (color)(0.0f, 0.0f, 0.0f);
            
            vector3 halfv = normalize(*vin + *vout);
            float hx_ax = halfv.x / ward->ax;
            float hy_ay = halfv.y / ward->ay;
            float dotHN = absCosTheta(&halfv);
            
            float dotHI = dot(halfv, *vin);
            return ward->R * exp(-(hx_ax * hx_ax + hy_ay * hy_ay) / (dotHN * dotHN)) /
            (4 * M_PI_F * ward->ax * ward->ay * dotHI * dotHI * dotHN * dotHN * dotHN * dotHN);
        }
        case 0:
        default: {
            break;
        }
    }
    return (color)(0.0f, 0.0f, 0.0f);
}

color sample_fs(const uchar* BSDF, const vector3* vout, const BSDFSample* sample,
                vector3* vin, float* dirPDF, BxDFType* sampledType) {
    const BSDFHead* head = (const BSDFHead*)BSDF;
    const vector3* s = &head->s;
    const vector3* t = &head->t;
    const vector3* n = &head->n;
    const vector3* ng = &head->ng;
    
    vector3 voutLocal = worldToLocal(s, t, n, vout);
    
    float BxDFProb = 1.0f / (float)head->numBxDFs;
    uint which = randUInt(sample->uComponent, head->numBxDFs);
    const BxDFHead* BxDF = (const BxDFHead*)(BSDF + head->offsetsBxDFs[which]);
    
    vector3 vinLocal;
    color ret = sample_fx(BxDF, &voutLocal, sample, &vinLocal, dirPDF);
    *sampledType = BxDF->fxType;
    
    *vin = localToWorld(s, t, n, &vinLocal);
    
    return ret;
}

color fs(const uchar* BSDF, const vector3* vout, const vector3* vin) {
    const BSDFHead* head = (const BSDFHead*)BSDF;
    const vector3* s = &head->s;
    const vector3* t = &head->t;
    const vector3* n = &head->n;
    const vector3* ng = &head->ng;
    vector3 voutLocal = worldToLocal(s, t, n, vout);
    vector3 vinLocal = worldToLocal(s, t, n, vin);
    
//    if (dot(*vin, *ng) * dot(*vout, *ng) > 0) // ignore BTDFs
//        flags = BxDFType(flags & ~BxDF_Transmission);
//    else // ignore BRDFs
//        flags = BxDFType(flags & ~BxDF_Transmission);
    color fs = (color)(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < head->numBxDFs; ++i) {
        ushort idxBxDF = head->offsetsBxDFs[i];
        fs += fx(BSDF + idxBxDF, &voutLocal, &vinLocal);
    }
    return fs;
}

#endif