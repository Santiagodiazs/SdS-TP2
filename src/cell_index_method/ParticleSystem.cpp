#include <cell_index_method/ParticleSystem.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using board_generation::Particle;

namespace cell_index_method {

namespace {
// Equivalente a Math.floorMod(x, m) de Java: siempre devuelve un resultado
// no negativo (a diferencia del operador % de C++ con numeros negativos).
int floorMod(int x, int m) {
    int r = x % m;
    if (r != 0 && ((r < 0) != (m < 0))) {
        r += m;
    }
    return r;
}
}

ParticleSystem::ParticleSystem(double interactionRadius)
    : particlesCount(1),
      length(1),
      subsquareCount(1),
      interactionRadius(interactionRadius),
      particlesRadius(1.0),
      periodicBoundary(false),
      ownedParticles() {
    this->particles = &ownedParticles;
    this->interactionsBuffer.resize(1);
    this->gridBuffer.resize(1);
}

ParticleSystem::ParticleSystem(std::vector<Particle>& particles,
                               double particlesRadius,
                               int particlesCount,
                               int length,
                               int subsquareCount,
                               double interactionRadius,
                               bool periodicBoundary)
    : particlesCount(particlesCount),
      length(length),
      subsquareCount(subsquareCount),
      interactionRadius(interactionRadius),
      particlesRadius(particlesRadius),
      periodicBoundary(periodicBoundary) {

    if (periodicBoundary && subsquareCount == 2) {
        throw std::invalid_argument(
            "M=2 no es valido con condiciones periodicas de contorno "
            "(el plano referencia la misma celda dos veces cuando se curva)");
    }

    this->particles = &particles;

    this->interactionsBuffer.assign(particlesCount, std::vector<int>());
    this->gridBuffer.assign(static_cast<size_t>(subsquareCount) * subsquareCount,
                             std::vector<Particle*>());
}

long long ParticleSystem::benchmarkOnce(AlgorithmType algorithmType) {
    auto start = std::chrono::high_resolution_clock::now();
    this->start(algorithmType);
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

std::vector<std::vector<int>> ParticleSystem::getInteractions(AlgorithmType algorithmType) {
    switch (algorithmType) {
        case AlgorithmType::BRUTE_FORCE:
            return bruteForce();
        case AlgorithmType::CELL_INDEX_METHOD:
            return cellIndexMethod();
    }
    throw std::invalid_argument("AlgorithmType desconocido");
}

void ParticleSystem::start(AlgorithmType algorithmType) {
    auto startTime = std::chrono::system_clock::now();
    std::vector<std::vector<int>> interactions;
    switch (algorithmType) {
        case AlgorithmType::BRUTE_FORCE:
            interactions = bruteForce();
            break;
        case AlgorithmType::CELL_INDEX_METHOD:
            interactions = cellIndexMethod();
            break;
    }
    auto endTime = std::chrono::system_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Execution time: " << elapsedMs << " ms" << std::endl;

    std::ofstream writer("resources/interactions.txt");
    if (!writer) {
        std::cerr << "Error writing data: no se pudo abrir el archivo" << std::endl;
        return;
    }
    for (size_t key = 0; key < interactions.size(); key++) {
        writer << key << ": [";
        const auto& values = interactions[key];
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) writer << ", ";
            writer << values[i];
        }
        writer << "]\n";
    }
}

/**
 * Lo que hago aca es conseguir el d^2 con (x1-x2)^2 + (y1-x2)^2 en vez de
 * hacer la raiz de lo de la derecha, y se compara con la distancia maxima.
 * Es como hacer dist = sqrt(dx*dx + dy*dy) < maxDist pero sin calcular
 * raices
 */
std::vector<std::vector<int>> ParticleSystem::bruteForce() {
    if (particlesCount != static_cast<int>(particles->size())) {
        throw std::invalid_argument("Hubo un error con la cantidad de particulas");
    }

    for (auto& neighbours : interactionsBuffer) {
        neighbours.clear();
    }

    for (Particle& p : *particles) {
        p.clearNeighbours(); 
    }

    double maxDist = 2 * particlesRadius + interactionRadius;
    double maxDistSq = maxDist * maxDist;

    for (int i = 0; i < particlesCount; i++) {
        double px = (*particles)[i].getXLocation();
        double py = (*particles)[i].getYLocation();

        for (int j = i + 1; j < particlesCount; j++) {
            double qx = (*particles)[j].getXLocation();
            double qy = (*particles)[j].getYLocation();

            double distSq = distanceSquared(px, py, qx, qy);

            if (distSq < maxDistSq) {
                interactionsBuffer[i].push_back(j);
                interactionsBuffer[j].push_back(i);

                (*particles)[i].addNeighbour(&(*particles)[j]);
                (*particles)[j].addNeighbour(&(*particles)[i]);
            }
        }
    }
    return interactionsBuffer;
}

