#ifndef GALAXY_TREE  
#define GALAXY_TREE

#include "Types.h"
#include "Particles.h"

#include <vector>
#include <algorithm>
#include <limits>

namespace GALAXY {


// For use on device. uses raw pointers, intended to be copied into from host.
struct DeviceNodes
{
    floatType *mass             = nullptr;                       // Total mass of all particles in this subtree
    floatType *centerOfMass[3]  = {nullptr, nullptr, nullptr};   // Geometrical center of this octant
    floatType *width            = nullptr;
    intType   *childNodeIndices = nullptr;                       // Indices of child nodes, -1 if they are empty, has size 8 * count, indexed with [nodeIdx * 8 + child].
    intType   *leafParticleIdx  = nullptr;                       // Index of particle if this is a leaf, -1 if it is not a leaf or empty leaf
    intType   *isLeaf           = nullptr;                       // 1 = leaf, 0 = internal
    intType    count            = 0;
    intType    allocCount       = 0;                             // Size of allocated arrays, can be larger than the number of nodes
};



// For use on host. Uses std::vector which makes creating tree simpler.
struct Nodes
{
    std::vector<floatType> mass;                  // Total mass of all particles in this subtree
    std::vector<floatType> center[3];             // Geometrical center of this octant
    std::vector<floatType> centerOfMass[3];                   
    std::vector<floatType> width;  
    std::vector<intType> childNodeIndices;        // Indices of child nodes, -1 if they are empty, has size 8 * count, indexed with [nodeIdx * 8 + child].
    std::vector<intType> leafParticleIdx;         // Index of particle if this is a leaf, -1 if it is not a leaf or empty leaf
    std::vector<intType> isLeaf;                  // 1 = leaf, 0 = internal
    intType count = 0;

    void addNode()
    {
        mass.push_back( 0.0f );
        for ( intType i = 0; i < 3; i++ ) {
            center[i].push_back( 0.0f );
            centerOfMass[i].push_back( 0.0f );
        }
        width.push_back( 0.0f );
        for ( intType i = 0; i < 8; i++ ) {
            childNodeIndices.push_back( -1 );
        }
        leafParticleIdx.push_back( -1 );
        isLeaf.push_back( 1 );
        count++;
    }

    void clear()
    {
        mass.clear();
        for ( intType i = 0; i < 3; i++ ) {
            center[i].clear();
            centerOfMass[i].clear();
        }
        width.clear();
        childNodeIndices.clear();
        leafParticleIdx.clear();
        isLeaf.clear();
        count = 0;
    }

    void reserve( size_t n )
    {
        mass.reserve( n );
        for ( intType i = 0; i < 3; i++ ) {
            center[i].reserve( n );
            centerOfMass[i].reserve( n );
        }
        width.reserve( n );
        childNodeIndices.reserve( 8 * n );
        leafParticleIdx.reserve( n );
        isLeaf.reserve( n );
        count = 0;
    }

};



class Tree
{
    
public:

    Nodes nodes;
    intType nParticles;


    // Builds (or rebuilds) tree from particles 
    void Build( const Particles &particles )
    {
        nodes.clear();
        nParticles = particles.count;

        if ( nParticles == 0 )
            return;

        nodes.reserve( 8 * nParticles );

        // Root node
        nodes.addNode();
        SetRootCenterAndWidth( particles );
        
        // Recursively add all particles
        for ( intType p = 0; p != nParticles; p++ ) {
            InsertParticle(0, p, particles, 0);
        }

        // Compute the center of mass of the nodes
        ComputeCenterOfMass( 0, particles );

    }

private:

    void SetRootCenterAndWidth( const Particles &particles )
    {
        // Get the particle position bounds
        floatType min[3] = {std::numeric_limits<floatType>::max(), 
                            std::numeric_limits<floatType>::max(), 
                            std::numeric_limits<floatType>::max()}, 
                  max[3] = {std::numeric_limits<floatType>::lowest(), 
                            std::numeric_limits<floatType>::lowest(), 
                            std::numeric_limits<floatType>::lowest()}; 

        for ( intType p = 0; p != nParticles; p++ ) {
            for ( intType i = 0; i != 3; i++ ) {
                max[i] = std::max( max[i], particles.pos[i][p] );
                min[i] = std::min( min[i], particles.pos[i][p] );
            }
        }

        for ( intType i = 0; i != 3; i++ ) {
            const floatType delta = max[i] - min[i];
            nodes.center[i][0] = min[i] + delta / 2.0f; 
            nodes.width[0]     = std::max( nodes.width[0], delta);
        }
        nodes.width[0] *= (1.0f + 1e-3f);    // Tolerence so particles are always within bounds
    
    }



