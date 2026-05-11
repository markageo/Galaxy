#ifndef GALAXY_INITIAL_CONDITIONS 
#define GALAXY_INITIAL_CONDITIONS

#include "Particles.h"
#include "InputData.h"

namespace GALAXY
{

Particles RandomStatioaryParticles( const InputData &, 
                                    const floatType, 
                                    const floatType );

}

#endif  // end GALAXY_INITIAL_CONDITIONS