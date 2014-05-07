//
//  XORShift.hpp
//  GIRenderer
//
//  Created by 渡部 心 on 11/07/21.
//  Copyright 2011年 __MyCompanyName__. All rights reserved.
//

#ifndef GIRenderer_XORShift_hpp
#define GIRenderer_XORShift_hpp

class XORShiftRandom32 {
    unsigned int seed128[4];
public:
    XORShiftRandom32();
    XORShiftRandom32(int seed);
    XORShiftRandom32(const XORShiftRandom32 &xor32) {
        for (int i = 0; i < 4; ++i) {
            seed128[i] = xor32.seed128[i];
        }
    }
    
    unsigned int getUInt() {
        unsigned int* a(seed128);
        unsigned int  t(a[0] ^ (a[0] << 11));
        a[0] = a[1];
        a[1] = a[2];
        a[2] = a[3];
        return a[3] = (a[3] ^ (a[3] >> 19)) ^ (t ^ (t >> 8));
    };
    
    float getFloat0cTo1o() {
        return getUInt() * (1.0 / 4294967296.0);//この方法ではfloatの精度上2^32に近い数字は1をとってしまうことがある．
        //        return (getUInt() & 0xffffff) / (float)(1 << 24);//この方法なら1は絶対にとらないが乱数の質としては低下する？
    };
    
    float getFloat0cTo1c() {
        return getUInt() * (1.0 / 4294967295.0);
        //        return (getUInt() & 0xffffff) / (float)(1 << 24 - 1);
    };
    
    //このままでは極めて1に近い数字はfloatの精度上1になってしまうため問題？．
    float getFloat0oTo1o() {
        return (float)(((double)getUInt() + 0.5) * (1.0 / 4294967296.0));
    };
};

#endif
