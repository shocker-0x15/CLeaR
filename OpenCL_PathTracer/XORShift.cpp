//
//  XORShift.cpp
//  GIRenderer
//
//  Created by 渡部 心 on 11/07/21.
//  Copyright 2011年 __MyCompanyName__. All rights reserved.
//

#include "XORShift.hpp"
#include <time.h>

XORShiftRandom32::XORShiftRandom32() {
    int seed = (int)time(NULL);
    for (unsigned int i = 0; i < 4; ++i) {
        seed128[i] = seed = 1812433253U * (seed ^ (seed >> 30)) + i;
    }
    for (int i = 0; i < 50; ++i) {
        getUInt();
    }
}

XORShiftRandom32::XORShiftRandom32(int seed) {
    for (unsigned int i = 0; i < 4; ++i) {
        seed128[i] = seed = 1812433253U * (seed ^ (seed >> 30)) + i;
    }
    for (int i = 0; i < 50; ++i) {
        getUInt();
    }
}
