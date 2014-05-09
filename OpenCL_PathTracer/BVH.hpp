//
//  BVH.h
//  OpenCL_TEST
//
//  Created by 渡部 心 on 2014/05/04.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_TEST__BVH__
#define __OpenCL_TEST__BVH__

#include <cstdio>
#include <cmath>
#include <vector>
#include "clUtility.hpp"

struct BBox {
    cl_float3 min;
    cl_float3 max;
    
    BBox() {
        min.s0 = min.s1 = min.s2 = INFINITY;
        max.s0 = max.s1 = max.s2 = -INFINITY;
    };
    BBox(const cl_float3 &p) : min(p), max(p) { };
    BBox(const cl_float3 &pmin, const cl_float3 &pmax) : min(pmin), max(pmax) { };
    
    float centerOfAxis(uint8_t axis) const {
        if (axis == 0)
            return (min.s0 + max.s0) * 0.5f;
        else if (axis == 1)
            return (min.s1 + max.s1) * 0.5f;
        else
            return (min.s2 + max.s2) * 0.5f;
    }
    
    uint8_t widestAxis() const {
        float dx = max.s0 - min.s0;
        float dy = max.s1 - min.s1;
        float dz = max.s2 - min.s2;
        if (dx > dy && dx > dz)
            return 0;
        else if (dy > dz)
            return 1;
        else
            return 2;
    }
    
    void unionP(const cl_float3 &p) {
        min = minCLfloat3(min, p);
        max = maxCLfloat3(max, p);
    }
    
    void unionBB(const BBox &b) {
        min = minCLfloat3(min, b.min);
        max = maxCLfloat3(max, b.max);
    }
};

struct BVHNode {
    BBox bbox;
    uint32_t children[2]; uint32_t dum0[2];
    BVHNode(const BBox &bb, uint32_t child1 = -1, uint32_t child2 = -1) : bbox(bb) {
        children[0] = child1;
        children[1] = child2;
    };
};

struct BVH {
    std::vector<BBox> leaves;
    std::vector<uint32_t> lIdx;
    std::vector<BVHNode> nodes;
    
    void addLeaf(const BBox &bb) {
        leaves.push_back(bb);
        lIdx.push_back((uint32_t)leaves.size() - 1);
    }
    
    inline void swapIndex(uint32_t left, uint32_t right) {
        uint32_t temp = lIdx[left];
        lIdx[left] = lIdx[right];
        lIdx[right] = temp;
    }
    
    void divide(uint32_t start, uint32_t end, uint8_t axis, uint32_t splitIdx) {
        if (splitIdx <= start || splitIdx + 1 >= end)
            return;
        uint32_t left = start - 1;
        uint32_t right = end;
        while (true) {
            float pivot = leaves[lIdx[(left + right) / 2]].centerOfAxis(axis);
            uint32_t curStart = left;
            uint32_t curEnd = right;
            while (true) {
                while (leaves[lIdx[++left]].centerOfAxis(axis) < pivot);
                while (leaves[lIdx[--right]].centerOfAxis(axis) > pivot);
                
                if (left < right)
                    swapIndex(left, right);
                else
                    break;
            }
            if (left > splitIdx) {
                right = left;
                left = curStart;
            }
            else {
                left = right;
                right = curEnd;
            }
            if (right - left < 3)
                return;
        }
    }
    
    void buildRecursive(uint32_t parent, uint32_t start, uint32_t end) {
        BBox bbox;
        for (uint32_t i = start; i < end; ++i)
            bbox.unionBB(leaves[lIdx[i]]);
        
        nodes[parent].bbox = bbox;
        
        if (end - start < 2) {
            nodes[parent].children[0] = lIdx[start];
            return;
        }
        
        uint8_t widestAxis = bbox.widestAxis();
        uint32_t median = (start + end) / 2;
        divide(start, end, widestAxis, median);
        
        BVHNode child1{BBox{}};
        nodes.push_back(child1);
        nodes[parent].children[0] = (uint32_t)nodes.size() - 1;
        buildRecursive((uint32_t)nodes.size() - 1, start, median);
        
        BVHNode child2{BBox{}};
        nodes.push_back(child2);
        nodes[parent].children[1] = (uint32_t)nodes.size() - 1;
        buildRecursive((uint32_t)nodes.size() - 1, median, end);
    }
    
    void build() {
        nodes.push_back(BVHNode{BBox{}});
        buildRecursive(0, 0, (uint32_t)lIdx.size());
    }
};

#endif /* defined(__OpenCL_TEST__BVH__) */
