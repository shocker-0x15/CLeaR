#ifndef device_reflection_cl
#define device_reflection_cl

#include "global.cl"
#include "rng.cl"
#include "texture.cl"
#include "materials.cl"

typedef enum {
    BxDFID_Diffuse = 0,
    BxDFID_SpecularReflection,
    BxDFID_SpecularTransmission,
    BxDFID_NewWard,
    BxDFID_AshikhminS,
    BxDFID_AshikhminD,
} BxDFID;

typedef enum {
    FresnelID_NoOp = 0,
    FresnelID_Conductor,
    FresnelID_Dielectric,
} FresnelID;

typedef enum {
    BxDF_Reflection   = 1 << 0,
    BxDF_Transmission = 1 << 1,
    
    BxDF_Diffuse      = 1 << 2,
    BxDF_Glossy       = 1 << 3,
    BxDF_Specular     = 1 << 4,
    
    BxDF_Non_Singular    = BxDF_Diffuse | BxDF_Glossy,
    BxDF_All_Types        = BxDF_Diffuse | BxDF_Glossy | BxDF_Specular,
    BxDF_All_Reflection   = BxDF_Reflection | BxDF_All_Types,
    BxDF_All_Transmission = BxDF_Transmission | BxDF_All_Types,
    BxDF_All              = BxDF_All_Reflection | BxDF_All_Transmission
} BxDFType;

//12bytes
typedef struct __attribute__((aligned(4))) {
    float uComponent;
    float uDir[2];
} BSDFSample;

//8bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    BxDFType fxType __attribute__((aligned(4)));
} BxDFHead;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BxDFHead head;
    color R __attribute__((aligned(16)));
    float A, B;
} Diffuse;

//1bytes
typedef struct __attribute__((aligned(1))) {
    uchar ftype;
} FresnelHead;

//48bytes
typedef struct __attribute__((aligned(16))) {
    FresnelHead head;
    color eta __attribute__((aligned(16))), k;
} FresnelConductor;

//12bytes
typedef struct __attribute__((aligned(4))) {
    FresnelHead head;
    float etaExt __attribute__((aligned(4))), etaInt;
} FresnelDielectric;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BxDFHead head;
    color R __attribute__((aligned(16)));
    const global uchar* fresnel;
} SpecularReflection;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BxDFHead head;
    color T __attribute__((aligned(16)));
    float etaExt, etaInt;
    const global uchar* fresnel;
} SpecularTransmission;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BxDFHead head;
    color R __attribute__((aligned(16)));
    float ax, ay;
} NewWard;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BxDFHead head;
    color Rs __attribute__((aligned(16)));
    float nu, nv;
} AshikhminS;

//48bytes
typedef struct __attribute__((aligned(16))) {
    BxDFHead head;
    color Rd __attribute__((aligned(16))), Rs;
} AshikhminD;

//80bytes
typedef struct __attribute__((aligned(16))) {
    DDFHead ddfHead;
    uchar numBxDFs __attribute__((aligned(2)));//EDFHeadが2バイトアラインにしないと何故か落ちるので一応合わせておく。
    ushort offsetsBxDFs[4] __attribute__((aligned(2)));
    vector3 n __attribute__((aligned(16))), s, t, ng;
} BSDFHead;

//------------------------

static inline float cosTheta(const vector3* v);
static inline float absCosTheta(const vector3* v);
static inline float sinTheta2(const vector3* v);
static inline float sinTheta(const vector3* v);
static float cosPhi(const vector3* v);
static float sinPhi(const vector3* v);

static inline vector3 halfvec(const vector3* v0, const vector3* v1);

static bool matchType(const BxDFHead* BxDF, BxDFType mask);
bool hasNonSpecular(const uchar* BSDF);

void BSDFAlloc(const Scene* scene, uint offset, const Intersection* isect, uchar* BSDF);

static color evaluateFresnel(const global uchar* fresnel, float cosi);

static color sample_fx(const BxDFHead* BSDF, const vector3* vout, const BSDFSample* sample,
                       vector3* vin, float* dirPDF);
static color fx(const BxDFHead* BxDF, const vector3* vout, const vector3* vin);
static float fx_pdf(const BxDFHead* BxDF, const vector3* vout, const vector3* vin);

