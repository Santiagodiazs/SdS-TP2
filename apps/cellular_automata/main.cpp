#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <stdexcept>

#include <cellular_automata/CellularAutomataSystem.h>
#include <cellular_automata/UpdateRule.h>
#include <cellular_automata/VicsekRule.h>
#include <cellular_automata/VoterRule.h>
#include <cell_index_method/AlgorithmType.h>

namespace {

void printUsage(const char* programName) {
    std::cerr << "Uso: " << programName << " [opciones]\n"
              << "  --model <vicsek|voter>     Regla de actualizacion (default: vicsek)\n"
              << "  --length <L>               Lado de la caja (default: 10)\n"
              << "  --density <rho>            Densidad de particulas (usa esto O --particles, no ambos)\n"
              << "  --particles <N>            Cantidad de particulas (usa esto O --density)\n"
              << "  --radius <rc>              Radio de interaccion (default: 1.0)\n"
              << "  --noise <eta>              Amplitud del ruido (default: 0.1)\n"
              << "  --steps <n>                Cantidad de pasos de simulacion (default: 1000)\n"
              << "  --seed <s>                 (No usado todavia, ver nota en el codigo)\n"
              << "  --output <prefijo>         Prefijo de los archivos de salida (default: resources/)\n"
              << "  --no-frames                No guarda frames; util para barridos largos\n"
              << "  --benchmark                Mide tiempos de CIM en vez de correr la simulacion\n"
              << "  --benchmark-algorithm <cim|brute>  Algoritmo a medir (default: cim)\n"
              << "  --repetitions <n>          Cantidad de repeticiones del benchmark (default: 30)\n"
              << std::endl;
}

// Busca un flag tipo "--nombre valor" en argv y devuelve "valor" como string.
// Si no esta presente, devuelve defaultValue.
std::string getArgOrDefault(int argc, char** argv, const std::string& flag, const std::string& defaultValue) {
    for (int i = 1; i < argc - 1; i++) {
        if (flag == argv[i]) {
            return std::string(argv[i + 1]);
        }
    }
    return defaultValue;
}

bool hasArg(int argc, char** argv, const std::string& flag) {
    for (int i = 1; i < argc; i++) {
        if (flag == argv[i]) return true;
    }
    return false;
}

} 

int main(int argc, char** argv) {
    if (hasArg(argc, argv, "--help") || hasArg(argc, argv, "-h")) {
        printUsage(argv[0]);
        return 0;
    }

    try {
        std::string outputPrefix = getArgOrDefault(argc, argv, "--output", "resources/");
        std::string modelName = getArgOrDefault(argc, argv, "--model", "vicsek");
        int length = std::stoi(getArgOrDefault(argc, argv, "--length", "10"));
        double interactionRadius = std::stod(getArgOrDefault(argc, argv, "--radius", "1.0"));
        double noise = std::stod(getArgOrDefault(argc, argv, "--noise", "0.1"));
        int steps = std::stoi(getArgOrDefault(argc, argv, "--steps", "1000"));

        // particleCount se puede pasar directo, o derivar de una densidad rho = N / L^2.
        int particleCount;
        if (hasArg(argc, argv, "--particles")) {
            particleCount = std::stoi(getArgOrDefault(argc, argv, "--particles", "0"));
        } else {
            double density = std::stod(getArgOrDefault(argc, argv, "--density", "2.0"));
            particleCount = static_cast<int>(density * length * length);
        }

        std::unique_ptr<cellular_automata::UpdateRule> updateRule;
        if (modelName == "vicsek") {
            updateRule = std::make_unique<cellular_automata::VicsekRule>();
        } else if (modelName == "voter") {
            updateRule = std::make_unique<cellular_automata::VoterRule>();
        } else {
            std::cerr << "Modelo desconocido: " << modelName << " (usar 'vicsek' o 'voter')" << std::endl;
            return 1;
        }

        if (hasArg(argc, argv, "--benchmark")) {
            int repetitions = std::stoi(getArgOrDefault(argc, argv, "--repetitions", "30"));
            std::string benchmarkPath = getArgOrDefault(argc, argv, "--output", "resources/cim_benchmark.csv");
            std::string benchmarkAlgorithm = getArgOrDefault(argc, argv, "--benchmark-algorithm", "cim");
            cell_index_method::AlgorithmType algorithmType;
            std::string algorithmLabel;
            if (benchmarkAlgorithm == "cim") {
                algorithmType = cell_index_method::AlgorithmType::CELL_INDEX_METHOD;
                algorithmLabel = "CIM";
            } else if (benchmarkAlgorithm == "brute") {
                algorithmType = cell_index_method::AlgorithmType::BRUTE_FORCE;
                algorithmLabel = "BRUTE_FORCE";
            } else {
                throw std::invalid_argument("--benchmark-algorithm debe ser 'cim' o 'brute'");
            }

            std::ifstream existingBenchmark(benchmarkPath);
            const bool needsHeader = !existingBenchmark ||
                existingBenchmark.peek() == std::ifstream::traits_type::eof();
            std::ofstream out(benchmarkPath, std::ios::app);
            if (needsHeader) {
                out << "N,length,radius,algorithm,rep,time_ns\n";
            }

            for (int rep = 0; rep < repetitions; rep++) {
                cellular_automata::CellularAutomataSystem benchSystem(
                    length, particleCount, interactionRadius, noise, steps,
                    updateRule.get(), /*periodicBoundary=*/true
                );
                long long ns = benchSystem.benchmark(algorithmType);
                out << particleCount << "," << length << "," << interactionRadius << ","
                    << algorithmLabel << "," << rep << "," << ns << "\n";
            }

            std::cout << "Benchmark " << algorithmLabel << ": " << repetitions << " repeticiones para N=" << particleCount
                    << " guardadas en " << benchmarkPath << std::endl;
            return 0;
        }

        std::cout << "Corriendo simulacion: modelo=" << modelName
                  << " L=" << length
                  << " N=" << particleCount
                  << " rc=" << interactionRadius
                  << " eta=" << noise
                  << " steps=" << steps << std::endl;

        cellular_automata::CellularAutomataSystem system(
            length,
            particleCount,
            interactionRadius,
            noise,
            steps,
            updateRule.get(),
            /*periodicBoundary=*/true
        );

        system.run(steps, outputPrefix, !hasArg(argc, argv, "--no-frames"));

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
