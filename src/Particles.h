#ifndef GALAXY_PARTICLES
#define GALAXY_PARTICLES

#include "Types.h"
#include <vector>
#include <array>

namespace GALAXY 
{

struct Particles
{
    Particles() : count( 0 ) {};

    void Reserve( const intType n ) 
    {
        for ( intType i = 0; i != 3; i++ ) {
            pos[i].reserve(n);
            vel[i].reserve(n);
            accel[i].reserve(n);
        }
        mass.reserve(n);
    }

    void AddParticleBack()
    {
        for ( intType i = 0; i != 3; i++ ) {
            pos[i].push_back( 0.0f );
            vel[i].push_back( 0.0f );
            accel[i].push_back( 0.0f );
        }
        mass.push_back( 0.0f );
        count++;
    }

    intType count;
    std::array< std::vector<floatType>, 3 > pos;
    std::array< std::vector<floatType>, 3 > vel;
    std::array< std::vector<floatType>, 3 > accel;
    std::vector<floatType> mass;
};


}   // end namespace GALAXY

#endif  // GALAXY_PARTICLES