color sample_fs(const uchar* BSDF, const vector3* vout, const BSDFSample* sample,
                vector3* vin, float* dirPDF, BxDFType* sampledType);
color fs(const uchar* BSDF, const vector3* vout, const vector3* vin);
float fs_pdf(const uchar* BSDF, const vector3* vout, const vector3* vin);
inline float absCosNsBSDF(const uchar* BSDF, const vector3* v);

//------------------------

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

static float cosPhi(const vector3* v) {
    float sinT = sinTheta(v);
    if (sinT == 0.0f) return 1.0f;
    return clamp(v->x / sinT, -1.0f, 1.0f);
}

static float sinPhi(const vector3* v) {
    float sinT = sinTheta(v);
    if (sinT == 0.0f) return 0.0f;
    return clamp(v->y / sinT, -1.0f, 1.0f);
}


static inline vector3 halfvec(const vector3* v0, const vector3* v1) {
    return normalize(*v0 + *v1);
}


static bool matchType(const BxDFHead* BxDF, BxDFType mask) {
    return (BxDF->fxType & mask) == BxDF->fxType;
}

bool hasNonSpecular(const uchar* BSDF) {
    const BSDFHead* fsHead = (const BSDFHead*)BSDF;
    for (int i = 0; i < fsHead->numBxDFs; ++i) {
        const BxDFHead* bxdf = (const BxDFHead*)(BSDF + fsHead->offsetsBxDFs[i]);
        if((bool)(bxdf->fxType & BxDF_Non_Singular))
            return true;
    }
    return false;
}


