#define PARTICLE_RADIUS 0

#include <stdexcept>
#include <cmath>
#include <vector>
#include <fstream>

#include <cellular_automata/CellularAutomataSystem.h>
#include <cellular_automata/UpdateRule.h>
#include <utils/UniformDoubleGenerator.h>
#include <cell_index_method/ParticleSystem.h>

namespace {
struct DisjointSet {
    std::vector<int> parent, size;
    explicit DisjointSet(int n) : parent(n), size(n, 1) {
        for (int i = 0; i < n; i++) parent[i] = i;
    }
    int find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }
    void unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return;
        if (size[a] < size[b]) std::swap(a, b);
        parent[b] = a;
        size[a] += size[b];
    }
};
}

namespace cellular_automata {

int CellularAutomataSystem::computeSubsquareCount(int length, double interactionRadius, bool periodicBoundary) {
    int subsquareCount = static_cast<int>(length / interactionRadius);
    if (periodicBoundary && subsquareCount == 2) {
        throw std::invalid_argument(
            "Invalid subsquare count, try other length and interaction radius combination");
    }
    return subsquareCount;
}

std::vector<board_generation::Particle> CellularAutomataSystem::generateParticles(int count, int length) {
    std::vector<board_generation::Particle> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        double x = utils::UniformDoubleGenerator::getUniformDoubleValue(0, length);
        double y = utils::UniformDoubleGenerator::getUniformDoubleValue(0, length);
        result.emplace_back(i, x, y);
    }
    return result;
}

CellularAutomataSystem::CellularAutomataSystem(int length, 
                                            int particleCount, 
                                            double interactionRadius, 
                                            double noise, 
                                            int steps, 
                                            cellular_automata::UpdateRule* updateRule, 
                                            bool periodicBoundary)
    : length(length),
      particleCount(particleCount),
      interactionRadius(interactionRadius),
      noise(noise),
      steps(steps),
      updateRule(updateRule),
      periodicBoundary(periodicBoundary),
      subsquareCount(computeSubsquareCount(length, interactionRadius, periodicBoundary)),
      particles(generateParticles(particleCount, length)),
      particleSystem(particles, PARTICLE_RADIUS, particleCount, length,
                     subsquareCount, interactionRadius, true) { }

void CellularAutomataSystem::step() {
    particleSystem.getInteractions(cell_index_method::AlgorithmType::CELL_INDEX_METHOD);

    std::vector<board_generation::Particle>* particlesPtr = this->particleSystem.getParticles();
    int particlesCount = particlesPtr->size();

    std::vector<double> newAngles(particlesCount);
    for (int i = 0 ; i < particlesCount ; i++) {
        board_generation::Particle& p = (*particlesPtr)[i];
        newAngles[i] = updateRule->computeNewAngle(p, p.getNeighbours(), noise);
    }

    for (int i = 0 ; i < particlesCount ; i++) {
        board_generation::Particle& p = (*particlesPtr)[i];

        p.setAngle(newAngles[i]);
        p.updateVelocityFromAngle(newAngles[i]);

        double newX = std::fmod(p.getXLocation() + p.getXVelocity(), length);
        if (newX < 0) newX += length;
        p.setXLocation(newX);

        double newY = std::fmod(p.getYLocation() + p.getYVelocity(), length);
        if (newY < 0) newY += length;
        p.setYLocation(newY);
    }
}

void CellularAutomataSystem::run(int steps) {
    std::ofstream framesStream("resources/frames.txt");
    std::ofstream observablesStream("resources/observables.txt");

    for (int i = 0 ; i < steps ; i++) {
        step();
        writeFrame(&framesStream);
        writeObservablesLog(&observablesStream, i);
    }
}

double CellularAutomataSystem::cumputeOrderParameter() {
    double sumVx = 0.0;
    double sumVy = 0.0;

    for (const auto& p : particles) {
        sumVx += p.getXVelocity();
        sumVy += p.getYVelocity();
    }

    double speed = 0.03;
    double magnitude = std::sqrt(sumVx * sumVx + sumVy * sumVy);
    return magnitude / (particleCount * speed);

}

double CellularAutomataSystem::computeLargestClusterFraction() {
    DisjointSet dsu(particleCount);
    for (const auto& p : particles) {
        for (const board_generation::Particle* neighbour : p.getNeighbours()) {
            dsu.unite(p.getId(), neighbour->getId());
        }
    }

    int maxSize = 0;
    for (int i = 0; i < particleCount; i++) {
        maxSize = std::max(maxSize, dsu.size[dsu.find(i)]);
    }
    return static_cast<double>(maxSize) / particleCount;
}

void CellularAutomataSystem::writeFrame(std::ofstream* stream) {
    for (const board_generation::Particle& p : this->particles) {
        (*stream) << p.getXLocation() << " " << p.getYLocation() << " "
                << p.getAngle() << "\n";
    }
    (*stream) << "\n";
}

void CellularAutomataSystem::writeObservablesLog(std::ofstream* stream, int t) {
    double va = cumputeOrderParameter();
    double s = computeLargestClusterFraction();
    (*stream) << t << " " << va << " " << s << "\n";
}
}