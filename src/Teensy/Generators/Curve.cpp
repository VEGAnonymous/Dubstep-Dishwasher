#include "Teensy/Generators/Curve.h"
#include "Teensy/Utilities/Utilities.h"

#include <cmath>
#include <algorithm>

/* PRIVATE */

/*

std::vector<CurvePoint> points;
float freq, phase; bool loop;

*/
        
inline float Curve::applyCurve(float t, float curveAmount) const {
    if (curveAmount == 0.0f) return t; // Linear

    // Exponential
    float exp = powf(2.0f, fabsf(curveAmount) * 3.0f);
    if (curveAmount < 0.0f) return 1.0f - powf(1.0f - t, exp); // Ease-in
    else return powf(t, exp); // Ease-out
}

float Curve::evaluateCurve(float x) const {
    if (points.size() < 2) return 0.0f;
    
    float xClamped = loop ? fmodf(x, 1.0f) : std::clamp(x, 0.0f, 1.0f);
    if (xClamped < 0.0f) xClamped += 1.0f;
    
    // Find segment containing x
    int segmentIndex = -1;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        if (points[i].x <= xClamped && xClamped <= points[i + 1].x) { 
            segmentIndex = i; 
            break; 
        }
    }
    
    // Edge cases, endpoints
    if (segmentIndex == -1) {
        if (xClamped <= points.front().x) return points.front().y;
        if (xClamped >= points.back().x) return points.back().y;
        return 0.0f;
    }
    
    // Interpolate within segment
    const CurvePoint& p0 = points[segmentIndex];
    const CurvePoint& p1 = points[segmentIndex + 1];
    
    float t = (xClamped - p0.x) / (p1.x - p0.x);
    float curved = applyCurve(t, p0.curve);
    
    // Lerp y
    return lerp(p0.y, p1.y, curved);
}

/* PRIVATE */

Curve::Curve(float freq, bool loop) : freq(freq), phase(0.0f), loop(loop) {
    // Default Tri UP
    points.push_back(CurvePoint(0.0f, 0.0f, 0.0f));
    points.push_back(CurvePoint(0.5f, 1.0f, 0.0f));
    points.push_back(CurvePoint(1.0f, 0.0f, 0.0f));
}

void Curve::setFreq(float freq) { this->freq = freq; }
float Curve::getFreq() const { return freq; }

void Curve::setPhase(float phase) { this->phase = std::clamp(phase, 0.0f, 1.0f); }
float Curve::getPhase() const { return phase; }

void Curve::setCurve(const std::vector<CurvePoint>& points_) {
    if (points_.size() < 2) return;
    points = points_;
    
    // Sort by x
    std::sort(points.begin(), points.end(), [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
    // Ensure endpoints are actually endpoints
    if (points.front().x != 0.0f) points.insert(points.begin(), CurvePoint(0.0f, points.front().y, 0.0f));
    if (points.back().x != 1.0f) points.push_back(CurvePoint(1.0f, points.back().y, 0.0f));
}

void Curve::setCurvePoint(float x, float y, float curve) {
    if (x == 0.0f) points[0] = CurvePoint(x, y, curve);
    else if (x == 1.0f) points.back() = CurvePoint(x, y, curve);
    else points.push_back(CurvePoint(x, y, curve));
    // Resort by x if necessary
    std::sort(points.begin(), points.end(), [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
}

void Curve::clearCurve() {
    points.clear();
    // Set minimal valid curve
    points.push_back(CurvePoint(0.0f, 0.0f, 0.0f));
    points.push_back(CurvePoint(1.0f, 0.0f, 0.0f));
}

const std::vector<CurvePoint>& Curve::getCurve() const { return points; }

float Curve::next() {
    float val = evaluateCurve(phase);

    // Advance phase
    phase += freq / SAMPLE_RATE;
    if (phase >= 1.0f) phase -= 1.0f;
    
    return val;
}

void Curve::printCurve() const {
    Serial.printf("Curve: %d points, freq=%.3f, phase=%.3f, loop=%d\n", 
                    points.size(), freq, phase, loop);
    for (size_t i = 0; i < points.size(); ++i) {
        Serial.printf("  Point %d: x=%.3f, y=%.3f, curve=%.3f\n", 
                        i, points[i].x, points[i].y, points[i].curve);
    }
}