kernel void tonemapping12B(uint width, uint height, uint b_width, uint spp,
                           global float3* src,
                           global uchar* dst) {
    const global float3* srcPix = (float*)src + (width * (get_global_size(0) - get_global_id(0) - 1) + get_global_id(1)) * 3;
    const uint dIdx = b_width * get_global_id(0) + get_global_id(1) * 3;

    float3 pixVal = *srcPix * (1.0f / spp);
    float Y = 0.2989 * pixVal.r + 0.5866 * pixVal.g + 0.1144 * pixVal.b;
    float scaleTM = (1.0 - exp(-Y)) / Y;
    if (Y == 0.0f) scaleTM = 1.0f;

    uchar3 result = convert_uchar3_rtz(min(255.0f * pow(scaleTM * pixVal, 1.0f / 2.2f), 255.0f));
    dst[dIdx + 0] = result.b;
    dst[dIdx + 1] = result.g;
    dst[dIdx + 2] = result.r;
}

kernel void linearmap(uint width, uint height, uint b_width, uint spp,
                      global float3* src,
                      global uchar* dst) {
    const global float3* srcPix = (float*)src + (width * (get_global_size(0) - get_global_id(0) - 1) + get_global_id(1)) * 3;
    const uint dIdx = b_width * get_global_id(0) + get_global_id(1) * 3;
    
    float3 pixVal = *srcPix;
    pixVal -= (float3)(3, 3, 3);
    pixVal /= 2;
    
    uchar3 result = convert_uchar3_rtz(min(255.0f * pixVal, 255.0f));
    dst[dIdx + 0] = result.b;
    dst[dIdx + 1] = result.g;
    dst[dIdx + 2] = result.r;
}

kernel void tonemapping16B(uint width, uint height, uint b_width, uint spp,
                           global float3* src, global uchar* dst) {
    const global float3* srcPix = src + width * (get_global_size(1) - get_global_id(1) - 1) + get_global_id(0);
    const uint dIdx = b_width * get_global_id(1) + get_global_id(0) * 3;
    
    float3 pixVal = *srcPix * (1.0f / spp);
    float Y = 0.2989 * pixVal.r + 0.5866 * pixVal.g + 0.1144 * pixVal.b;
    float scaleTM = (1.0 - exp(-Y)) / Y;
    if (Y == 0.0f) scaleTM = 1.0f;
    
    uchar3 result = convert_uchar3_rtz(min(255.0f * pow(scaleTM * pixVal, 1.0f / 2.2f), 255.0f));
    dst[dIdx + 0] = result.b;
    dst[dIdx + 1] = result.g;
    dst[dIdx + 2] = result.r;
}