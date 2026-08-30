#include <iostream>
#include <vector>
#include <random>
#include <cmath>

#include <cellular_automata/VoterRule.h>
#include <utils/UniformIntGenerator.h>
#include <utils/UniformDoubleGenerator.h>

namespace cellular_automata {

double VoterRule::computeNewAngle(board_generation::Particle& p,
                                    const std::vector<board_generation::Particle*>& neighbours,
                                    double noise) const {
    double noiseTerm = utils::UniformDoubleGenerator::getUniformDoubleValue(-noise/2.0, noise/2.0);

    if (neighbours.empty()) {
        return p.getAngle() + noiseTerm;
    }

    int randomIndex = utils::UniformIntGenerator::getUniformIntValue(0, neighbours.size() - 1);

    return neighbours[randomIndex]->getAngle() + noiseTerm;
}

}