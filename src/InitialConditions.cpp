#include "Types.h"
#include "InitialConditions.h"

#include <random>

namespace GALAXY
{

Particles RandomStatioaryParticles( const InputData &inputData,
                                    const floatType positionLowerBound,
                                    const floatType positionUpperBound )
{

    Particles particles;
    particles.Reserve( inputData.numberOfInitialParticles );

    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<floatType> distr(positionLowerBound, positionUpperBound); 

    // Initialise particles at random positions on the square [positionLowerBound, positionUpperBound]^3
    for ( intType p = 0; p != inputData.numberOfInitialParticles; p++ ) {

        particles.AddParticleBack();

        for ( intType i = 0; i != 3; i++ ) {
            particles.pos[i].back()    = distr(gen);
            particles.vel[i].back()    = 0.0f;
            particles.accel[i].back()  = 0.0f;
        }
        particles.mass.back() = inputData.particleMass;

    }

    return particles;
}

}