#ifndef device_materials_cl
#define device_materials_cl

#include "global.cl"

//8bytes
typedef struct __attribute__((aligned(4))) {
    uchar numBxDFs;
    uchar hasBump;
    uint idx_bump __attribute__((aligned(4)));
} MaterialInfo;

//12bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_R __attribute__((aligned(4)));
    uint idx_sigma;
} DiffuseBRDFInfo;

//12bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_R __attribute__((aligned(4)));
    uint idx_Fresnel;
} SpecularBRDFInfo;

//20bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_T __attribute__((aligned(4)));
    float etaExt;
    float etaInt;
    uint idx_Fresnel;
} SpecularBTDFInfo;

//16bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_R __attribute__((aligned(4)));
    uint idx_anisoX;
    uint idx_anisoY;
} NewWardBRDFInfo;

//16bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_Rs __attribute__((aligned(4)));
    uint idx_nu;
    uint idx_nv;
} AshikhminSBRDFInfo;

//12bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_Rs __attribute__((aligned(4)));
    uint idx_Rd;
} AshikhminDBRDFInfo;


//1bytes
typedef struct __attribute__((aligned(1))) {
    uchar numEEDFs;
} LightPropertyInfo;

//8bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_M __attribute__((aligned(4)));
} DiffuseEDFInfo;

#endif
