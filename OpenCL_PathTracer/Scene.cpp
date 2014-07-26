//
//  Scene.cpp
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/07/15.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#include "Scene.h"
#include "CreateFunctions.hpp"

void Scene::build() {
    if (othersRef.count("Environment") == 0) {
        MaterialCreator &mc = MaterialCreator::sharedInstance();
        mc.setScene(this);
        
        mc.beginLightProperty("Dark");
        mc.endLightProperty();
        
        struct EnvironmentHead {
            uint32_t offsetEnvLightProperty;
        };
        EnvironmentHead envHead;
        envHead.offsetEnvLightProperty = (uint32_t)idxOfLight("Dark");
        setEnvironment(addDataAligned(&otherResouces, envHead, 4));
    }
    
    calcLightPowerDistribution();
    
    for (int i = 0; i < faces.size(); ++i) {
        BBox bb{vertices[faces[i].p0]};
        bb.unionP(vertices[faces[i].p1]);
        bb.unionP(vertices[faces[i].p2]);
        bvh.addLeaf(bb);
    }
    bvh.build();
    
    *(uint32_t*)&otherResouces[0] = (uint32_t)idxOfOther("Camera");
    *(uint32_t*)&otherResouces[4] = (uint32_t)idxOfOther("Environment");
    *(uint32_t*)&otherResouces[8] = (uint32_t)idxOfOther("LightPowerDistribution");
}
