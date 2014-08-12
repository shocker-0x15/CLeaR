//
//  Matrix4fStack.hpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 11/11/25.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef OpenCL_PathTracer_Matrix4fStack_hpp
#define OpenCL_PathTracer_Matrix4fStack_hpp

#include "Matrix4f.hpp"
#include <stack>

class Matrix4fStack {
    std::stack<Matrix4f> m_stack;
    bool m_mulFromLeft;
    
public:
    Matrix4fStack(bool mulFromLeft = false) {
        m_stack.push(Matrix4f::Identity);
        m_mulFromLeft = mulFromLeft;
    }
    
    explicit Matrix4fStack(const Matrix4f& mat) {
        m_stack.push(mat);
    }
    
    void push() {
        m_stack.push(m_stack.top());
    }
    
    void pop() {
        if (m_stack.size() > 1)
            m_stack.pop();
        else {
            m_stack.top() = Matrix4f::Identity;
        }
    }
    
    Matrix4fStack& operator=(const Matrix4f& mat) {
        m_stack.top() = mat;
        return *this;
    }
    
    Matrix4fStack& operator*=(const Matrix4f& mat) {
        if (m_mulFromLeft)
            m_stack.top() = mat * m_stack.top();
        else
            m_stack.top() *= mat;
        return *this;
    }
    
    const Matrix4f& top() const {
        return m_stack.top();
    }
    
    Matrix4f& top() {
        return m_stack.top();
    }
    
    operator Matrix4f() const {
        return m_stack.top();
    }
};

#endif
