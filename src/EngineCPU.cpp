#include "Engine.h"

#include <cmath>

namespace GALAXY {


class EngineCPU : public EngineBase 
{

    Particles &m_particles;
    const InputData &m_inputData;

    public:

        EngineCPU( Particles &,
                   const InputData & );
        void Initialise() override;
        void CopyHostToDevice() override;
        void CopyDeviceToHost() override;
        void ComputeAccelerations() override;
        void Kick() override;
        void Drift() override;

};



std::unique_ptr<EngineBase> MakeEngineCPU( Particles &particles, 
                                           const InputData &inputData ) 
{ 
    return std::make_unique<EngineCPU>( particles, inputData ); 
}



EngineCPU::EngineCPU( Particles &particles,
                      const InputData &inputData ) : 
        m_particles(particles),
        m_inputData(inputData)
        {};


// These are no op on CPU, nothing to allocate or copy
void EngineCPU::Initialise() { /* NULL */ };
void EngineCPU::CopyHostToDevice() { /* NULL */ };
void EngineCPU::CopyDeviceToHost() { /* NULL */ };


void EngineCPU::ComputeAccelerations()
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

            const floatType K = m_inputData.gravitationalConstant * m_particles.mass[p2] / R3;   // Divide out mass of current particle (p1) to get acceleration
        
            for ( intType i = 0; i != 3; i++ ) {
                m_particles.accel[i][p1] += K * ( m_particles.pos[i][p2] - m_particles.pos[i][p1] );
            }

        }
    }
}



void EngineCPU::Kick()
{
    for ( intType i = 0; i != 3; i++ ) {

        #pragma omp parallel for
        for ( intType p = 0; p != m_particles.count; p++ ) {

            m_particles.vel[i][p] += 0.5f * m_particles.accel[i][p] * m_inputData.timeStepSize;

        }
    }
}



void EngineCPU::Drift()
{
    for ( intType i = 0; i != 3; i++ ) {

        #pragma omp parallel for
        for ( intType p = 0; p != m_particles.count; p++ ) {

            m_particles.pos[i][p] += m_particles.vel[i][p] * m_inputData.timeStepSize;

        }
    }
}



}   // end namespace GALAXY

