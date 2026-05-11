#ifndef GALAXY_INPUT_DATA
#define GALAXY_INPUT_DATA

#include "Types.h"

#include <string>

namespace GALAXY  
{

struct InputData 
{
    intType numberOfInitialParticles;
    floatType particleMass;
    floatType gravitationalConstant;
    floatType softeningLength;
    intType numberOfTimeSteps;
    floatType timeStepSize;
    std::string outputPath;
};

InputData ReadInputDataFromCommandLine( int , char const ** );

}   // end namespace GALAXY

#endif  // GALAXY_INPUT_DATA