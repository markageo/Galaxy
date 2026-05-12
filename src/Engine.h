#ifndef GALAXY_ENGINE  
#define GALAXY_ENGINE

#include "Types.h"
#include "InputData.h"
#include "Particles.h"

#include <memory>

namespace GALAXY {

class EngineBase {

    public:
        virtual ~EngineBase() = default;
        virtual void Initialise() = 0;
        virtual void CopyHostToDevice() = 0;
        virtual void CopyDeviceToHost() = 0;
        virtual void Kick() = 0;
        virtual void Drift() = 0;
        virtual void ComputeAccelerations() = 0;
};

// Factory functions
std::unique_ptr<EngineBase> MakeEngineCPU( Particles &, const InputData & );    // Definition in EngineCPU.cpp
std::unique_ptr<EngineBase> MakeEngineCUDA( Particles &, const InputData & );   // Definition in EngineCUDA.cpp

}   // end namespace GALAXY

#endif  // GALAXY_ENGINE