void BSDFAlloc(const Scene* scene, uint offset, const Intersection* isect, uchar* BSDF) {
    BSDFHead* fsHead = (BSDFHead*)BSDF;
    fsHead->ddfHead._type = DDFType_BSDF;
    const global uchar* matsData_p = scene->materialsData + offset;
    const global MaterialInfo* matInfo = (const global MaterialInfo*)matsData_p;
    
    fsHead->numBxDFs = matInfo->numBxDFs;
    fsHead->offsetsBxDFs[0] = fsHead->offsetsBxDFs[1] =
    fsHead->offsetsBxDFs[2] = fsHead->offsetsBxDFs[3] = 0;
    
    fsHead->n = isect->sNormal;
    if (isect->hasTangent)
        fsHead->s = isect->sTangent;
    else
        makeTangent(&isect->sNormal, &fsHead->s);
    
    if ((bool)matInfo->hasBump) {
        vector3 nBump = normalize(2.0f * evaluateColorTexture(scene->texturesData + matInfo->idx_bump, isect->uv) - 1.0f);
        vector3 vDir = cross(isect->uDir, isect->sNormal);
        fsHead->n = (vector3)(dot((vector3)(isect->uDir.x, vDir.x, isect->sNormal.x), nBump),
                              dot((vector3)(isect->uDir.y, vDir.y, isect->sNormal.y), nBump),
                              dot((vector3)(isect->uDir.z, vDir.z, isect->sNormal.z), nBump));
        vector3 ax = cross(isect->sNormal, fsHead->n);
        float sinTH = fmin(length(ax), 1.0f);
        if (sinTH > 0.0001f) {
            ax = normalize(ax);
            float cosTH = cos(asin(sinTH));
            float oneMcosTH = 1 - cosTH;
            fsHead->s = (vector3)(dot((vector3)(ax.x * ax.x * oneMcosTH + cosTH,
                                                ax.x * ax.y * oneMcosTH - ax.z * sinTH,
                                                ax.z * ax.x * oneMcosTH + ax.y * sinTH), fsHead->s),
                                  dot((vector3)(ax.x * ax.y * oneMcosTH + ax.z * sinTH,
                                                ax.y * ax.y * oneMcosTH + cosTH,
                                                ax.y * ax.z * oneMcosTH - ax.x * sinTH), fsHead->s),
                                  dot((vector3)(ax.z * ax.x * oneMcosTH - ax.y * sinTH,
                                                ax.y * ax.z * oneMcosTH + ax.x * sinTH,
                                                ax.z * ax.z * oneMcosTH + cosTH), fsHead->s));
        }
    }
    
    fsHead->t = cross(fsHead->n, fsHead->s);
    fsHead->ng = isect->gNormal;
    
    uchar* BSDFp = BSDF + sizeof(BSDFHead);
    matsData_p += sizeof(MaterialInfo);
    for (int i = 0; i < fsHead->numBxDFs; ++i) {
        AlignPtrG(&matsData_p, 4);
        uchar BxDFID = *matsData_p;
        switch (BxDFID) {
            case BxDFID_Diffuse: {
                AlignPtr(&BSDFp, 16);
                fsHead->offsetsBxDFs[i] = (ushort)((uintptr_t)BSDFp - (uintptr_t)BSDF);
                
                Diffuse* diffuse = (Diffuse*)BSDFp;
                const global DiffuseBRDFInfo* diffuseInfo = (const global DiffuseBRDFInfo*)matsData_p;
                
                diffuse->head.id = BxDFID;
                diffuse->head.fxType = (BxDFType)(BxDF_Reflection | BxDF_Diffuse);
                diffuse->R = evaluateColorTexture(scene->texturesData + diffuseInfo->idx_R, isect->uv);
                float sigma2 = evaluateFloatTexture(scene->texturesData + diffuseInfo->idx_sigma, isect->uv);
                sigma2 *= sigma2;
                diffuse->A = 1.0f - (sigma2 / (2.0f * (sigma2 + 0.33f)));
                diffuse->B = 0.45f * sigma2 / (sigma2 + 0.09f);
                
                BSDFp += sizeof(Diffuse);
                matsData_p += sizeof(DiffuseBRDFInfo);
                break;
            }
            case BxDFID_SpecularReflection: {
                AlignPtr(&BSDFp, 16);
                fsHead->offsetsBxDFs[i] = (ushort)((uintptr_t)BSDFp - (uintptr_t)BSDF);
                
                SpecularReflection* speR = (SpecularReflection*)BSDFp;
                const global SpecularBRDFInfo* speRInfo = (const global SpecularBRDFInfo*)matsData_p;
                
                speR->head.id = BxDFID;
                speR->head.fxType = (BxDFType)(BxDF_Reflection | BxDF_Specular);
                speR->R = evaluateColorTexture(scene->texturesData + speRInfo->idx_R, isect->uv);
                speR->fresnel = scene->texturesData + speRInfo->idx_Fresnel;
                
                BSDFp += sizeof(SpecularReflection);
                matsData_p += sizeof(SpecularBRDFInfo);
                break;
            }
            case BxDFID_SpecularTransmission: {
                AlignPtr(&BSDFp, 16);
                fsHead->offsetsBxDFs[i] = (ushort)((uintptr_t)BSDFp - (uintptr_t)BSDF);
                
                SpecularTransmission* speT = (SpecularTransmission*)BSDFp;
                const global SpecularBTDFInfo* speTInfo = (const global SpecularBTDFInfo*)matsData_p;
                
                speT->head.id = BxDFID;
                speT->head.fxType = (BxDFType)(BxDF_Transmission | BxDF_Specular);
                speT->T = evaluateColorTexture(scene->texturesData + speTInfo->idx_T, isect->uv);
                speT->etaExt = speTInfo->etaExt;
                speT->etaInt = speTInfo->etaInt;
                speT->fresnel = scene->texturesData + speTInfo->idx_Fresnel;
                
                BSDFp += sizeof(SpecularTransmission);
                matsData_p += sizeof(SpecularBTDFInfo);
                break;
            }
            case BxDFID_NewWard: {
                AlignPtr(&BSDFp, 16);
                fsHead->offsetsBxDFs[i] = (ushort)((uintptr_t)BSDFp - (uintptr_t)BSDF);
                
                NewWard* ward = (NewWard*)BSDFp;
                const global NewWardBRDFInfo* wardInfo = (const global NewWardBRDFInfo*)matsData_p;
                
                ward->head.id = BxDFID;
                ward->head.fxType = (BxDFType)(BxDF_Reflection | BxDF_Glossy);
                ward->R = evaluateColorTexture(scene->texturesData + wardInfo->idx_R, isect->uv);
                ward->ax = evaluateFloatTexture(scene->texturesData + wardInfo->idx_anisoX, isect->uv);
                ward->ay = evaluateFloatTexture(scene->texturesData + wardInfo->idx_anisoY, isect->uv);
                
                BSDFp += sizeof(NewWard);
                matsData_p += sizeof(NewWardBRDFInfo);
                break;
            }
            case BxDFID_AshikhminS: {
                AlignPtr(&BSDFp, 16);
                fsHead->offsetsBxDFs[i] = (ushort)((uintptr_t)BSDFp - (uintptr_t)BSDF);
                
                AshikhminS* ashS = (AshikhminS*)BSDFp;
                const global AshikhminSBRDFInfo* ashSInfo = (const global AshikhminSBRDFInfo*)matsData_p;
                
                ashS->head.id = BxDFID;
                ashS->head.fxType = (BxDFType)(BxDF_Reflection | BxDF_Glossy);
                ashS->Rs = evaluateColorTexture(scene->texturesData + ashSInfo->idx_Rs, isect->uv);
                ashS->nu = evaluateFloatTexture(scene->texturesData + ashSInfo->idx_nu, isect->uv);
                ashS->nv = evaluateFloatTexture(scene->texturesData + ashSInfo->idx_nv, isect->uv);
                
                BSDFp += sizeof(AshikhminS);
                matsData_p += sizeof(AshikhminSBRDFInfo);
                break;
            }
            case BxDFID_AshikhminD: {
                AlignPtr(&BSDFp, 16);
                fsHead->offsetsBxDFs[i] = (ushort)((uintptr_t)BSDFp - (uintptr_t)BSDF);
                
                AshikhminD* ashD = (AshikhminD*)BSDFp;
                const global AshikhminDBRDFInfo* ashDInfo = (const global AshikhminDBRDFInfo*)matsData_p;
                
                ashD->head.id = BxDFID;
                ashD->head.fxType = (BxDFType)(BxDF_Reflection | BxDF_Diffuse);
                ashD->Rd = evaluateColorTexture(scene->texturesData + ashDInfo->idx_Rd, isect->uv);
                ashD->Rs = evaluateColorTexture(scene->texturesData + ashDInfo->idx_Rs, isect->uv);
                
                BSDFp += sizeof(AshikhminD);
                matsData_p += sizeof(AshikhminDBRDFInfo);
                break;
            }
            default: {
                break;
            }
        }
    }
}


