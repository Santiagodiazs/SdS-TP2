#pragma once

#include <vector>

#include <board_generation/Particle.h>

namespace cellular_automata {

    class UpdateRule {

    public:
        virtual ~UpdateRule() = default;

        virtual double computeNewAngle(board_generation::Particle& p, const std::vector<board_generation::Particle*>& neighbours, double noise) const = 0;
    };
    
}

