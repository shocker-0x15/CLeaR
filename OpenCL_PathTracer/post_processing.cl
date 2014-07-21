void atomic_add_f32(volatile global float* address, float value);

void atomic_add_f32(volatile global float* address, float value) {
    uint oldval, newval, readback;
    
    oldval = as_uint(*address);
    newval = as_uint(*address + value);
//    *(float*)&oldval = *address;
//    *(float*)&newval = (*(float*)&oldval + value);
    while ((readback = atomic_cmpxchg((volatile global uint*)address, oldval, newval)) != oldval) {
        oldval = readback;
        newval = as_uint(as_float(oldval) + value);
//        *(float*)&newval = (*(float*)&oldval + value);
    }
//    return as_float(oldval);
//    return *(float*)&oldval;
}

kernel void clear(uint width, uint height, global float3* buffer) {
    *(buffer + width * get_global_id(1) + get_global_id(0)) = (float3)(0.0f, 0.0f, 0.0f);
}

kernel void scaling(uint width, uint height, uint spp, global float3* src, global float3* output) {
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const global float3* srcPix = src + width * gid1 + gid0;
    global float3* dstPix = output + width * gid1 + gid0;
    *dstPix = *srcPix * (1.0f / spp);
}

constant float gauss5x5[] = {
    0.002969017f, 0.01330621f, 0.021938231f, 0.01330621f, 0.002969017f,
    0.01330621f, 0.059634295f, 0.098320331f, 0.059634295f, 0.01330621f,
    0.021938231f, 0.098320331f, 0.162102822f, 0.098320331f, 0.021938231f,
    0.01330621f, 0.059634295f, 0.098320331f, 0.059634295f, 0.01330621f,
    0.002969017f, 0.01330621f, 0.021938231f, 0.01330621f, 0.002969017f,
};
constant float gauss9x9[] = {
    0.000763447f, 0.001831415f, 0.003421534f, 0.004978302f, 0.005641155f, 0.004978302f, 0.003421534f, 0.001831415f, 0.000763447f,
    0.001831415f, 0.004393336f, 0.008207832f, 0.011942326f, 0.013532428f, 0.011942326f, 0.008207832f, 0.004393336f, 0.001831415f,
    0.003421534f, 0.008207832f, 0.01533425f, 0.022311201f, 0.025281903f, 0.022311201f, 0.01533425f, 0.008207832f, 0.003421534f,
    0.004978302f, 0.011942326f, 0.022311201f, 0.032462606f, 0.036784952f, 0.032462606f, 0.022311201f, 0.011942326f, 0.004978302f,
    0.005641155f, 0.013532428f, 0.025281903f, 0.036784952f, 0.041682812f, 0.036784952f, 0.025281903f, 0.013532428f, 0.005641155f,
    0.004978302f, 0.011942326f, 0.022311201f, 0.032462606f, 0.036784952f, 0.032462606f, 0.022311201f, 0.011942326f, 0.004978302f,
    0.003421534f, 0.008207832f, 0.01533425f, 0.022311201f, 0.025281903f, 0.022311201f, 0.01533425f, 0.008207832f, 0.003421534f,
    0.001831415f, 0.004393336f, 0.008207832f, 0.011942326f, 0.013532428f, 0.011942326f, 0.008207832f, 0.004393336f, 0.001831415f,
    0.000763447f, 0.001831415f, 0.003421534f, 0.004978302f, 0.005641155f, 0.004978302f, 0.003421534f, 0.001831415f, 0.000763447f,
};

kernel void gaussianScattering(uint width, uint height, uint spp, global float3* src, global float3* output) {
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const global float3* srcPix = src + width * gid1 + gid0;
    
    const int fRadius = 4;
    for (int i = -fRadius; i <= fRadius; ++i) {
        for (int j = -fRadius; j <= fRadius; ++j) {
            int px = clamp((int)gid0 + j, 0, (int)width - 1);
            int py = clamp((int)gid1 + i, 0, (int)height - 1);
            global float* dstPixBase = (global float*)(output + width * py + px);
            float filterVal = gauss9x9[(2 * fRadius + 1) * (i + fRadius) + (j + fRadius)] * (1.0f / spp);
            atomic_add_f32(dstPixBase + 0, filterVal * srcPix->s0);
            atomic_add_f32(dstPixBase + 1, filterVal * srcPix->s1);
            atomic_add_f32(dstPixBase + 2, filterVal * srcPix->s2);
        }
    }
}

constant float threshold = 30;
kernel void bloom(uint width, uint height, uint spp, global float3* src, global float3* output) {
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const global float3* srcPix = src + width * gid1 + gid0;
    
    for (int i = 0; i < 3; ++i) {
        float pixVal = *((const global float*)srcPix + i) * (1.0f / spp);
        if (pixVal > threshold) {
            float excess = pixVal - threshold;
            const int fRadius = excess > 7.0f ? 7 : (int)excess;
            for (int dy = -fRadius; dy <= fRadius; ++dy) {
                for (int dx = -fRadius; dx <= fRadius; ++dx) {
                    float dist = sqrt((float)(dx * dx + dy * dy));
                    
                    int px = clamp((int)gid0 + dx, 0, (int)width - 1);
                    int py = clamp((int)gid1 + dy, 0, (int)height - 1);
                    global float* dstPixBase = (global float*)(output + width * py + px);
                    if (dx != 0 || dy != 0)
                        atomic_add_f32(dstPixBase + i, exp(-dist) * excess);
                    else
                        atomic_add_f32(dstPixBase + i, threshold);
                }
            }
        }
        else {
            global float* dstPixBase = (global float*)(output + width * gid1 + gid0);
            atomic_add_f32(dstPixBase + i, pixVal);
        }
    }
}

kernel void toneMapping(uint width, uint height, uint b_width, global float3* src, global uchar* dst) {
    const uint gid0 = get_global_id(0);
    const uint gid1 = get_global_id(1);
    const global float3* srcPix = src + width * gid1 + gid0;
    const uint dIdx = b_width * (height - 1 - gid1) + gid0 * 3;
    
    float3 pixVal = *srcPix;
    float3 satVal = (float3)(pixVal.s0 > threshold ? threshold : pixVal.s0,
                             pixVal.s1 > threshold ? threshold : pixVal.s1,
                             pixVal.s2 > threshold ? threshold : pixVal.s2);
    float Y = 0.2989 * satVal.s0 + 0.5866 * satVal.s1 + 0.1144 * satVal.s2;
    float scaleTM = (1.0 - exp(-Y)) / Y;
    if (Y == 0.0f)
        scaleTM = 1.0f;
    
    uchar3 result = convert_uchar3_rtz(clamp(255.0f * pow(scaleTM * satVal, 1.0f / 2.2f), 0.0f, 255.0f));
    dst[dIdx + 0] = result.b;
    dst[dIdx + 1] = result.g;
    dst[dIdx + 2] = result.r;
}
