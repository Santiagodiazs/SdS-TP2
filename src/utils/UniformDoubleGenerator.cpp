#include <utils/UniformDoubleGenerator.h>
#include <random>

namespace utils {

namespace {
    std::mt19937 randomEngine{std::random_device{}()};
}

double UniformDoubleGenerator::getUniformDoubleValue(double a, double b) {
    std::uniform_real_distribution<double> dist(a, b);
    return dist(randomEngine);
}

}