#ifndef GALAXY_INPUT_DATA
#define GALAXY_INPUT_DATA

#include "Types.h"

#include <string>

namespace GALAXY  
{

struct InputData 
{
    intType numberOfInitialParticles;
    floatType diskRadius, 
              diskCutoffRadius, 
              diskThickness, 
              diskMass, 
              toomreStabilityParameter, 
              haloMass,
              haloScaleRadius,
              gravitationalConstant, 
              softeningLength,
              maxOpeningAngle;
    intType numberOfTimeSteps;
    floatType timeStepSize;

    enum class ForceAlgorithms {
        AllPairs, BarnesHut
    };
    ForceAlgorithms forceAlgorithm;

    enum class Backends {
        CUDA, OpenMP
    };
    Backends backend;

    std::string outputPath;
    intType outputInterval;
};

InputData ReadInputDataFromCommandLine( int , char const ** );

}   // end namespace GALAXY

#endif  // GALAXY_INPUT_DATA