static color evaluateFresnel(const global uchar* fresnel, float cosi) {
    const global FresnelHead* head = (const global FresnelHead*)fresnel;
    switch (head->ftype) {
        case FresnelID_NoOp:
            return colorOne;
        case FresnelID_Conductor: {
            const global FresnelConductor* frCond = (const global FresnelConductor*)fresnel;
            color eta = frCond->eta;
            color k = frCond->k;
            cosi = fabs(cosi);
            
            color tmp = (eta * eta + k * k) * cosi * cosi;
            color Rparl2 = (tmp - (2.0f * eta * cosi) + 1) / (tmp + (2.0f * eta * cosi) + 1);
            color tmp_f = eta * eta + k * k;
            color Rperp2 = (tmp_f - (2.0f * eta * cosi) + cosi*cosi) / (tmp_f + (2.0f * eta * cosi) + cosi * cosi);
            return (Rparl2 + Rperp2) / 2.0f;
        }
        case FresnelID_Dielectric: {
            const global FresnelDielectric* frDiel = (const global FresnelDielectric*)fresnel;
            cosi = clamp(cosi, -1.0f, 1.0f);
            
            bool entering = cosi > 0.0f;
            float ei = frDiel->etaExt;
            float et = frDiel->etaInt;
            if (!entering) {
                ei = frDiel->etaInt;
                et = frDiel->etaExt;
            }
            
            float sint = ei / et * sqrt(fmax(0.0f, 1.0f - cosi * cosi));
            if (sint >= 1.0f) {
                return colorOne;
            }
            else {
                float cost = sqrt(fmax(0.0f, 1.0f - sint * sint));
                cosi = fabs(cosi);
                
                float Rparl = ((et * cosi) - (ei * cost)) / ((et * cosi) + (ei * cost));
                float Rperp = ((ei * cosi) - (et * cost)) / ((ei * cosi) + (et * cost));
                return (color)(Rparl * Rparl + Rperp * Rperp) / 2.0f;
            }
        }
        default:
            return colorZero;
    }
}


