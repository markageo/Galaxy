#ifndef GALAXY_INITIAL_CONDITIONS 
#define GALAXY_INITIAL_CONDITIONS

#include "Particles.h"
#include "InputData.h"

namespace GALAXY
{

Particles RandomStationaryParticles( const InputData &, 
                                     const floatType, 
                                     const floatType );


Particles ExponentialDisk( const InputData & );

}

#endif  // end GALAXY_INITIAL_CONDITIONS