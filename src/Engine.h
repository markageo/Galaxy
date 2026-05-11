#ifndef GALAXY_ENGINE  
#define GALAXY_ENGINE

#include "Types.h"
#include "InputData.h"
#include "Particles.h"

#include <cmath>

namespace GALAXY {

class EngineBase {

public:

    virtual ~EngineBase() = default;

    virtual void CopyHostToDevice( const Particles & ) = 0;

    virtual void CopyDeviceToHost( const Particles & ) = 0;

    virtual void Kick() = 0;

    virtual void Drift() = 0;

    virtual void ComputeAccelerations() = 0;

};



class EngineCPU : public EngineBase 
{

Particles &m_particles;
const InputData &m_inputData;

public:

    EngineCPU( Particles &particles,
               const InputData &inputData ) : 
            m_particles(particles),
            m_inputData(inputData)
            {};

    // These are no op on CPU, nothing to copy
    void CopyHostToDevice( const Particles & ) override { /* NULL */ };
    void CopyDeviceToHost( const Particles & ) override { /* NULL */ };



    void ComputeAccelerations() override
    {
        // Brute force
        #pragma omp parallel for
        for ( intType p1 = 0; p1 != m_particles.count; p1++ ) {

            for ( intType i = 0; i != 3; i++ ) {
                m_particles.accel[i][p1] = 0.0f;
            }

            for ( intType p2 = 0; p2 != m_particles.count; p2++ ) {

                // Dont calculate force of particle on itself
                if ( p1 == p2 )
                    continue;

                // Distance between particles
                const floatType R2 = std::pow( m_particles.pos[0][p2] - m_particles.pos[0][p1], 2.0f )
                                   + std::pow( m_particles.pos[1][p2] - m_particles.pos[1][p1], 2.0f )
                                   + std::pow( m_particles.pos[2][p2] - m_particles.pos[2][p1], 2.0f )
                                   + std::pow( m_inputData.softeningLength, 2.0f );

                const floatType R3 = std::pow( R2, 3.0f / 2.0f );

                const floatType K = m_inputData.gravitationalConstant * m_particles.mass[p2];   // Divide out mass of current particle (p1) to get acceleration
            
                for ( intType i = 0; i != 3; i++ ) {
                    m_particles.accel[i][p1] += K * ( m_particles.pos[i][p2] - m_particles.pos[i][p1] ) / R3;
                }

            }
        }
    }



    void Kick() override
    {
        for ( intType i = 0; i != 3; i++ ) {

            #pragma omp parallel for
            for ( intType p = 0; p != m_particles.count; p++ ) {

                m_particles.vel[i][p] += 0.5f * m_particles.accel[i][p] * m_inputData.timeStepSize;

            }
        }
    }



    void Drift() override
    {
        for ( intType i = 0; i != 3; i++ ) {

            #pragma omp parallel for
            for ( intType p = 0; p != m_particles.count; p++ ) {

                m_particles.pos[i][p] += 0.5f * m_particles.vel[i][p] * m_inputData.timeStepSize;

            }
        }
    }

};


}   // end namespace GALAXY

#endif  // GALAXY_ENGINE