    void InsertParticle( intType nodeIdx,
                         intType particleIdx,
                         const Particles &particles,
                         intType recursionDepth )
    {

        // Empty leaf, store body here
        if ( nodes.isLeaf[nodeIdx] && nodes.leafParticleIdx[nodeIdx] == -1 ) {
            nodes.leafParticleIdx[nodeIdx] = particleIdx;
            return ;
        }

        // Occupied leaf, subdivide
        else if ( nodes.isLeaf[nodeIdx] ) {

            intType existingParticleIdx = nodes.leafParticleIdx[nodeIdx];
            nodes.leafParticleIdx[nodeIdx] = -1;
            nodes.isLeaf[nodeIdx]          = 0;

            InsertParticleIntoChild( nodeIdx, existingParticleIdx, particles, recursionDepth + 1 );
            InsertParticleIntoChild( nodeIdx, particleIdx        , particles, recursionDepth + 1 );

        }

        // Internal node, recursive insertion
        else {

            InsertParticleIntoChild( nodeIdx, particleIdx, particles, recursionDepth + 1 );

        } 

    }



    void InsertParticleIntoChild( intType nodeIdx,
                                  intType particleIdx,
                                  const Particles &particles,
                                  intType recursionDepth )
    {

        intType octant = GetOctant( nodeIdx, particles.pos[0][particleIdx], particles.pos[1][particleIdx], particles.pos[2][particleIdx] );

        // Create child node if this is unoccupied
        if ( nodes.childNodeIndices[8*nodeIdx  + octant] == -1 ) {

            nodes.addNode();
            nodes.childNodeIndices[8*nodeIdx + octant] = static_cast<intType>( nodes.count ) - 1;

            SetChildCenterAndWidth(nodeIdx, octant); 

        }

        InsertParticle( nodes.childNodeIndices[8*nodeIdx + octant], particleIdx, particles, recursionDepth );

    }



    // Returns 0 - 7 depending on which octant of the node's bounding box the given point lies in.
    // Each of the first three bits represent a coordinate, with 0 being the negative side, 
    // and 1 being the positive side i.e.
    // Bit 0 represents the x coordinate,
    // Bit 1 represents the y coordinate,
    // Bit 2 represents the z coordinate.
    // The coordiantes are then:
    // octant 0: -x, -y, -z (000)
    // octant 1: +x, -y, -z (100)
    // octant 2: -x, +y, -z (010)
    // octant 3: +x, +y, -z (110)
    // octant 4: -x, -y, +z (001)
    // octant 5: +x, -y, +z (101)
    // octant 6: -x, +y, +z (011)
    // octant 7: +x, +y, +z (111)
    intType GetOctant( intType   nodeIdx,
                       floatType x,
                       floatType y, 
                       floatType z ) const 
    {
        int octant = 0;

        // Set each bit
        if ( x >= nodes.center[0][nodeIdx] )
            octant |= 1;

        if ( y >= nodes.center[1][nodeIdx] )
            octant |= 2;

        if ( z >= nodes.center[2][nodeIdx] )
            octant |= 4;

        return static_cast<intType>( octant );
    }



    void SetChildCenterAndWidth( intType nodeIdx, 
                                 intType octant )
    {
        const intType childIdx = nodes.childNodeIndices[8*nodeIdx + octant];

        nodes.width[childIdx] = nodes.width[nodeIdx] * 0.5f;

        for ( intType i = 0; i != 3; i++ ) {
            nodes.center[i][childIdx] = nodes.center[i][nodeIdx] + ( ( octant & (1 << i) ) ?   nodes.width[childIdx] * 0.5f 
                                                                                           : - nodes.width[childIdx] * 0.5f );
        }

    }



    void ComputeCenterOfMass( intType nodeIdx, 
                              const Particles &particles )
    {

        // Just a single particle 
        if ( nodes.isLeaf[nodeIdx] ) {

            // Not an empty leaf
            if ( nodes.leafParticleIdx[nodeIdx] >= 0 ) {
                for ( intType i = 0; i != 3; i++ ) {
                    nodes.centerOfMass[i][nodeIdx] = particles.pos[i][ nodes.leafParticleIdx[nodeIdx] ]; 
                }
                nodes.mass[nodeIdx] = particles.mass[ nodes.leafParticleIdx[nodeIdx] ];
            }

            return;
            
        }


        // Recurse through the children
        for ( intType c = 0; c != 8; c++ ) {

            if ( nodes.childNodeIndices[8*nodeIdx + c] == -1 )
                continue;

            ComputeCenterOfMass( nodes.childNodeIndices[8*nodeIdx + c], particles );
        }


        // Update the center of mass contribution of the children
        for ( intType c = 0; c != 8; c++ ) {

            if ( nodes.childNodeIndices[8*nodeIdx + c] == -1 )
                continue;

            const intType childIdx    = nodes.childNodeIndices[8*nodeIdx + c];
            const floatType childMass = nodes.mass[childIdx];

            for ( intType i = 0; i != 3; i++ ) {
                nodes.centerOfMass[i][nodeIdx] += childMass * nodes.centerOfMass[i][childIdx]; 
            }
            nodes.mass[nodeIdx] += childMass;

        }

        if ( nodes.mass[nodeIdx] > 0 ) {
            for ( intType i = 0; i != 3; i++ ) {
                nodes.centerOfMass[i][nodeIdx] /= nodes.mass[nodeIdx]; 
            }
        }

    }

};



}   // end namespace GALAXY

#endif  // GALAXY_TREE