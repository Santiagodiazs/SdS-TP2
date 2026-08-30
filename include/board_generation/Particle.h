#pragma once

#include <string>
#include <vector>

namespace board_generation {

class Particle {
public:
    Particle(int id, double x, double y, double particleRadius);
    Particle(int id, double x, double y);

    int getId() const;

    double getXLocation() const;
    void setXLocation(double x);

    double getYLocation() const;
    void setYLocation(double y);

    std::string getColour() const;

    double getXVelocity() const;
    void setXVelocity(double xVelocity);

    double getYVelocity() const;
    void setYVelocity(double yVelocity);

    void updateVelocityFromAngle(double angle);

    void setCellXIndex(int cellXIndex);
    int getCellXIndex() const;

    void setCellYIndex(int cellYIndex);
    int getCellYIndex() const;

    void setAngle(double angle);
    double getAngle() const;

    double getParticleRadius() const;

    const std::vector<Particle*>& getNeighbours() const;
    void setNeighbours(std::vector<Particle*> neighbours);
    void addNeighbour(Particle* neighbour);
    void clearNeighbours();
    
    std::string toString() const;

    bool operator==(const Particle& other) const;

private:
    int id;
    double particleRadius;
    std::string colour;
    double x;
    double y;
    double xVelocity;
    double yVelocity;
    double angle;
    int cellXIndex = 0;
    int cellYIndex = 0;
    std::vector<Particle*> neighbours;
};

}