#ifndef sim_materials_cl
#define sim_materials_cl

#include "sim_global.hpp"

namespace sim {
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
    } DiffuseBRDFInfo;
    
    //12bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_R;
        uint idx_Fresnel;
    } SpecularBRDFInfo;
    
    //20bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_T;
        float etaExt;
        float etaInt;
        uint idx_Fresnel;
    } SpecularBTDFInfo;
    
    //16bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_R;
        uint idx_anisoX;
        uint idx_anisoY;
    } NewWardBRDFInfo;
    
    //16bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_Rs;
        uint idx_nu;
        uint idx_nv;
    } AshikhminSBRDFInfo;
    
    //12bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_Rs;
        uint idx_Rd;
    } AshikhminDBRDFInfo;
    
    
    //1bytes
    typedef struct {
        uchar numEEDFs;
    } LightPropertyInfo;
    
    //8bytes
    typedef struct {
        uchar id; uchar dum0[3];
        uint idx_M;
    } DiffuseEDFInfo;
}

#endif