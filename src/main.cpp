/*---------------------------------------------------------------------------*\
   3D Barnes-Hut Galaxy Simulator

   Mark George
\*---------------------------------------------------------------------------*/

#include <iostream>
#include <memory>
#include <functional>
#include <omp.h>

#include "Particles.h"
#include "CSVParticleStateWriter.h"
#if HAS_HDF5
    #include "HDF5ParticleStateWriter.h"
#endif
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
            #ifdef USE_CUDA
                enginePtr = MakeEngineCUDA( particles, inputData );
            #endif
            break;
    }
    enginePtr->Initialise();
    
    // Set particle state function and write initial condition to file
    std::function<void(const Particles &, const std::string &, intType )> WriteParticleStateToFile;
    switch ( inputData.stateFileFormat ) {
        case InputData::StateFileFormats::CSV:
            WriteParticleStateToFile = WriteParticleStateToCSVFile;
            break;
        case InputData::StateFileFormats::HDF5:
            #ifdef HAS_HDF5
                WriteParticleStateToFile = WriteParticleStateToHDF5File;
            #endif
            break;
    }

    std::string filename = inputData.outputPath + "particles_" + std::to_string(0);
    WriteParticleStateToFile( particles, filename, 0 );
        

    bool writeDuringRun = inputData.outputInterval != 0;

    // Time loop
    std::cout << "Iteration 0 (Written to file)" << std::endl;
    enginePtr->CopyHostToDevice();
    enginePtr->ComputeAccelerations();
    for ( intType n = 1; n <= inputData.numberOfTimeSteps; n++ ) {

        // Propagate particles one timestep
        enginePtr->Kick();
        enginePtr->Drift();
        enginePtr->ComputeAccelerations();
        enginePtr->Kick();
        enginePtr->Synchronise();

        std::cout << "Iteration " + std::to_string( n );
        
        // Write to file
        const bool outputThisIteration = ( writeDuringRun && ( n % inputData.outputInterval ) == 0 ),
                   isFinalIteration    = n == inputData.numberOfTimeSteps;

        if ( outputThisIteration || isFinalIteration ) {
            
            enginePtr->CopyDeviceToHost();   // Only copy back to host if we are writing to file

            std::string filename = inputData.outputPath + "particles_" + std::to_string(n);
            WriteParticleStateToFile( particles, filename, n );
            std::cout << " (Written to file)";
        }

        std::cout << std::endl;
    }

    FreeParticlesHeap( particles );
    
    return 0;
}