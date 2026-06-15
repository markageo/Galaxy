#ifndef GALAXY_TREE  
#define GALAXY_TREE

#include "Types.h"
#include "Particles.h"

#include <vector>
#include <algorithm>
#include <limits>

namespace GALAXY {

struct Node
{
    floatType mass              = 0.0f;                                 // Total mass of all particles in this subtree
    floatType center[3]         = {0.0f, 0.0f, 0.0f};                   // Geometrical center of this octant
    floatType centerOfMass[3]   = {0.0f, 0.0f, 0.0f};                   
    floatType width             = 0.0f;  
    intType childNodeIndices[8] = {-1, -1, -1, -1, -1, -1, -1, -1};     // Indices of child nodes, -1 if they are empty
    intType leafParticleIdx     = -1;                                   // Index of particle if this is a leaf, -1 if it is not a leaf or empty leaf
    bool isLeaf = true;
};



class Tree
{
    
public:

    std::vector<Node> nodes;
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
        nodes.emplace_back();
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

        auto &rootNode = nodes.front();
        for ( intType i = 0; i != 3; i++ ) {
            const floatType delta = max[i] - min[i];
            rootNode.center[i] = min[i] + delta / 2.0f; 
            rootNode.width     = std::max( rootNode.width, delta);
        }
        rootNode.width *= (1.0f + 1e-3);    // Tolerence so particles are always within bounds
    
    }



    void InsertParticle( intType nodeIdx,
                         intType particleIdx,
                         const Particles &particles,
                         intType recursionDepth )
    {

        // Empty leaf, store body here
        if ( nodes[nodeIdx].isLeaf && nodes[nodeIdx].leafParticleIdx == -1 ) {
            nodes[nodeIdx].leafParticleIdx = particleIdx;
            return ;
        }

        // Occupied leaf, subdivide
        else if ( nodes[nodeIdx].isLeaf ) {

            intType existingParticleIdx = nodes[nodeIdx].leafParticleIdx;
            nodes[nodeIdx].leafParticleIdx = -1;
            nodes[nodeIdx].isLeaf          = false;

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

        intType octant = GetOctant( nodes[nodeIdx], particles.pos[0][particleIdx], particles.pos[1][particleIdx], particles.pos[2][particleIdx] );

        // Create child node if this is unoccupied
        if ( nodes[nodeIdx].childNodeIndices[octant] == -1 ) {

            nodes.emplace_back();
            nodes[nodeIdx].childNodeIndices[octant] = static_cast<intType>( nodes.size() ) - 1;

            SetChildCenterAndWidth(nodeIdx, octant); 

        }

        InsertParticle( nodes[nodeIdx].childNodeIndices[octant], particleIdx, particles, recursionDepth );

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
    intType GetOctant( const Node &node,
                       floatType x,
                       floatType y, 
                       floatType z ) const 
    {
        int octant = 0;

        // Set each bit
        if ( x >= node.center[0] )
            octant |= 1;

        if ( y >= node.center[1] )
            octant |= 2;

        if ( z >= node.center[2] )
            octant |= 4;

        return static_cast<intType>( octant );
    }



    void SetChildCenterAndWidth( intType nodeIdx, 
                                 intType octant )
    {
        const intType childIdx = nodes[nodeIdx].childNodeIndices[octant];
        Node &child = nodes[childIdx];

        child.width = nodes[nodeIdx].width * 0.5f;

        for ( intType i = 0; i != 3; i++ ) {
            child.center[i] = nodes[nodeIdx].center[i] + ( ( octant & (1 << i) ) ?   child.width * 0.5f 
                                                                                 : - child.width * 0.5f );
        }

    }



    void ComputeCenterOfMass( intType nodeIdx, 
                              const Particles &particles )
    {
        Node &node = nodes[ nodeIdx ];

        // Just a single particle 
        if ( node.isLeaf ) {

            // Not an empty leaf
            if ( node.leafParticleIdx >= 0 ) {
                for ( intType i = 0; i != 3; i++ ) {
                    node.centerOfMass[i] = particles.pos[i][ node.leafParticleIdx ]; 
                }
                node.mass = particles.mass[ node.leafParticleIdx ];
            }

            return;
            
        }


        // Recurse through the children
        for ( intType c = 0; c != 8; c++ ) {

            if ( node.childNodeIndices[c] == -1 )
                continue;

            ComputeCenterOfMass( node.childNodeIndices[c], particles );
        }


        // Update the centre of mass contribution of the children
        for ( intType c = 0; c != 8; c++ ) {

            if ( node.childNodeIndices[c] == -1 )
                continue;

            const intType childIdx    = node.childNodeIndices[c];
            const floatType childMass = nodes[childIdx].mass;

            for ( intType i = 0; i != 3; i++ ) {
                node.centerOfMass[i] += childMass * nodes[childIdx].centerOfMass[i]; 
            }
            node.mass += childMass;

        }

        if ( node.mass > 0 ) {
            for ( intType i = 0; i != 3; i++ ) {
                node.centerOfMass[i] /= node.mass; 
            }
        }

    }

};



}   // end namespace GALAXY

#endif  // GALAXY_TREE