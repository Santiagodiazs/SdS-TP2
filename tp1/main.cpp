#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <board_generation/Tablero.h>
#include <cell_index_method/AlgorithmType.h>
#include <cell_index_method/ParticleSystem.h>

using board_generation::Tablero;
using cell_index_method::AlgorithmType;
using cell_index_method::ParticleSystem;

namespace {

constexpr int L_FIXED = 20;
constexpr double RC = 1.0;
constexpr double MAX_RADIUS = 0.26;
constexpr double PACKING_FRACTION = 0.5;
constexpr int DEFAULT_RUNS = 100;
constexpr int DEFAULT_WARMUP_RUNS = 30;

const std::string OUTPUT_M = "resources/variacion_M.csv";
const std::string OUTPUT_N_FREE = "resources/variacion_N_densidad_libre.csv";
const std::string OUTPUT_N_FIXED = "resources/variacion_N_densidad_fija.csv";

struct ResultRowM {
    int n;
    int m;
    std::string algorithm;
    double meanMs;
    double stdMs;
    int runs;
    int maxMAllowed;

    std::string toCsv() const {
        std::ostringstream oss;
        oss << n << "," << m << "," << algorithm << "," << meanMs << "," << stdMs << ","
            << runs << "," << maxMAllowed << "\n";
        return oss.str();
    }
};

struct BestMByN {
    int n;
    int m;
    double meanMs;
};

struct ResultRowN {
    std::string algorithm;
    std::string scenario;
    int n;
    int l;
    int m;
    double meanMs;
    double stdMs;
    int runs;
    double targetDensity;
    double radius;

    std::string toCsv() const {
        std::ostringstream oss;
        oss << algorithm << "," << scenario << "," << n << "," << l << "," << m << "," << meanMs << "," << stdMs << ","
            << runs << "," << targetDensity << "," << radius << "\n";        return oss.str();
    }
};

struct BenchStats {
    double meanMs;
    double stdMs;

