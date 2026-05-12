/*---------------------------------------------------------------------------*\
   3D Barnes-Hut Galaxy Simulator

   Mark George
\*---------------------------------------------------------------------------*/

#include <iostream>
#include <memory>
#include <omp.h>

#include "Particles.h"
#include "CSVParticleWriter.h"
#include "InputData.h"
#include "InitialConditions.h"
#include "Engine.h"

int main(int argc, char const *argv[])
{
    using namespace GALAXY;

    InputData inputData = ReadInputDataFromCommandLine( argc, argv );

    Particles particles = CreateParticlesHeap( inputData.numberOfInitialParticles );

    SetExponentialDisk( particles, inputData );

    std::unique_ptr<EngineBase> enginePtr;
    switch ( inputData.backend ) {
        case InputData::Backends::OpenMP:
            enginePtr = MakeEngineCPU( particles, inputData );
            break;
        case InputData::Backends::CUDA:
            enginePtr = MakeEngineCUDA( particles, inputData );
            break;
    }
    enginePtr->Initialise();
    
    // Write initial condition to file
    std::string filename = inputData.outputPath + "particles_" + std::to_string(0) + ".csv";
    WriteParticleStateToFile( particles, filename );

    bool writeDuringRun = inputData.outputInterval != 0;

    // Time loop
    std::cout << "Iteration 0 (Written to file)" << std::endl;
    enginePtr->CopyHostToDevice();
    for ( intType n = 1; n <= inputData.numberOfTimeSteps; n++ ) {

        // Propagate particles one timestep
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();
        enginePtr->Drift();
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();

        std::cout << "Iteration " + std::to_string( n );
        
        // Write to file
        const bool outputThisIteration = ( writeDuringRun && ( n % inputData.outputInterval ) == 0 ),
                   isFinalIteration    = n == inputData.numberOfTimeSteps;

        if ( outputThisIteration || isFinalIteration ) {
            enginePtr->CopyDeviceToHost();   // Only copy back to host if we are writing to file

            std::string filename = inputData.outputPath + "particles_" + std::to_string(n) + ".csv";
            WriteParticleStateToFile( particles, filename );
            std::cout << " (Written to file)";

            // Copy back if there is another iteration
            if ( !isFinalIteration )
                enginePtr->CopyHostToDevice();
        }

        std::cout << std::endl;
    }

    FreeParticlesHeap( particles );
    
    return 0;
}