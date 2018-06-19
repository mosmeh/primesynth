#pragma once
#include <array>

namespace primasynth {

class Envelope {
public:
    enum class Section {
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Finished
    };

    Envelope(double outputRate, unsigned int interval);

    Section getSection() const;
    double getValue() const;
    double getAtten() const;
    bool isFinished() const;

    void setParameter(Section section, double param);
    void release();
    void update();

private:
    const double effectiveOutputRate_;
    std::array<double, 6> params_;
    Section section_;
    unsigned int sectionSteps_;
    double atten_, value_;

    void changeSection(Section section);
};

}
