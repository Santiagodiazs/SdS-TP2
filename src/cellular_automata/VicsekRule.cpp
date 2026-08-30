#include <iostream>
#include <cmath>

#include <cellular_automata/VicsekRule.h>
#include <utils/UniformDoubleGenerator.h>

namespace cellular_automata {

double VicsekRule::computeNewAngle(board_generation::Particle& p,
                                    const std::vector<board_generation::Particle*>& neighbours,
                                    double noise) const {
    double sinSum = std::sin(p.getAngle());
    double cosSum = std::cos(p.getAngle());
    int count = 1;

    for (const board_generation::Particle* particle : neighbours) {
        sinSum += std::sin(particle->getAngle());
        cosSum += std::cos(particle->getAngle());
        count++;
    }

    double sinAvg = sinSum / count;
    double cosAvg = cosSum / count;

    double noiseTerm = utils::UniformDoubleGenerator::getUniformDoubleValue(-noise / 2.0, noise / 2.0);

    return std::atan2(sinAvg, cosAvg) + noiseTerm;
}

}