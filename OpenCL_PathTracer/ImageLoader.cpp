//
//  ImageLoader.cpp
//  OpenCL_TEST
//
//  Created by 渡部 心 on 2014/05/05.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "ImageLoader.hpp"
#include <libpng16/png.h>
#include <ImfInputFile.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <cstdlib>
#include <string>
#include <cassert>

enum EXRType {
    Plane = 0,
    LatitudeLongitude,
    CubeMap
};

bool loadEXR(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height, EXRType* exrtype) {
    using namespace Imf;
    using namespace Imath;
    RgbaInputFile file(fileName);
    Imf_2_1::Header header = file.header();
    
    Box2i dw = file.dataWindow();
    *width = dw.max.x - dw.min.x + 1;
    *height = dw.max.y - dw.min.y + 1;
    Array2D<Rgba> pixels{*height, *width};
    pixels.resizeErase(*height, *width);
    file.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * *width, 1, *width);
    file.readPixels(dw.min.y, dw.max.y);
    
    size_t curSize = storage->size();
    size_t rowSize = *width * sizeof(Rgba);
    storage->resize(storage->size() + *height * rowSize);
    uint8_t* orgDataHead = &(*storage)[curSize];
    uint8_t* curDataHead = orgDataHead;
    for (int i = 0; i < *height; ++i) {
        memcpy(curDataHead, pixels[i], rowSize);
        curDataHead += rowSize;
    }
    
    return true;
}

bool loadJPEG(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height) {
    return false;
}

int libPNGreadChunkCallback(png_structp pngStruct, png_unknown_chunkp chunk) {
    return 0;
}

void libPNGreadRowCallback(png_structp pngStruct, png_uint_32 row, int pass) {
    
}

bool loadPNG(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height, ColorChannel::Value* color, bool gammaCorrection) {
    FILE* fp = fopen(fileName, "rb");
    if (fp == nullptr) {
        printf("Failed to open the png file.\n");
        return false;
    }
    
    uint8_t headSignature[8];
    fread(headSignature, 1, sizeof(headSignature), fp);
    if (png_sig_cmp(headSignature, 0, sizeof(headSignature))) {
        fclose(fp);
        printf("This is not a png file.\n");
        return false;
    }
    
    png_structp pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!pngStruct) {
        fclose(fp);
        printf("libpng: Failed to create a read struct.\n");
        return false;
    }
    
    //画像データ前のチャンク用の構造体を確保。
    png_infop pngInfo = png_create_info_struct(pngStruct);
    if (!pngInfo) {
        png_destroy_read_struct(&pngStruct, nullptr, nullptr);
        fclose(fp);
        printf("libpng: Failed to create an info struct.\n");
        return false;
    }
    
    //画像データ後のチャンク用の構造体を確保。
    png_infop pngEndInfo = png_create_info_struct(pngStruct);
    if (!pngEndInfo) {
        png_destroy_read_struct(&pngStruct, &pngInfo, nullptr);
        fclose(fp);
        printf("libpng: Failed to create an end info struct.\n");
        return false;
    }
    
    if (setjmp(png_jmpbuf(pngStruct))) {
        png_destroy_read_struct(&pngStruct, &pngInfo, &pngEndInfo);
        fclose(fp);
        printf("libpng: Error.\n");
        return false;
    }
    
    png_init_io(pngStruct, fp);
    png_set_sig_bytes(pngStruct, sizeof(headSignature));
    png_set_read_user_chunk_fn(pngStruct, png_get_user_chunk_ptr(pngStruct), libPNGreadChunkCallback);
//    png_set_read_status_fn(pngStruct, libPNGreadRowCallback);
#ifdef PNG_UNKNOWN_CHUNKS_SUPPORTED
    png_byte chunkList[0] = {};
    png_set_keep_unknown_chunks(pngStruct, PNG_HANDLE_CHUNK_AS_DEFAULT, chunkList, 0);
#endif
    png_set_user_limits(pngStruct, 5120, 5120);//5120x5120より大きい画像は却下する。
//    png_set_chunk_cache_max(pngStruct, 0x7FFFFFFF);//補助チャンクsPLT, tEXt, iTXt, zTXtの限界数を設定する。
    
    png_read_info(pngStruct, pngInfo);
    
    int bitDepth, colorType, interlaceMethod, compressionMethod, filterMethod;
    png_get_IHDR(pngStruct, pngInfo, width, height, &bitDepth, &colorType, &interlaceMethod, &compressionMethod, &filterMethod);
    assert(colorType != PNG_COLOR_TYPE_PALETTE && colorType != PNG_COLOR_TYPE_GA);
    if (colorType == PNG_COLOR_TYPE_RGB)
        *color = ColorChannel::RGB888;
    else if (colorType == PNG_COLOR_TYPE_RGBA)
        *color = ColorChannel::RGBA8888;
    else if (colorType == PNG_COLOR_TYPE_GRAY)
        *color = ColorChannel::Gray8;
    size_t channels = png_get_channels(pngStruct, pngInfo);
    size_t rowBytes = png_get_rowbytes(pngStruct, pngInfo);
    
    if (bitDepth == 16)
        png_set_strip_16(pngStruct);//16ビットの深度を8ビットに変換する。
