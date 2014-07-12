#ifndef sim_materials_cl
#define sim_materials_cl

#include "sim_global.hpp"

namespace sim {
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
    typedef struct {
        uchar numBxDFs;
        uchar hasBump; uchar dum0[2];
        uint idx_bump;
    } MaterialInfo;
    
    //12bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_R;
        uint idx_sigma;
    } DiffuseRElem;
    
    //12bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_R;
        uint idx_Fresnel;
    } SpecularRElem;
    
    //20bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_T;
        float etaExt;
        float etaInt;
        uint idx_Fresnel;
    } SpecularTElem;
    
    //16bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_R;
        uint idx_anisoX;
        uint idx_anisoY;
    } NewWardElem;
    
    //16bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_Rs;
        uint idx_nu;
        uint idx_nv;
    } AshikhminSElem;
    
    //12bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_Rs;
        uint idx_Rd;
    } AshikhminDElem;
    
    
    //1bytes
    typedef struct {
        uchar numEEDFs;
    } LightPropertyInfo;
    
    //8bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_M;
    } DiffuseLElem;
    
    //12bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_Le;
        uint idx_Dist2D;
    } ImageBasedEnvLElem;
}

#endif
