//
//  extra_atomics.cl
//  OpenCL_PathTracer
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

void atomic_add_f32(volatile global float* address, float value);
void atomic_min_f32(volatile local float* address, float value);
void atomic_max_f32(volatile local float* address, float value);


void atomic_add_f32(volatile global float* address, float value) {
    uint oldval, newval, readback;
    
    oldval = as_uint(*address);
    newval = as_uint(*address + value);
    while ((readback = atomic_cmpxchg((volatile global uint*)address, oldval, newval)) != oldval) {
        oldval = readback;
        newval = as_uint(as_float(oldval) + value);
    }
}

void atomic_min_f32(volatile local float* address, float value) {
    uint oldval, newval, readback;
    
    oldval = as_uint(*address);
    newval = as_uint(fmin(*address, value));
    while ((readback = atomic_cmpxchg((volatile local uint*)address, oldval, newval)) != oldval) {
        oldval = readback;
        newval = as_uint(fmin(as_float(oldval), value));
    }
}

void atomic_max_f32(volatile local float* address, float value) {
    uint oldval, newval, readback;
    
    oldval = as_uint(*address);
    newval = as_uint(fmax(*address, value));
    while ((readback = atomic_cmpxchg((volatile local uint*)address, oldval, newval)) != oldval) {
        oldval = readback;
        newval = as_uint(fmax(as_float(oldval), value));
    }
}
