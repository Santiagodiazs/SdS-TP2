#pragma once

#include <vector>

#include <cellular_automata/UpdateRule.h>

namespace cellular_automata {

class VicsekRule : public UpdateRule {
public:
    double computeNewAngle(board_generation::Particle& p, const std::vector<board_generation::Particle*>& neighbours, double noise) const override;
};
}