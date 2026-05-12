#ifndef GALAXY_PARTICLES
#define GALAXY_PARTICLES

#include "Types.h"

namespace GALAXY 
{

struct Particles
{
    intType count;
    floatType* pos[3];
    floatType* vel[3];
    floatType* accel[3];
    floatType* mass;
};


inline Particles CreateParticlesHeap( intType n )
{
    Particles particles;
    
    particles.count = n;
    for ( intType i = 0; i != 3; i++ ) {
        particles.pos[i]   = new floatType[n];
        particles.vel[i]   = new floatType[n];
        particles.accel[i] = new floatType[n];
    }
    particles.mass = new floatType[n];

    return particles;
}


inline void FreeParticlesHeap( Particles &particles )
{
    particles.count = 0;
    for ( intType i = 0; i != 3; i++ ) {
        delete[] particles.pos[i];
        delete[] particles.vel[i];
        delete[] particles.accel[i];
    }
    delete[] particles.mass;
}


}   // end namespace GALAXY

#endif  // GALAXY_PARTICLES