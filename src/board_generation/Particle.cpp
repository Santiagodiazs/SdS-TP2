#include <board_generation/Particle.h>

#include <cmath>
#include <random>
#include <sstream>

namespace board_generation {

namespace {
    thread_local std::mt19937 velocityRng{std::random_device{}()};
}

Particle::Particle(int id, double x, double y, double particleRadius)
    : id(id), particleRadius(particleRadius), colour("Red"), x(x), y(y) {

    double speed = 0.03;
    std::uniform_real_distribution<double> angleDist(0.0, 2 * M_PI);
    double theta = angleDist(velocityRng);
    this->xVelocity = speed * std::cos(theta);
    this->yVelocity = speed * std::sin(theta);
    this->angle = theta;
}

Particle::Particle(int id, double x, double y)
    : id(id), colour("Red"), x(x), y(y), particleRadius(0.0) {
    
    double speed = 0.03;
    std::uniform_real_distribution<double> angleDist(0.0, 2 * M_PI);
    double theta = angleDist(velocityRng);
    this->xVelocity = speed * std::cos(theta);
    this->yVelocity = speed * std::sin(theta);
    this->angle = theta;   
}

int Particle::getId() const { return id; }

double Particle::getXLocation() const { return x; }
void Particle::setXLocation(double x) { this->x = x; }

double Particle::getYLocation() const { return y; }
void Particle::setYLocation(double y) { this->y = y; }

std::string Particle::getColour() const { return colour; }

double Particle::getXVelocity() const { return xVelocity; }
void Particle::setXVelocity(double xVelocity) { this->xVelocity = xVelocity; }

double Particle::getYVelocity() const { return yVelocity; }
void Particle::setYVelocity(double yVelocity) { this->yVelocity = yVelocity; }

void Particle::updateVelocityFromAngle(double angle) {
    this->xVelocity = 0.03 * std::cos(angle);
    this->yVelocity = 0.03 * std::sin(angle);
}

void Particle::setCellXIndex(int cellXIndex) { this->cellXIndex = cellXIndex; }
int Particle::getCellXIndex() const { return cellXIndex; }

void Particle::setCellYIndex(int cellYIndex) { this->cellYIndex = cellYIndex; }
int Particle::getCellYIndex() const { return cellYIndex; }

void Particle::setAngle(double angle) { this->angle = angle; }
double Particle::getAngle() const { return angle; }

double Particle::getParticleRadius() const { return particleRadius; }

const std::vector<Particle*>& Particle::getNeighbours() const { return neighbours; }
void Particle::setNeighbours(std::vector<Particle*> neighbours) { this->neighbours = std::move(neighbours); }
void Particle::addNeighbour(Particle* neighbour) { this->neighbours.push_back(neighbour); }
void Particle::clearNeighbours() { this->neighbours.clear(); }

std::string Particle::toString() const {
    std::ostringstream oss;
    oss << x << " " << y << " " << xVelocity << " " << yVelocity;
    return oss.str();
}

bool Particle::operator==(const Particle& other) const {
    return id == other.id;
}

} // namespace board_generation
