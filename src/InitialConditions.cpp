#include "Types.h"
#include "InitialConditions.h"

#include <random>
#include <algorithm>

namespace GALAXY
{

Particles RandomStationaryParticles( const InputData &inputData,
                                     const floatType positionLowerBound,
                                     const floatType positionUpperBound )
{

    Particles particles;
    particles.Reserve( inputData.numberOfInitialParticles );

    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<floatType> distr(positionLowerBound, positionUpperBound); 

    // Initialise particles at random positions on the square [positionLowerBound, positionUpperBound]^3
    for ( intType p = 0; p != inputData.numberOfInitialParticles; p++ ) {

        particles.AddParticleBack();

        for ( intType i = 0; i != 3; i++ ) {
            particles.pos[i].back()    = distr(gen);
            particles.vel[i].back()    = 0.0f;
            particles.accel[i].back()  = 0.0f;
        }
        particles.mass.back() = 1.0f;

    }

    return particles;
}



namespace 
{

// Sample a radius point for exponential surface density using rejection sampling
floatType SampleExponentialRadius( floatType diskRadius, 
                                   floatType diskCutoffRadius,
                                   std::mt19937 &gen, 
                                   std::uniform_real_distribution<floatType> &distr )
{

    while (true) {

        const floatType r = diskCutoffRadius * distr(gen);
        const floatType u = distr(gen);
        const floatType p = ( r / diskRadius ) * std::exp( - r / diskRadius ) 
                          / std::exp( - 1.0f ); // normalisation
        if ( u < p )
            return r;
    }

}


floatType GetCircularDiskVelocity( floatType r,
                                   floatType diskCentralSurfaceDensity,
                                   floatType diskRadius, 
                                   floatType gravitationalConstant )
{
    const floatType y  = std::max( 1e-6, r / ( 2.0f * diskRadius ) );   // Clip to avoid zero value at zero
    const floatType v2 = 4.0f * M_PI * gravitationalConstant * diskCentralSurfaceDensity * diskRadius 
                       * y * y 
                       * ( std::cyl_bessel_i( 0, y ) * std::cyl_bessel_k( 0, y )  -  std::cyl_bessel_i( 1, y ) * std::cyl_bessel_k( 1, y ) );
    return sqrt(v2);
}



}   // end namespace anonymous



Particles ExponentialDisk( const InputData &inputData )
{
    // Exponential disk with surface density profile:
    // sigma = sigma0 * exp( r / Rd )
    // sigma  - disk surface density profile
    // sigma0 - disk surface density at R = 0
    // r      - radial coordinate
    // Rd     - disk radius

    Particles particles;
    particles.Reserve( inputData.numberOfInitialParticles );

    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<floatType> distr(0.0f, 1.0f); 

    // Mass of each particle, comes from integral of disk surface density profile 
    const floatType totalDiskMass = inputData.diskCentralSurfaceDensity * 2.0f * M_PI * std::pow( inputData.diskRadius, 2.0f ),
                    particleMass = totalDiskMass / inputData.numberOfInitialParticles;

    for ( intType p = 0; p != inputData.numberOfInitialParticles; p++ ) {

        particles.AddParticleBack();

        // Sample radius
        const floatType r = SampleExponentialRadius( inputData.diskRadius, inputData.diskCutoffRadius, gen, distr );

        // Random azimuth
        const floatType theta = 2.0f * M_PI * distr(gen);

        // Vertical position from sech^2 profile, the CDF is a tanh
        // Argument is random number in (-1, 1), with some tolerence since tanh blows up a +/- 1.
        const floatType tol = 1e-6;
        const floatType z = 2.0f * inputData.diskThickness * std::tanh( 2.0f * (1.0f - tol) * ( distr(gen) - 0.5f ) );    

        particles.pos[0].back()    = r * std::cos( theta );
        particles.pos[1].back()    = r * std::sin( theta );
        particles.pos[2].back()    = z;

        const floatType velocityMagnitude = GetCircularDiskVelocity( r, inputData.diskCentralSurfaceDensity, inputData.diskRadius, inputData.gravitationalConstant );

        particles.vel[0].back()    = - velocityMagnitude * std::sin( theta );
        particles.vel[1].back()    =   velocityMagnitude * std::cos( theta );
        particles.vel[2].back()    = 0.0f;


        // Zero acceleration
        for ( intType i = 0; i != 3; i++ ) {
            particles.accel[i].back()  = 0.0f;
        }
        

        particles.mass.back() = particleMass;

    }

    return particles;

}

}