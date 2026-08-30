#include <utils/UniformIntGenerator.h>

#include <random>

namespace utils {

namespace {
    std::mt19937 randomEngine{std::random_device{}()};
}

int UniformIntGenerator::getUniformIntValue(int a, int b) {
    std::uniform_int_distribution<int> dist(a, b);
    return dist(randomEngine);
}

} // namespace board_generation