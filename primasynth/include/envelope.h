#pragma once
#include <array>

namespace primasynth {
class Envelope {
public:
    enum class Phase { Delay, Attack, Hold, Decay, Sustain, Release, Finished };

    Envelope(double outputRate, unsigned int interval);

    Phase getPhase() const;
    double getValue() const;

    void setParameter(Phase phase, double param);
    void release();
    void update();

private:
    const double effectiveOutputRate_;
    std::array<double, 6> params_;
    Phase phase_;
    unsigned int phaseSteps_;
    double value_;

    void changePhase(Phase phase);
};
}
