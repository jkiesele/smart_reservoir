#pragma once

#include <vector>
#include <Arduino.h>
#include <stdint.h>

struct FillSensorPoint {
    uint8_t pin;
    float fraction;
};

class FillSensorConfig {
public:
    FillSensorConfig(std::initializer_list<FillSensorPoint> points,
    int referencePin = -1)
        : points_(points), referencePin_(referencePin)
         {}

    const std::vector<FillSensorPoint>& points() const { return points_; }
    const int referencePin() const { return referencePin_; }

    bool isValid() const {
        if (points_.empty()) return false;

        float prev = 0.0f;
        for (const auto& p : points_) {
            if (p.fraction <= 0.0f || p.fraction > 1.0f) return false;
            if (p.fraction <= prev) return false;
            prev = p.fraction;
        }

        return fabsf(points_.back().fraction - 1.0f) <= 1e-3f;
    }

private:
    std::vector<FillSensorPoint> points_;
    int referencePin_;
};