#pragma once
#include <Arduino.h>

// PID controller untuk mengatur kecepatan pompa saat inflasi menuju target incremental,
// sesuai Bab VI.B: rise time <10s, overshoot <5% setpoint, steady-state error <2 mmHg.
// Parameter Kp/Ki/Kd default di bawah adalah TITIK AWAL — wajib di-tuning ulang dengan
// Ziegler-Nichols / eksperimen di phantom arm, sesuai catatan proposal.

class PIDController {
public:
    PIDController(float kp = 2.0f, float ki = 0.5f, float kd = 0.1f)
        : Kp(kp), Ki(ki), Kd(kd) {}

    void setTunings(float kp, float ki, float kd) {
        Kp = kp; Ki = ki; Kd = kd;
    }

    void reset() {
        integral = 0.0f;
        lastError = 0.0f;
        lastTimeMs = millis();
    }

    // setpoint & measured dalam mmHg, output di-clamp ke [0, 255] untuk duty PWM pompa
    uint8_t compute(float setpoint, float measured) {
        unsigned long now = millis();
        float dt = (now - lastTimeMs) / 1000.0f;
        if (dt <= 0.0f) dt = 0.001f;

        float error = setpoint - measured;

        integral += error * dt;
        // anti-windup: clamp integral term supaya tidak menumpuk berlebihan
        // saat output sudah saturasi (mis. selama fase awal inflasi tekanan masih jauh)
        const float integralMax = 100.0f;
        if (integral > integralMax) integral = integralMax;
        if (integral < -integralMax) integral = -integralMax;

        float derivative = (error - lastError) / dt;

        float output = Kp * error + Ki * integral + Kd * derivative;

        lastError = error;
        lastTimeMs = now;

        if (output < 0) output = 0;
        if (output > 255) output = 255;
        return (uint8_t)output;
    }

private:
    float Kp, Ki, Kd;
    float integral = 0.0f;
    float lastError = 0.0f;
    unsigned long lastTimeMs = 0;
};
