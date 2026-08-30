#pragma once

#include <vector>

#include <cellular_automata/UpdateRule.h>
#include <board_generation/Particle.h>
#include <cell_index_method/ParticleSystem.h>

namespace cellular_automata {

class CellularAutomataSystem {
public:
    CellularAutomataSystem(
        int length, 
        int particleCount, 
        double interactionRadius, 
        double noise, int steps, 
        UpdateRule* updateRule, 
        bool periodicBoundary
    );

    void step();
    void run(int steps);
    double cumputeOrderParameter();
    double computeLargestClusterFraction();
    void writeFrame(std::ofstream* stream);
    void writeObservablesLog(std::ofstream* stream, int t);

private:
    int length;
    int particleCount;
    double interactionRadius;
    double noise;
    int steps;
    int subsquareCount;
    UpdateRule* updateRule;
    bool periodicBoundary;

    std::vector<board_generation::Particle> particles;
    cell_index_method::ParticleSystem particleSystem;

    static int computeSubsquareCount(int length, double interactionRadius, bool periodicBoundary);
    static std::vector<board_generation::Particle> generateParticles(int count, int length);
};
}