#ifndef GALAXY_INITIAL_CONDITIONS 
#define GALAXY_INITIAL_CONDITIONS

#include "Particles.h"
#include "InputData.h"

namespace GALAXY
{

void SetRandomStationaryParticles( Particles &,
                                   const InputData &, 
                                   const floatType, 
                                   const floatType );


void SetExponentialDisk( Particles &, 
                         const InputData & );

}

#endif  // end GALAXY_INITIAL_CONDITIONS