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

    Particles particles = RandomStatioaryParticles( inputData, -1.0f, 1.0f );

    std::unique_ptr<EngineBase> enginePtr = std::make_unique<EngineCPU>( particles, inputData );

    
    // Write initial condition to file
    std::string filename = inputData.outputPath + "particles_" + std::to_string(0) + ".csv";
    WriteParticleStateToFile( particles, filename );

    // Time loop
    for ( intType n = 1; n <= inputData.numberOfTimeSteps; n++ ) {

        // Propagate particles one timestep
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();
        enginePtr->Drift();
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();
        
        // Write to file
        std::string filename = inputData.outputPath + "particles_" + std::to_string(n) + ".csv";
        WriteParticleStateToFile( particles, filename );

    }

    
    return 0;
}