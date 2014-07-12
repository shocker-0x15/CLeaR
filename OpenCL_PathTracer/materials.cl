#ifndef device_materials_cl
#define device_materials_cl

#include "global.cl"

typedef enum {
    MatElem_Diffuse = 0,
    MatElem_SpecularReflection,
    MatElem_SpecularTransmission,
    MatElem_NewWard,
    MatElem_AshikhminS,
    MatElem_AshikhminD,
} MatElem;

typedef enum {
    LPElem_DiffuseEmission = 0,
} LPElem;

typedef enum {
    EnvLPElem_ImageBased = 0,
} EnvLPElem;


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
} DiffuseRElem;

//12bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_R __attribute__((aligned(4)));
    uint idx_Fresnel;
} SpecularRElem;

//20bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_T __attribute__((aligned(4)));
    float etaExt;
    float etaInt;
    uint idx_Fresnel;
} SpecularTElem;

//16bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_R __attribute__((aligned(4)));
    uint idx_anisoX;
    uint idx_anisoY;
} NewWardElem;

//16bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_Rs __attribute__((aligned(4)));
    uint idx_nu;
    uint idx_nv;
} AshikhminSElem;

//12bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_Rs __attribute__((aligned(4)));
    uint idx_Rd;
} AshikhminDElem;


//1bytes
typedef struct __attribute__((aligned(1))) {
    uchar numEEDFs;
} LightPropertyInfo;

//8bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_M __attribute__((aligned(4)));
} DiffuseLElem;

//12bytes
typedef struct __attribute__((aligned(4))) {
    uchar id;
    uint idx_Le __attribute__((aligned(4)));
    uint idx_Dist2D;
} ImageBasedEnvLElem;

#endif