//    png_set_invert_alpha(pngStruct);//アルファを透明度とする。
    if (bitDepth < 8)
        png_set_packing(pngStruct);//1, 2, 4ビットを1バイトに詰めずに読み込みたい場合。関数名とは逆の印象。
    png_color_8p significantBit;
    if (png_get_sBIT(pngStruct, pngInfo, &significantBit))
        png_set_shift(pngStruct, significantBit);//本来のビット深度が2のべき乗ではない場合に、本来の深度へと戻す。
//    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGB_ALPHA)
//        png_set_bgr(pngStruct);//RGBをBGRに変換する。
    if (colorType == PNG_COLOR_TYPE_RGB)
        png_set_filler(pngStruct, 0xFFFF, PNG_FILLER_AFTER);//3チャンネルの場合に1バイト0xFFを後ろ側に埋める。カラータイプは変わらない。
//    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY)
//        png_set_add_alpha(pngStruct, 0xFFFF, PNG_FILLER_AFTER);//アルファチャンネルを足す。
//    if (colorType == PNG_COLOR_TYPE_RGB_ALPHA)
//        png_set_swap_alpha(pngStruct);//RGBAのファイルをARGBで読み込みたい場合。
//    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
//        png_set_gray_to_rgb(pngStruct);//グレイスケール画像をRGBで表現させて読み込みたい場合。
//    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGB_ALPHA)
//        png_set_rgb_to_gray_fixed(pngStruct, 1, 21268, 71510);RGB画像をグレイスケールで読み込みたい場合。RとGのウェイトx100000を指定する。
//    png_color_16 myBackground;
//    png_color_16p imageBackground;
//    if (png_get_bKGD(pngStruct, pngInfo, &imageBackground))
//        png_set_background(pngStruct, imageBackground, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
//    else
//        png_set_background(pngStruct, &myBackground, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
    double gamma, screenGamma;
    const char* gammaStr;
    if (gammaCorrection) {
        if ((gammaStr = getenv("SCREEN_GAMMA")) != nullptr)
            screenGamma = (double)atof(gammaStr);
        else
            screenGamma = 2.2;
    }
    else{
        screenGamma = 1.0f;
    }
    if (png_get_gAMA(pngStruct, pngInfo, &gamma))
        png_set_gamma(pngStruct, screenGamma, gamma);//ファイルにガンマ値がある場合。
    else
        png_set_gamma(pngStruct, screenGamma, 0.45455);//ファイルにガンマ値が無い場合。
//    if (bitDepth == 1 && colorType == PNG_COLOR_TYPE_GRAY)
//        png_set_invert_mono(pngStruct);//2値画像の解釈を反転する。
//    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
//        png_set_invert_mono(pngStruct);//グレイスケール全般の解釈を反転する。
//    if (bitDepth == 16)
//        png_set_swap(pngStruct);//16ビット画像のエンディアンを変更する。
//    if (bitDepth < 8)
//        png_set_packswap(pngStruct);//4ビット以下の深度の場合のパッキングの順序を変更する。
//    png_set_read_user_transform_fn(pngStruct, );//自作変換関数を登録する。
//    int numPasses = png_set_interlace_handling(pngStruct);インターレース処理の登録。
    png_read_update_info(pngStruct, pngInfo);
    
    size_t curSize = storage->size();
    uint32_t stride;
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGBA)
        stride = 4;
    else
        stride = 1;
    size_t rowSize = *width * stride * sizeof(uint8_t);
    storage->resize(storage->size() + *height * rowSize);
    uint8_t* dataHead = &(*storage)[curSize];
    for (int i = 0; i < *height; ++i) {
        png_read_row(pngStruct, dataHead, nullptr);
        dataHead += rowSize;
    }
    
    png_read_end(pngStruct, pngEndInfo);
    png_destroy_read_struct(&pngStruct, &pngInfo, &pngEndInfo);
    
    fclose(fp);
    
    return true;
}

bool loadImage(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height, ColorChannel::Value* color, bool gammaCorrection) {
    std::string sFileName = fileName;
    
    size_t extPos = sFileName.find_first_of(".");
    if (extPos == std::string::npos)
        return false;
    
    std::string sExt = sFileName.substr(extPos + 1);
    if (!sExt.compare("jpg") || !sExt.compare("jpeg")) {
        *color = ColorChannel::RGB888;
        return loadJPEG(fileName, storage, width, height);
    }
    else if (!sExt.compare("png")) {
        return loadPNG(fileName, storage, width, height, color, gammaCorrection);
    }
    
    return false;
}

bool loadEnvMap(const char* fileName, std::vector<uint8_t>* storage, uint32_t* width, uint32_t* height) {
    EXRType exrtype;
    return loadEXR(fileName, storage, width, height, &exrtype);
}
