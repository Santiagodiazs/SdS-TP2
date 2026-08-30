#include <board_generation/Tablero.h>

#include <Utils/UniformDoubleGenerator.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>

namespace board_generation {

Tablero::Tablero(int length, int particleCount)
    : length(length), particleCount(particleCount) {
    this->particleRadius = utils::UniformDoubleGenerator::getUniformDoubleValue(0.23, 0.26);
    generateParticles();
}

/**
 * Lo que hago es una especie de CIM donde por cada particula en cierto lugar
 * de la grid, checkea si no se superpone con las particulas de su grid y las 8
 * que estan rodeandola
 */
void Tablero::generateParticles() {
    particles.clear();
    // Reservamos de antemano para que el vector no reubique su memoria y
    // los punteros que guardamos en 'grid' sigan siendo validos (en Java
    // esto no hace falta porque los objetos viven en el heap por separado).
    particles.reserve(particleCount);

    std::random_device rd;
    std::mt19937 random(rd());

    double cellSize = 2 * particleRadius;
    int gridSize = static_cast<int>(std::ceil(static_cast<double>(length) / cellSize));
    if (gridSize < 1) {
        gridSize = 1;
    }

    std::vector<std::vector<std::vector<Particle*>>> grid(
        gridSize, std::vector<std::vector<Particle*>>(gridSize));

    double minDistSq = std::pow(2 * particleRadius, 2);

    double minCoord = particleRadius;
    double maxCoord = length - particleRadius;
    int maxAttempts = 10000;

    std::uniform_real_distribution<double> coordDist(minCoord, maxCoord);

    for (int i = 0; i < particleCount; i++) {
        bool placed = false;
        for (int attempt = 0; attempt < maxAttempts; attempt++) {
            double xLocation = coordDist(random);
            double yLocation = coordDist(random);

            int gx = static_cast<int>(xLocation / cellSize);
            int gy = static_cast<int>(yLocation / cellSize);

            if (overlapsNeighbours(grid, gx, gy, gridSize, xLocation, yLocation, minDistSq)) {
                continue;
            }

            particles.emplace_back(i, xLocation, yLocation, particleRadius);
            grid[gx][gy].push_back(&particles.back());
            placed = true;
            break;
        }

        if (!placed) {
            throw std::runtime_error(
                "No se pudieron generar mas particulas sin superposicion. Reduci N o aumenta L.");
        }
    }
}

bool Tablero::overlapsNeighbours(std::vector<std::vector<std::vector<Particle*>>>& grid,
                                  int gx, int gy, int gridSize,
                                  double xLocation, double yLocation, double minDistSq) {
    for (int dx = -1; dx <= 1; dx++) {
        int nx = gx + dx;
        if (nx < 0 || nx >= gridSize) {
            continue;
        }
        for (int dy = -1; dy <= 1; dy++) {
            int ny = gy + dy;
            if (ny < 0 || ny >= gridSize) {
                continue;
            }
            for (Particle* other : grid[nx][ny]) {
                double ddx = xLocation - other->getXLocation();
                double ddy = yLocation - other->getYLocation();
                double distSq = ddx * ddx + ddy * ddy;
                if (distSq < minDistSq) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Tablero::generateStaticData() {
    std::ofstream writer("resources/static_board_data.txt");
    if (!writer) {
        std::cerr << "Error writing data: no se pudo abrir el archivo" << std::endl;
        return;
    }
    writer << particleCount << "\n";
    writer << length << "\n";
    for (const Particle& p : particles) {
        writer << p.getParticleRadius() << " " << p.getColour() << "\n";
    }
}

void Tablero::generateDynamicData(int iterations) {
    std::ofstream writer("resources/dynamic_board_data.txt");
    if (!writer) {
        std::cerr << "Error writing data: no se pudo abrir el archivo" << std::endl;
        return;
    }
    for (int i = 0; i < iterations; i++) {
        writer << i << "\n";
        for (const Particle& p : particles) {
            writer << p.toString() << "\n";
        }
    }
}

int Tablero::getLength() const { return length; }
int Tablero::getParticleCount() const { return particleCount; }
double Tablero::getParticleRadius() const { return particleRadius; }

std::vector<Particle>& Tablero::getParticles() { return particles; }

} // namespace board_generation