    static BenchStats fromSamples(const std::vector<double>& samples) {
        size_t n = samples.size();
        if (n == 0) {
            throw std::invalid_argument("No hay muestras para calcular estadisticas");
        }
        double mean = 0.0;
        for (double value : samples) {
            mean += value;
        }
        mean /= n;

        if (n == 1) {
            return {mean, 0.0};
        }

        double variance = 0.0;
        for (double value : samples) {
            double d = value - mean;
            variance += d * d;
        }
        variance /= (n - 1);
        return {mean, std::sqrt(variance)};
    }
};

int readIntArg(int argc, char* argv[], const std::string& key, int defaultValue) {
    for (int i = 1; i < argc - 1; i++) {
        if (key == argv[i]) {
            return std::stoi(argv[i + 1]);
        }
    }
    return defaultValue;
}

int maxParticlesForLength(int length) {
    return static_cast<int>(std::floor((length * length * PACKING_FRACTION) / (M_PI * MAX_RADIUS * MAX_RADIUS)));
}

int maxMAllowed(int length, double radius, double rc) {
    double cellSizeMin = rc + 2.0 * radius;
    int maxM = static_cast<int>(std::floor(length / cellSizeMin));
    return std::max(maxM, 1);
}

void assertMWithinBounds(int m, int length, double radius, double rc) {
    int maxM = maxMAllowed(length, radius, rc);
    if (m > maxM) {
        std::ostringstream oss;
        oss << "M=" << m << " supera el maximo permitido para L=" << length << ", r=" << radius
            << ", rc=" << rc << ". M_max=" << maxM;
        throw std::invalid_argument(oss.str());
    }
}

int lengthForConstantDensity(int n, double targetDensity, int mToKeepValid) {
    int length = std::max(1, static_cast<int>(std::ceil(std::sqrt(n / targetDensity))));
    while (n > maxParticlesForLength(length) || mToKeepValid > maxMAllowed(length, MAX_RADIUS, RC)) {
        length++;
    }
    return length;
}

std::vector<int> buildNSeries(int minN, int maxN, int points) {
    std::vector<int> values;
    std::set<int> seen;

    auto addValue = [&](int v) {
        if (seen.insert(v).second) {
            values.push_back(v);
        }
    };

    addValue(minN);
    if (points <= 2) {
        addValue(maxN);
        return values;
    }

    for (int i = 0; i < points; i++) {
        double t = i / static_cast<double>(points - 1);
        int n = static_cast<int>(std::round(minN + t * (maxN - minN)));
        addValue(std::max(minN, std::min(maxN, n)));
    }
    addValue(maxN);
    return values;
}

AlgorithmType algorithmForM(int m) {
    return m == 1 ? AlgorithmType::BRUTE_FORCE : AlgorithmType::CELL_INDEX_METHOD;
}

BenchStats measure(ParticleSystem& particleSystem, AlgorithmType algorithmType, int runs, int warmupRuns) {
    for (int i = 0; i < warmupRuns; i++) {
        particleSystem.benchmarkOnce(algorithmType);
    }

    std::vector<double> sampleMs;
    sampleMs.reserve(runs);
    for (int i = 0; i < runs; i++) {
        long long elapsedNs = particleSystem.benchmarkOnce(algorithmType);
        sampleMs.push_back(elapsedNs / 1'000'000.0);
    }
    return BenchStats::fromSamples(sampleMs);
}

void writeVariationMCsv(const std::vector<ResultRowM>& rows, const std::vector<BestMByN>& bestByN,
                         int bestMGlobal) {
    std::ofstream writer(OUTPUT_M);
    if (!writer) {
        throw std::runtime_error("No se pudo abrir " + OUTPUT_M);
    }
    writer << "N,M,algorithm,mean_ms,std_ms,runs,max_m_allowed\n";
    for (const auto& row : rows) {
        writer << row.toCsv();
    }
    writer << "# best_m_global=" << bestMGlobal << "\n";
    for (const auto& b : bestByN) {
        writer << "# best_m_for_n_" << b.n << "=" << b.m << ",mean_ms=" << b.meanMs << "\n";
    }
}

void writeVariationNCsv(const std::string& outputPath, const std::vector<ResultRowN>& rows) {
    std::ofstream writer(outputPath);
    if (!writer) {
        throw std::runtime_error("No se pudo abrir " + outputPath);
    }
    writer << "algorithm,scenario,N,L,M,mean_ms,std_ms,runs,target_density,radius\n";
    for (const auto& row : rows) {
        writer << row.toCsv();
    }
}

int chooseGlobalBestM(const std::vector<ResultRowM>& rows, const std::vector<int>& nValues, int maxCommonM) {
    double bestMean = std::numeric_limits<double>::infinity();
    int bestM = 1;
    for (int m = 1; m <= maxCommonM; m++) {
        if (m == 2) {
            continue;
        }
        double acc = 0.0;
        int count = 0;
        for (int n : nValues) {
            for (const auto& row : rows) {
                if (row.n == n && row.m == m) {
                    acc += row.meanMs;
                    count++;
                    break;
                }
            }
        }
        if (count == static_cast<int>(nValues.size())) {
            double meanAcrossN = acc / count;
            if (meanAcrossN < bestMean) {
                bestMean = meanAcrossN;
                bestM = m;
            }
        }
    }
    return bestM;
}

int runVariationM(int maxNFixedL, int mediumN, int highN, int runs, int warmupRuns) {
    std::vector<int> nValues = {mediumN, highN};
    int mGlobalLimit = std::numeric_limits<int>::max();
    std::vector<ResultRowM> rows;
    std::vector<BestMByN> bestByN;

    for (int n : nValues) {
        Tablero tablero(L_FIXED, n);
        int maxM = maxMAllowed(L_FIXED, tablero.getParticleRadius(), RC);
        mGlobalLimit = std::min(mGlobalLimit, maxM);

        double bestMean = std::numeric_limits<double>::infinity();
        int bestM = 1;

        for (int m = 1; m <= maxM; m++) {
            // Con borde periódico M=2 duplica celdas al curvar la grilla.
            if (m == 2) {
                continue;
            }
            AlgorithmType algorithm = (m == 1) ? AlgorithmType::BRUTE_FORCE : AlgorithmType::CELL_INDEX_METHOD;
            ParticleSystem system(tablero.getParticles(), tablero.getParticleRadius(), n, L_FIXED, m, RC, true);

            BenchStats stats = measure(system, algorithm, runs, warmupRuns);
            rows.push_back({n, m, algorithm == AlgorithmType::BRUTE_FORCE ? "BRUTE_FORCE" : "CELL_INDEX_METHOD",
                             stats.meanMs, stats.stdMs, runs, maxM});

            if (stats.meanMs < bestMean) {
                bestMean = stats.meanMs;
                bestM = m;
            }
        }
        bestByN.push_back({n, bestM, bestMean});
    }

    int bestMGlobal = chooseGlobalBestM(rows, nValues, mGlobalLimit);
    writeVariationMCsv(rows, bestByN, bestMGlobal);
    return bestMGlobal;
}

void runVariationN(int bestM, int maxNFixedL, int runs, int warmupRuns) {
    std::vector<int> nValues = buildNSeries(10, maxNFixedL, 12);
    int nIntermediate = nValues[nValues.size() / 2];
    double density = nIntermediate / static_cast<double>(L_FIXED * L_FIXED);

    std::vector<ResultRowN> freeDensityRows;
    std::vector<ResultRowN> fixedDensityRows;

for (int n : nValues) {
        // --- ESCENARIO: DENSIDAD LIBRE ---
        Tablero freeTablero(L_FIXED, n);
        assertMWithinBounds(bestM, L_FIXED, freeTablero.getParticleRadius(), RC);
        
        // 1. Medir Cell Index Method (M óptimo)
        ParticleSystem freeSystemCIM(freeTablero.getParticles(), freeTablero.getParticleRadius(), n, L_FIXED, bestM, RC, true);
        BenchStats freeStatsCIM = measure(freeSystemCIM, AlgorithmType::CELL_INDEX_METHOD, runs, warmupRuns);
        freeDensityRows.push_back({"CELL_INDEX_METHOD", "densidad_libre", n, L_FIXED, bestM, freeStatsCIM.meanMs, freeStatsCIM.stdMs, runs, density, freeTablero.getParticleRadius()});

        // 2. Medir Brute Force (M = 1 obligatoriamente)
        ParticleSystem freeSystemBF(freeTablero.getParticles(), freeTablero.getParticleRadius(), n, L_FIXED, 1, RC, true);
        BenchStats freeStatsBF = measure(freeSystemBF, AlgorithmType::BRUTE_FORCE, runs, warmupRuns);
        freeDensityRows.push_back({"BRUTE_FORCE", "densidad_libre", n, L_FIXED, 1, freeStatsBF.meanMs, freeStatsBF.stdMs, runs, density, freeTablero.getParticleRadius()});


        // --- ESCENARIO: DENSIDAD FIJA ---
        int lConstDensity = lengthForConstantDensity(n, density, bestM);
        Tablero fixedTablero(lConstDensity, n);
        assertMWithinBounds(bestM, lConstDensity, fixedTablero.getParticleRadius(), RC);
        
        // 1. Medir Cell Index Method (M óptimo)
        ParticleSystem fixedSystemCIM(fixedTablero.getParticles(), fixedTablero.getParticleRadius(), n, lConstDensity, bestM, RC, true);
        BenchStats fixedStatsCIM = measure(fixedSystemCIM, AlgorithmType::CELL_INDEX_METHOD, runs, warmupRuns);
        fixedDensityRows.push_back({"CELL_INDEX_METHOD", "densidad_fija", n, lConstDensity, bestM, fixedStatsCIM.meanMs, fixedStatsCIM.stdMs, runs, density, fixedTablero.getParticleRadius()});

        // 2. Medir Brute Force (M = 1 obligatoriamente)
        ParticleSystem fixedSystemBF(fixedTablero.getParticles(), fixedTablero.getParticleRadius(), n, lConstDensity, 1, RC, true);
        BenchStats fixedStatsBF = measure(fixedSystemBF, AlgorithmType::BRUTE_FORCE, runs, warmupRuns);
        fixedDensityRows.push_back({"BRUTE_FORCE", "densidad_fija", n, lConstDensity, 1, fixedStatsBF.meanMs, fixedStatsBF.stdMs, runs, density, fixedTablero.getParticleRadius()});
    }
    
    writeVariationNCsv(OUTPUT_N_FREE, freeDensityRows);
    writeVariationNCsv(OUTPUT_N_FIXED, fixedDensityRows);
}

} // namespace

int main(int argc, char* argv[]) {
    int runs = readIntArg(argc, argv, "--runs", DEFAULT_RUNS);
    int warmupRuns = readIntArg(argc, argv, "--warmup", DEFAULT_WARMUP_RUNS);
    if (runs < 1) {
        throw std::invalid_argument("--runs debe ser >= 1");
    }
    if (warmupRuns < 0) {
        throw std::invalid_argument("--warmup debe ser >= 0");
    }

    int maxNFixedL = maxParticlesForLength(L_FIXED);
    int mediumN = std::max(10, maxNFixedL / 2);
    int highN = maxNFixedL;

    int bestM = runVariationM(maxNFixedL, mediumN, highN, runs, warmupRuns);
    runVariationN(bestM, maxNFixedL, runs, warmupRuns);

    std::printf("Benchmark completado. M optimo=%d, N_max(L=20)=%d, runs=%d, warmup=%d\n",
                bestM, maxNFixedL, runs, warmupRuns);
    std::cout << "CSV generados:" << std::endl;
    std::cout << " - " << OUTPUT_M << std::endl;
    std::cout << " - " << OUTPUT_N_FREE << std::endl;
    std::cout << " - " << OUTPUT_N_FIXED << std::endl;

    return 0;
}
