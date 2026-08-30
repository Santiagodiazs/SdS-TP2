#pragma once

#include <vector>

#include <board_generation/Particle.h>

namespace board_generation {

class Tablero {
public:
    Tablero(int length, int particleCount);

    void generateStaticData();
    void generateDynamicData(int iterations);

    int getLength() const;
    int getParticleCount() const;
    double getParticleRadius() const;

    std::vector<Particle>& getParticles();

private:
    int length;
    int particleCount;
    double particleRadius;
    std::vector<Particle> particles;

    void generateParticles();
    bool overlapsNeighbours(std::vector<std::vector<std::vector<Particle*>>>& grid,
                             int gx, int gy, int gridSize,
                             double xLocation, double yLocation, double minDistSq);
};
}