/*---------------------------------------------------------------------------*\
   3D Barnes-Hut Galaxy Simulator

   Mark George
\*---------------------------------------------------------------------------*/

#include <iostream>
#include <memory>

#include "Particles.h"
#include "CSVParticleWriter.h"
#include "InputData.h"
#include "InitialConditions.h"
#include "Engine.h"

int main(int argc, char const *argv[])
{
    using namespace GALAXY;

    InputData inputData = ReadInputDataFromCommandLine( argc, argv );

    Particles particles = ExponentialDisk( inputData );

    std::unique_ptr<EngineBase> enginePtr = std::make_unique<EngineCPU>( particles, inputData );

    
    // Write initial condition to file
    std::string filename = inputData.outputPath + "particles_" + std::to_string(0) + ".csv";
    WriteParticleStateToFile( particles, filename );

    // Time loop
    std::cout << "Iteration 0 (Written to file)" << std::endl;
    for ( intType n = 1; n <= inputData.numberOfTimeSteps; n++ ) {

        // Propagate particles one timestep
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();
        enginePtr->Drift();
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();

        std::cout << "Iteration " + std::to_string( n );
        
        // Write to file
        bool outputThisIteration =  ( n % inputData.outputInterval ) == 0
                                 || n == inputData.numberOfTimeSteps;
        if ( outputThisIteration ) {
            std::string filename = inputData.outputPath + "particles_" + std::to_string(n) + ".csv";
            WriteParticleStateToFile( particles, filename );
            std::cout << " (Written to file)";
        }

        std::cout << std::endl;
    }

    
    return 0;
}