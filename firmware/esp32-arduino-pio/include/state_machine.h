#pragma once
#include <Arduino.h>
#include "config.h"

// State machine sesuai Bab VI.C proposal:
// IDLE -> SELF_CHECK -> ARMED -> INFLATING -> SAMPLING -> LOP_DETECTED -> HOLDING
// -> MONITORING -> ALARM -> SLOW_RELEASE -> DEFLATED

enum class TourniquetState {
    IDLE,
    SELF_CHECK,
    ARMED,
    INFLATING,
    SAMPLING,
    LOP_DETECTED,
    HOLDING,
    MONITORING,
    ALARM,
    SLOW_RELEASE,
    DEFLATED
};

inline const char* stateToString(TourniquetState s) {
    switch (s) {
        case TourniquetState::IDLE:         return "IDLE";
        case TourniquetState::SELF_CHECK:   return "SELF_CHECK";
        case TourniquetState::ARMED:        return "ARMED";
        case TourniquetState::INFLATING:    return "INFLATING";
        case TourniquetState::SAMPLING:     return "SAMPLING";
        case TourniquetState::LOP_DETECTED: return "LOP_DETECTED";
        case TourniquetState::HOLDING:      return "HOLDING";
        case TourniquetState::MONITORING:   return "MONITORING";
        case TourniquetState::ALARM:        return "ALARM";
        case TourniquetState::SLOW_RELEASE: return "SLOW_RELEASE";
        case TourniquetState::DEFLATED:     return "DEFLATED";
        default: return "UNKNOWN";
    }
}
