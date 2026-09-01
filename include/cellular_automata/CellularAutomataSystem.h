#pragma once

#include <stdexcept>
#include <cmath>
#include <vector>
#include <fstream>
#include <string>

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
        bool periodicBoundary,
        int subsquareCountOverride = 0
    );

    void step();
    void run(int steps, const std::string& outputDirectory = "resources/",
             bool writeFrames = true);
    double cumputeOrderParameter();
    double computeLargestClusterFraction();
    void writeFrame(std::ofstream* stream);
    void writeObservablesLog(std::ofstream* stream, int t);
    long long benchmark(cell_index_method::AlgorithmType algorithmType);
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

    static int computeSubsquareCount(int length, double interactionRadius, bool periodicBoundary,
                                     int subsquareCountOverride = 0);
    static std::vector<board_generation::Particle> generateParticles(int count, int length);
};
}
