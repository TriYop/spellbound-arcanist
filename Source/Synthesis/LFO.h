#pragma once
#include <cmath>

class LFO
{
public:
    LFO();
    ~LFO();

    void prepare (double sampleRate);

    float process();

    void setSpeed (float hz) { speed_ = hz; }

private:
    double sampleRate_ = 44100.0;
    float speed_ = 0.5f;
    float phase_ = 0.f;

    static constexpr float TWO_PI = 6.28318530718f;
};
