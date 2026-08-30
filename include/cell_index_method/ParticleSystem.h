#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <cell_index_method/AlgorithmType.h>
#include <board_generation/Particle.h>

namespace cell_index_method {

class ParticleSystem {
public:
    // Equivalente al constructor de un solo argumento de Java: crea un
    // sistema "vacio" que administra su propio vector de particulas.
    explicit ParticleSystem(double interactionRadius);

    ParticleSystem(std::vector<board_generation::Particle>& particles,
                   double particlesRadius,
                   int particlesCount,
                   int length,
                   int subsquareCount,
                   double interactionRadius,
                   bool periodicBoundary);

    // Devuelve el tiempo transcurrido en nanosegundos (equivalente a
    // System.nanoTime() en Java).
    long long benchmarkOnce(AlgorithmType algorithmType);

    void start(AlgorithmType algorithmType);

    std::vector<std::vector<int>> getInteractions(AlgorithmType algorithmType);

    void displayBoard();
    void displayInteractionVisualizer();

    std::vector<board_generation::Particle>* getParticles();
    void setParticles(std::vector<board_generation::Particle>* particles);
private:
    int particlesCount;
    int length;
    int subsquareCount;
    double interactionRadius;
    double particlesRadius;
    bool periodicBoundary;

    // Puntero al vector de particulas "propietario" (Tablero u otro), o al
    // vector interno cuando se usa el constructor de un solo argumento.
    std::vector<board_generation::Particle>* particles;
    std::vector<board_generation::Particle> ownedParticles;

    // Buffers reutilizados entre llamadas a benchmarkOnce() para no medir
    // el costo de asignar memoria en cada iteracion del benchmark: eso
    // agrega ruido y ademas no forma parte del algoritmo en si, sino de la
    // forma en que lo estamos midiendo.
    // (Map<Integer, List<Integer>> en Java -> vector indexado por id, ya
    // que los ids son 0..particlesCount-1)
    std::vector<std::vector<int>> interactionsBuffer;
    std::vector<std::vector<board_generation::Particle*>> gridBuffer;

    std::vector<std::vector<int>> bruteForce();
    std::vector<std::vector<int>> cellIndexMethod();
    void checkAdjacentAndRecord(board_generation::Particle& p, int x, int y, double maxDistSq);

    /**
     * Para condiciones periodicas a veces es mas corto cruzar el borde que
     * ver la distancia directa, entonces por eso se compara la posicion con
     * la length/2
     */
    double distanceSquared(double px, double py, double qx, double qy) const;

    void launchPython(const std::string& script, const std::vector<std::string>& args);
};

}