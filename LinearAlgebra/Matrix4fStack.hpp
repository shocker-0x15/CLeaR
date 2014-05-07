//
//  Matrix4fStack.hpp
//  GIRenderer
//
//  Created by 渡部 心 on 11/11/25.
//  Copyright (c) 2011年 __MyCompanyName__. All rights reserved.
//

#ifndef GIRenderer_Matrix4fStack_hpp
#define GIRenderer_Matrix4fStack_hpp

#include "GlobalIncludes.hpp"
#include "Matrix4f.hpp"

class Matrix4fStack {
    std::vector<Matrix4f> stack;
public:
    Matrix4f current;
    
    Matrix4fStack() : current(Matrix4f::Identity) { };
    
    void push() {
        stack.push_back(current);
    }
    
    void pop() {
        if (stack.size() > 0) {
            current = stack[stack.size() - 1];
            stack.pop_back();
        }
    }
    
    void preMult(const Matrix4f &mat) {
        current = mat * current;
    }
    
    void postMult(const Matrix4f &mat) {
        current *= mat;
    }
};

#endif
