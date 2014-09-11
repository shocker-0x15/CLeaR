//
//  StopWatch.h
//  OpenCL_PathTracer
//
//  Created by 渡部 心 on 2014/09/12.
//  Copyright (c) 2014年 渡部 心. All rights reserved.
//

#ifndef __OpenCL_PathTracer__StopWatch__
#define __OpenCL_PathTracer__StopWatch__

#include <chrono>
#include <vector>
#include <stdint.h>

namespace {
    using namespace std::chrono;
    
    class StopWatch {
        std::vector<system_clock::time_point> m_startTPStack;
    public:
        typedef enum {
            Microseconds,
            Milliseconds,
            Seconds,
        } EDurationType;
        
        system_clock::time_point start() {
            system_clock::time_point startTimePoint = system_clock::now();
            m_startTPStack.push_back(startTimePoint);
            return startTimePoint;
        }
        
        inline uint64_t durationCast(const system_clock::duration &duration, EDurationType dt) {
            switch (dt) {
                case Microseconds:
                    return duration_cast<microseconds>(duration).count();
                case Milliseconds:
                    return duration_cast<milliseconds>(duration).count();
                case Seconds:
                    return duration_cast<seconds>(duration).count();
                default:
                    break;
            }
            return UINT64_MAX;
        }
        
        uint64_t stop(EDurationType dt = EDurationType::Milliseconds) {
            system_clock::duration duration = system_clock::now() - m_startTPStack.back();
            m_startTPStack.pop_back();
            return durationCast(duration, dt);
        }
        
        uint64_t elapsed(EDurationType dt = EDurationType::Milliseconds) {
            system_clock::duration duration = system_clock::now() - m_startTPStack.back();
            return durationCast(duration, dt);
        }
        
        uint64_t elapsedFromRoot(EDurationType dt = EDurationType::Milliseconds) {
            system_clock::duration duration = system_clock::now() - m_startTPStack.front();
            return durationCast(duration, dt);
        }
    };
}

#endif /* defined(__OpenCL_PathTracer__StopWatch__) */