static color sample_fx(const BxDFHead* BxDF, const vector3* vout, const BSDFSample* sample,
                       vector3* vin, float* dirPDF) {
    switch (BxDF->id) {
        case BxDFID_Diffuse: {
            *vin = cosineSampleHemisphere(sample->uDir[0], sample->uDir[1]);
            if (vout->z < 0.0f)
                vin->z *= -1;
            *dirPDF = absCosTheta(vin) / M_PI_F;
            
            return fx(BxDF, vout, vin);
        }
        case BxDFID_SpecularReflection: {
            const SpecularReflection* speR = (const SpecularReflection*)BxDF;
            
            *vin = (vector3)(-vout->x, -vout->y, vout->z);
            *dirPDF = 1.0f;
            
            return speR->R * evaluateFresnel(speR->fresnel, cosTheta(vout)) / absCosTheta(vin);
        }
        case BxDFID_SpecularTransmission: {
            const SpecularTransmission* speT = (const SpecularTransmission*)BxDF;
            
            bool entering = cosTheta(vout) > 0.0f;
            float ei = speT->etaExt;
            float et = speT->etaInt;
            if (!entering) {
                ei = speT->etaInt;
                et = speT->etaExt;
            }
            
            float sini2 = sinTheta2(vout);
            float eta = ei / et;
            float sint2 = eta * eta * sini2;
            
            if (sint2 >= 1.0f)
                return colorZero;
            float cost = sqrt(fmax(0.0f, 1.0f - sint2));
            if (entering)
                cost = -cost;
            float sintOverSini = eta;
            *vin = (vector3)(sintOverSini * -vout->x, sintOverSini * -vout->y, cost);
            *dirPDF = 1.0f;
            
            color f = evaluateFresnel(speT->fresnel, cosTheta(vout));
            return (colorOne - f) * speT->T / absCosTheta(vin);
        }
        case BxDFID_NewWard: {
            const NewWard* ward = (const NewWard*)BxDF;
            
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
                return colorZero;
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
        case BxDFID_AshikhminS: {
            const AshikhminS* ashS = (const AshikhminS*)BxDF;
            
            float quad = 2 * M_PI_F * sample->uDir[1];
            float phi_h = atan2(sqrt(ashS->nu + 1) * sin(quad), sqrt(ashS->nv + 1) * cos(quad));
            float cosphi = cos(phi_h);
            float sinphi = sin(phi_h);
            float theta_h = acos(pow(1 - sample->uDir[0], 1.0f / (ashS->nu * cosphi * cosphi + ashS->nv * sinphi * sinphi + 1)));
            if (vin->z < 0)
                theta_h = M_PI_F - theta_h;
            vector3 halfv = (vector3)(sin(theta_h) * cos(phi_h), sin(theta_h) * sin(phi_h), cos(theta_h));
            *vin = 2 * dot(*vout, halfv) * halfv - *vout;
            if (vout->z * vin->z <= 0) {
                *dirPDF = 0.0f;
                return colorZero;
            }
            *dirPDF = fx_pdf(BxDF, vout, vin);
            return fx(BxDF, vout, vin);
        }
        case BxDFID_AshikhminD: {
            *vin = cosineSampleHemisphere(sample->uDir[0], sample->uDir[1]);
            if (vout->z < 0.0f)
                vin->z *= -1;
            *dirPDF = absCosTheta(vin) / M_PI_F;
            
            return fx(BxDF, vout, vin);
        }
        default: {
            break;
        }
    }
    return colorZero;
}

static color fx(const BxDFHead* BxDF, const vector3* vout, const vector3* vin) {
    switch (BxDF->id) {
        case BxDFID_Diffuse: {
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
        case BxDFID_SpecularReflection: {
            return colorZero;
        }
        case BxDFID_SpecularTransmission: {
            return colorZero;
        }
        case BxDFID_NewWard: {
            if (vin->z * vout->z <= 0.0f)
                return colorZero;
            
            const NewWard* ward = (const NewWard*)BxDF;
            
            vector3 halfv = halfvec(vout, vin);
            float hx_ax = halfv.x / ward->ax;
            float hy_ay = halfv.y / ward->ay;
            float dotHN = absCosTheta(&halfv);
            
            float dotHI = dot(halfv, *vin);
            return ward->R * exp(-(hx_ax * hx_ax + hy_ay * hy_ay) / (dotHN * dotHN)) /
            (4 * M_PI_F * ward->ax * ward->ay * dotHI * dotHI * dotHN * dotHN * dotHN * dotHN);
        }
        case BxDFID_AshikhminS: {
            if (vin->z * vout->z <= 0.0f)
                return colorZero;
            
            const AshikhminS* ashS = (const AshikhminS*)BxDF;
            
            vector3 halfv = halfvec(vout, vin);
            float dotHV = dot(halfv, *vout);
            float exp = (ashS->nu * halfv.x * halfv.x + ashS->nv * halfv.y * halfv.y) / (1 - halfv.z * halfv.z);
            color fr = ashS->Rs + (colorOne - ashS->Rs) * pow(1.0f - dotHV, 5);
            return sqrt((ashS->nu + 1) * (ashS->nv + 1)) / (8 * M_PI_F) *
            pow(fabs(halfv.z), exp) / (dotHV * fmax(absCosTheta(vout), absCosTheta(vin))) * fr;
        }
        case BxDFID_AshikhminD: {
            if (vin->z * vout->z <= 0.0f)
                return colorZero;
            
            const AshikhminD* ashD = (const AshikhminD*)BxDF;
            
            return 28 * ashD->Rd / (23 * M_PI_F) * (colorOne - ashD->Rs) *
            (1.0f - pow(1.0f - absCosTheta(vout) / 2, 5)) * (1.0f - pow(1.0f - absCosTheta(vin) / 2, 5));
        }
        default: {
            break;
        }
    }
    return colorZero;
}

static float fx_pdf(const BxDFHead* BxDF, const vector3* vout, const vector3* vin) {
    switch (BxDF->id) {
        case BxDFID_Diffuse:
            return absCosTheta(vin) / M_PI_F;
        case BxDFID_SpecularReflection:
            return 0.0f;
        case BxDFID_SpecularTransmission:
            return 0.0f;
        case BxDFID_NewWard: {
            if (vin->z * vout->z <= 0)
                return 0.0f;
            
            const NewWard* ward = (const NewWard*)BxDF;
            
            vector3 halfv = halfvec(vout, vin);
            float hx_ax = halfv.x / ward->ax;
            float hy_ay = halfv.y / ward->ay;
            float dotHN = absCosTheta(&halfv);
            float dotHI = dot(halfv, *vin);
            float numerator = exp(-(hx_ax * hx_ax + hy_ay * hy_ay) / (dotHN * dotHN));
            float commonDenom = 4 * M_PI_F * ward->ax * ward->ay * dotHI * dotHN * dotHN * dotHN;
            
            return numerator / commonDenom;
        }
        case BxDFID_AshikhminS: {
            if (vout->z * vin->z <= 0)
                return 0.0f;
            
            const AshikhminS* ashS = (const AshikhminS*)BxDF;
            
            vector3 halfv = halfvec(vout, vin);
            float exp = (ashS->nu * halfv.x * halfv.x + ashS->nv * halfv.y * halfv.y) / (1 - halfv.z * halfv.z);
            return sqrt((ashS->nu + 1) * (ashS->nv + 1)) / (2 * M_PI_F) * pow(fabs(halfv.z), exp) / (4 * dot(*vout, halfv));
        }
        case BxDFID_AshikhminD:
            return absCosTheta(vin) / M_PI_F;
        default: {
            break;
        }
    }
    return 0.0f;
}


color sample_fs(const uchar* BSDF, const vector3* vout, const BSDFSample* sample,
                vector3* vin, float* dirPDF, BxDFType* sampledType) {
    const BSDFHead* head = (const BSDFHead*)BSDF;
    BxDFType flags = BxDF_All;
    uint numMatches = head->numBxDFs;
    const vector3* s = &head->s;
    const vector3* t = &head->t;
    const vector3* n = &head->n;
    const vector3* ng = &head->ng;
    
    vector3 voutLocal = worldToLocal(s, t, n, vout);
    
    uint which = randUInt(sample->uComponent, head->numBxDFs);
    const BxDFHead* BxDF = (const BxDFHead*)(BSDF + head->offsetsBxDFs[which]);
    
    vector3 vinLocal;
    color ret = sample_fx(BxDF, &voutLocal, sample, &vinLocal, dirPDF);
    if (*dirPDF == 0.0f) {
        *sampledType = BxDFType(0);
        return colorZero;
    }
    *sampledType = BxDF->fxType;
    
    *vin = localToWorld(s, t, n, &vinLocal);
    
    if (!(BxDF->fxType & BxDF_Specular) && numMatches > 1) {
        for (uint i = 0; i < head->numBxDFs; ++i) {
            const BxDFHead* ifx = (const BxDFHead*)(BSDF + head->offsetsBxDFs[i]);
            if (i != which && matchType(ifx, flags))
                *dirPDF += fx_pdf(ifx, &voutLocal, &vinLocal);
        }
    }
    *dirPDF /= (float)numMatches;
    
    if (!(BxDF->fxType & BxDF_Specular)) {
        ret = colorZero;
        if (dot(*vin, *ng) * dot(*vout, *ng) > 0)
            flags = (BxDFType)(flags & ~BxDF_Transmission);
        else
            flags = (BxDFType)(flags & ~BxDF_Reflection);
        for (uint i = 0; i < head->numBxDFs; ++i) {
            const BxDFHead* ifx = (const BxDFHead*)(BSDF + head->offsetsBxDFs[i]);
            if (matchType(ifx, flags))
                ret += fx(ifx, &voutLocal, &vinLocal);
        }
    }
    
    return ret;
}

color fs(const uchar* BSDF, const vector3* vout, const vector3* vin) {
    const BSDFHead* head = (const BSDFHead*)BSDF;
    BxDFType flags = BxDF_All;
    uint numMatches = head->numBxDFs;
    const vector3* s = &head->s;
    const vector3* t = &head->t;
    const vector3* n = &head->n;
    const vector3* ng = &head->ng;
    vector3 voutLocal = worldToLocal(s, t, n, vout);
    vector3 vinLocal = worldToLocal(s, t, n, vin);
    
    color ret = colorZero;
    if (dot(*vin, *ng) * dot(*vout, *ng) > 0)
        flags = (BxDFType)(flags & ~BxDF_Transmission);
    else
        flags = (BxDFType)(flags & ~BxDF_Reflection);
    for (uint i = 0; i < head->numBxDFs; ++i) {
        const BxDFHead* ifx = (const BxDFHead*)(BSDF + head->offsetsBxDFs[i]);
        if (matchType(ifx, flags))
            ret += fx(ifx, &voutLocal, &vinLocal);
    }
    
    return ret;
}

float fs_pdf(const uchar* BSDF, const vector3* vout, const vector3* vin) {
    const BSDFHead* head = (const BSDFHead*)BSDF;
    BxDFType flags = BxDF_All;
    uint numMatches = head->numBxDFs;
    const vector3* s = &head->s;
    const vector3* t = &head->t;
    const vector3* n = &head->n;
    const vector3* ng = &head->ng;
    vector3 voutLocal = worldToLocal(s, t, n, vout);
    vector3 vinLocal = worldToLocal(s, t, n, vin);
    
    float dirPDF = 0.0f;
    for (uint i = 0; i < head->numBxDFs; ++i) {
        const BxDFHead* ifx = (const BxDFHead*)(BSDF + head->offsetsBxDFs[i]);
        if (matchType(ifx, flags))
            dirPDF += fx_pdf(ifx, &voutLocal, &vinLocal);
    }
    
    return dirPDF / numMatches;
}

inline float absCosNsBSDF(const uchar* BSDF, const vector3* v) {
    return fabs(dot(((const BSDFHead*)BSDF)->n, *v));
}

#endif