std::vector<std::vector<int>> ParticleSystem::cellIndexMethod() {
    if (particlesCount != static_cast<int>(particles->size())) {
        throw std::invalid_argument("Hubo un error con la cantidad de particulas");
    }

    for (auto& neighbours : interactionsBuffer) {
        neighbours.clear();
    }

    for (auto& cell : gridBuffer) {
        cell.clear();
    }

    for (Particle& p : *particles) {
        p.clearNeighbours();
    }

    for (Particle& p : *particles) {
        int gridX = std::min(static_cast<int>(p.getXLocation() / (static_cast<double>(length) / subsquareCount)),
                              subsquareCount - 1);
        int gridY = std::min(static_cast<int>(p.getYLocation() / (static_cast<double>(length) / subsquareCount)),
                              subsquareCount - 1);
        int gridIndex = gridX + gridY * subsquareCount;

        p.setCellXIndex(gridX);
        p.setCellYIndex(gridY);
        gridBuffer[gridIndex].push_back(&p);
    }

    double maxDist = 2 * particlesRadius + interactionRadius;
    double maxDistSq = maxDist * maxDist;
    double cellSize = static_cast<double>(length) / subsquareCount;

    if (subsquareCount > 1 && cellSize < maxDist) {
        throw std::invalid_argument("subsquareCount too large for given interactionRadius");
    }

    for (auto& cell : gridBuffer) {
        for (Particle* p : cell) {
            int cellX = p->getCellXIndex();
            int cellY = p->getCellYIndex();

            if (subsquareCount == 1) {
                // Unica celda: no hay grids que armar, se compara todo contra todo.
                checkAdjacentAndRecord(*p, 0, 0, maxDistSq);
            } else {
                checkAdjacentAndRecord(*p, cellX, cellY, maxDistSq);
                checkAdjacentAndRecord(*p, cellX, cellY + 1, maxDistSq);
                checkAdjacentAndRecord(*p, cellX + 1, cellY + 1, maxDistSq);
                checkAdjacentAndRecord(*p, cellX + 1, cellY, maxDistSq);
                checkAdjacentAndRecord(*p, cellX + 1, cellY - 1, maxDistSq);
            }
        }
    }

    return interactionsBuffer;
}

void ParticleSystem::checkAdjacentAndRecord(Particle& p, int x, int y, double maxDistSq) {
    int cellX, cellY;

    if (periodicBoundary) {
        cellX = floorMod(x, subsquareCount);
        cellY = floorMod(y, subsquareCount);
    } else {
        if (x < 0 || x >= subsquareCount || y < 0 || y >= subsquareCount) {
            return;
        }
        cellX = x;
        cellY = y;
    }

    int gridIndex = cellX + cellY * subsquareCount;
    for (Particle* other : gridBuffer[gridIndex]) {

        if (p.getCellXIndex() == cellX && p.getCellYIndex() == cellY && other->getId() <= p.getId()) {
            continue;
        }

        double distSq = distanceSquared(p.getXLocation(), p.getYLocation(),
                                         other->getXLocation(), other->getYLocation());

        if (distSq < maxDistSq) {
            interactionsBuffer[p.getId()].push_back(other->getId());
            interactionsBuffer[other->getId()].push_back(p.getId());
            p.addNeighbour(other);
            other->addNeighbour(&p);
        }
    }
}

double ParticleSystem::distanceSquared(double px, double py, double qx, double qy) const {
    double dx = px - qx;
    double dy = py - qy;
    if (periodicBoundary) {
        if (dx > length / 2.0) dx -= length;
        else if (dx < -length / 2.0) dx += length;
        if (dy > length / 2.0) dy -= length;
        else if (dy < -length / 2.0) dy += length;
    }
    return dx * dx + dy * dy;
}

void ParticleSystem::displayBoard() {
    launchPython("scripts/display_board.py",
                 {"resources/static_board_data.txt",
                  "resources/dynamic_board_data.txt"});
}

void ParticleSystem::displayInteractionVisualizer() {
    std::ostringstream radiusStr;
    radiusStr << interactionRadius;
    launchPython("scripts/interaction_visualizer.py",
                 {"resources/static_board_data.txt",
                  "resources/dynamic_board_data.txt",
                  "resources/interactions.txt",
                  "--radius", radiusStr.str()});
}

std::vector<board_generation::Particle>* ParticleSystem::getParticles() {
    return this->particles;
}

void ParticleSystem::setParticles(std::vector<board_generation::Particle>* particles) {
    this->particles = particles;
}

void ParticleSystem::launchPython(const std::string& script, const std::vector<std::string>& args) {
    // Equivalente a lanzar "py <script> <args...>" con ProcessBuilder en
    // Java. Los scripts de Python no se tradujeron, se siguen ejecutando
    // tal cual con el interprete de Python.
    std::ostringstream command;
    command << "python3 \"" << script << "\"";
    for (const auto& arg : args) {
        command << " \"" << arg << "\"";
    }
    int result = std::system(command.str().c_str());
    if (result != 0) {
        std::cerr << "Error launching Python (codigo " << result << ")" << std::endl;
    }
}

} // namespace cell_index_method
