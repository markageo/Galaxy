#include "Engine.h"
#include "Tree.h"

#include <cuda.h>
#include <cstring>
#include <stdexcept>

#define CUDA_CHECK(call)                                                    \
    do {                                                                    \
        cudaError_t _e = (call);                                            \
        if (_e != cudaSuccess) {                                            \
            fprintf(stderr, "CUDA error %s:%d  %s\n",                       \
                    __FILE__, __LINE__, cudaGetErrorString(_e));            \
            std::abort();                                                   \
        }                                                                   \
    } while (0)



namespace GALAXY {

constexpr int BLOCK_SIZE = 256;


class EngineCUDA : public EngineBase 
{
    Particles &m_particles,
               m_particlesPinned,
               m_particlesDevice;
    const InputData &m_inputData;
    Tree m_tree;
    DeviceNodes m_treeNodesDevice;
    void (EngineCUDA::*m_accelerationFunctionPtr)();

    cudaStream_t m_stream;

    public:

        EngineCUDA( Particles &,
                    const InputData & );
        
        ~EngineCUDA() override;

        void Initialise() override;
        void CopyHostToDevice() override;
        void CopyDeviceToHost() override;
        void ComputeAccelerations() override;
        void Kick() override;
        void Drift() override;
        void Synchronise() override;

    private: 

        void AllocateDeviceMemory();
        void AllocateHostPinnedMemory();
        void FreeMemory();

        void ReallocateTreeNodesOnDevice();
        void CopyTreeNodesToDevice();
        void CopyOnlyPosAndMassDeviceToHost();

        void ComputeAccelerationsAllPairs();
        void ComputeAccelerationsBarnesHut();
};



std::unique_ptr<EngineBase> MakeEngineCUDA( Particles &particles, 
                                            const InputData &inputData ) 
{ 
    return std::make_unique<EngineCUDA>( particles, inputData ); 
}



EngineCUDA::EngineCUDA( Particles &particles,
                        const InputData &inputData ) : 
        m_particles(particles),
        m_inputData(inputData)
        {
            switch ( inputData.forceAlgorithm ) {
                case InputData::ForceAlgorithms::AllPairs:
                    m_accelerationFunctionPtr = &EngineCUDA::ComputeAccelerationsAllPairs;
                    break;

                case InputData::ForceAlgorithms::BarnesHut:
                    m_accelerationFunctionPtr = &EngineCUDA::ComputeAccelerationsBarnesHut;
                    break;
            }

            m_particlesDevice.count = m_particles.count;
            m_particlesPinned.count = m_particles.count;
        };



EngineCUDA::~EngineCUDA()
{
    FreeMemory();
}



void EngineCUDA::Initialise()
{
    int deviceCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&deviceCount));

    if (deviceCount == 0) 
        throw std::runtime_error("No CUDA device found");

    CUDA_CHECK(cudaSetDevice(0));

    CUDA_CHECK(cudaStreamCreate(&m_stream));

    AllocateDeviceMemory();
    AllocateHostPinnedMemory();
}



void EngineCUDA::Synchronise()
{
    CUDA_CHECK(cudaStreamSynchronize(m_stream));
} 



void EngineCUDA::AllocateDeviceMemory()
{
    const intType bytes = m_particles.count * sizeof(floatType);

    for ( intType i = 0; i != 3; i++ ) {
        CUDA_CHECK(cudaMalloc(&m_particlesDevice.pos[i]  , bytes));
        CUDA_CHECK(cudaMalloc(&m_particlesDevice.vel[i]  , bytes));
        CUDA_CHECK(cudaMalloc(&m_particlesDevice.accel[i], bytes));
    }
    CUDA_CHECK(cudaMalloc(&m_particlesDevice.mass, bytes));
}



void EngineCUDA::AllocateHostPinnedMemory()
{
    const intType bytes = m_particles.count * sizeof(floatType);

    for ( intType i = 0; i != 3; i++ ) {
        CUDA_CHECK(cudaMallocHost(&m_particlesPinned.pos[i]  , bytes));
        CUDA_CHECK(cudaMallocHost(&m_particlesPinned.vel[i]  , bytes));
        CUDA_CHECK(cudaMallocHost(&m_particlesPinned.accel[i], bytes));
    }
    CUDA_CHECK(cudaMallocHost(&m_particlesPinned.mass, bytes));
}



void EngineCUDA::FreeMemory()
{
    // Pinned host memory
    for ( intType i = 0; i != 3; i++ ) {
        cudaFreeHost(m_particlesPinned.pos[i]);
        cudaFreeHost(m_particlesPinned.vel[i]);
        cudaFreeHost(m_particlesPinned.accel[i]);
    }
    cudaFreeHost(m_particlesPinned.mass);
    m_particlesPinned.count = 0;

    // Device memory
    for ( intType i = 0; i != 3; i++ ) {
        cudaFree(m_particlesDevice.pos[i]);
        cudaFree(m_particlesDevice.vel[i]);
        cudaFree(m_particlesDevice.accel[i]);
    }
    cudaFree(m_particlesDevice.mass);
    m_particlesDevice.count = 0;

    cudaFree(m_treeNodesDevice.mass);
    for ( intType i = 0; i < 3; i++ ) {
        cudaFree(m_treeNodesDevice.centerOfMass[i]);
    }
    cudaFree(m_treeNodesDevice.width);
    cudaFree(m_treeNodesDevice.childNodeIndices);
    cudaFree(m_treeNodesDevice.leafParticleIdx);
    cudaFree(m_treeNodesDevice.isLeaf);
    m_treeNodesDevice.count = 0;
    m_treeNodesDevice.allocCount = 0;
}



void EngineCUDA::CopyHostToDevice()
{
    const intType bytes = m_particles.count * sizeof(floatType);

    // Copy heap memory into pinned buffers
    for ( intType i = 0; i != 3; i++ ) {
        std::memcpy(m_particlesPinned.pos[i]  , m_particles.pos[i]  , bytes);
        std::memcpy(m_particlesPinned.vel[i]  , m_particles.vel[i]  , bytes);
        std::memcpy(m_particlesPinned.accel[i], m_particles.accel[i], bytes);
    }
    std::memcpy(m_particlesPinned.mass, m_particles.mass, bytes);

    // Copy to device
    for ( intType i = 0; i != 3; i++ ) {
        CUDA_CHECK(cudaMemcpyAsync(m_particlesDevice.pos[i]  , m_particlesPinned.pos[i]  , bytes, cudaMemcpyHostToDevice, m_stream));
        CUDA_CHECK(cudaMemcpyAsync(m_particlesDevice.vel[i]  , m_particlesPinned.vel[i]  , bytes, cudaMemcpyHostToDevice, m_stream));
        CUDA_CHECK(cudaMemcpyAsync(m_particlesDevice.accel[i], m_particlesPinned.accel[i], bytes, cudaMemcpyHostToDevice, m_stream));
    }
    CUDA_CHECK(cudaMemcpyAsync(m_particlesDevice.mass, m_particlesPinned.mass, bytes, cudaMemcpyHostToDevice, m_stream));
}


void EngineCUDA::CopyDeviceToHost()
{
    CUDA_CHECK(cudaStreamSynchronize(m_stream));
    const intType bytes = m_particles.count * sizeof(floatType);

    // Copy to host pinned memory
    for ( intType i = 0; i != 3; i++ ) {
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.pos[i]  , m_particlesDevice.pos[i]  , bytes, cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.vel[i]  , m_particlesDevice.vel[i]  , bytes, cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.accel[i], m_particlesDevice.accel[i], bytes, cudaMemcpyDeviceToHost));
    }
    CUDA_CHECK(cudaMemcpy(m_particlesPinned.mass, m_particlesDevice.mass, bytes, cudaMemcpyDeviceToHost));


    // Copy from pinned memory to heap
    for ( intType i = 0; i != 3; i++ ) {
        std::memcpy(m_particles.pos[i]  , m_particlesPinned.pos[i]  , bytes);
        std::memcpy(m_particles.vel[i]  , m_particlesPinned.vel[i]  , bytes);
        std::memcpy(m_particles.accel[i], m_particlesPinned.accel[i], bytes);
    }
    std::memcpy(m_particles.mass, m_particlesPinned.mass, bytes);
}



void EngineCUDA::CopyOnlyPosAndMassDeviceToHost()
{
    CUDA_CHECK(cudaStreamSynchronize(m_stream));
    const intType bytes = m_particles.count * sizeof(floatType);

    // Copy to host pinned memory
    for ( intType i = 0; i != 3; i++ ) {
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.pos[i]  , m_particlesDevice.pos[i]  , bytes, cudaMemcpyDeviceToHost));
    }
    CUDA_CHECK(cudaMemcpy(m_particlesPinned.mass, m_particlesDevice.mass, bytes, cudaMemcpyDeviceToHost));


    // Copy from pinned memory to heap
    for ( intType i = 0; i != 3; i++ ) {
        std::memcpy(m_particles.pos[i]  , m_particlesPinned.pos[i]  , bytes);
    }
    std::memcpy(m_particles.mass, m_particlesPinned.mass, bytes);
}



void EngineCUDA::ComputeAccelerations()
{ (this->*m_accelerationFunctionPtr)(); }



__device__ void ComputeHernquistHalo_kernel( floatType &axi,
                                             floatType &ayi,
                                             floatType &azi,
                                             floatType xi,
                                             floatType yi,
                                             floatType zi,
                                             floatType gravitationalConstant, 
                                             floatType haloMass, 
                                             floatType haloScaleRadius )
{

    const floatType R = sqrt( xi*xi + yi*yi + zi*zi );
    const floatType K = - gravitationalConstant * haloMass 
                        / ( ( R + haloScaleRadius ) * ( R + haloScaleRadius ) * R );
    
    axi += K * xi;
    ayi += K * yi;
    azi += K * zi;
}



__global__ void ComputeAccelerationsAllPairs_kernel( floatType* __restrict__ ax,
                                                     floatType* __restrict__ ay,
                                                     floatType* __restrict__ az,
                                                     const floatType* __restrict__ x,
                                                     const floatType* __restrict__ y,
                                                     const floatType* __restrict__ z,
                                                     const floatType* __restrict__ mass,
                                                     floatType gravitationalConstant, 
                                                     floatType softeningLength,
                                                     floatType haloMass, 
                                                     floatType haloScaleRadius,
                                                     intType nParticles )
{
    // Shared particle data for a given tile, size is dynamically allocated
    extern __shared__ floatType shared[];
    floatType* xTile    = shared;
    floatType* yTile    = shared + blockDim.x;
    floatType* zTile    = shared + blockDim.x * 2;
    floatType* massTile = shared + blockDim.x * 3;

    const intType p1 = blockDim.x * blockIdx.x + threadIdx.x;
    const bool p1Valid = p1 < nParticles;

    // Temporary for accumialting
    floatType axTemp = 0.0f,
              ayTemp = 0.0f,
              azTemp = 0.0f;

    const floatType softeningLength2 = softeningLength * softeningLength;

    const intType nTiles = ( nParticles + blockDim.x - 1 ) / blockDim.x;

    // Loop through tiles
    for ( intType tile = 0; tile != nTiles; tile++ ) {

        // Fill the shared memory, each thread in the block loads a single particle
        const intType p2 = tile * blockDim.x + threadIdx.x;
        if ( p2 < nParticles ) {
            xTile[threadIdx.x]    = x[p2];
            yTile[threadIdx.x]    = y[p2];
            zTile[threadIdx.x]    = z[p2];
            massTile[threadIdx.x] = mass[p2];
        } else {
            xTile[threadIdx.x]    = 0.0f;
            yTile[threadIdx.x]    = 0.0f;
            zTile[threadIdx.x]    = 0.0f;
            massTile[threadIdx.x] = 0.0f;
        }

        __syncthreads();

        // Loop through each element in a tile
        if ( p1Valid ) {

            for ( intType i = 0; i != blockDim.x; i++ ) {

                const intType p2Global = tile * blockDim.x + i;
                if ( p2Global >= nParticles )
                    continue;

                if ( p2Global == p1 )
                    continue;

                const floatType dx = xTile[i] - x[p1],
                                dy = yTile[i] - y[p1],
                                dz = zTile[i] - z[p1];

                const floatType R2 = dx*dx + dy*dy + dz*dz + softeningLength2;
                const floatType R3 = R2 * sqrt( R2 );
                const floatType K  = gravitationalConstant * massTile[i] / R3;

                axTemp += K * dx;
                ayTemp += K * dy;
                azTemp += K * dz;

            }

        }

        __syncthreads();

    }

    if ( p1Valid ) {

        ComputeHernquistHalo_kernel( axTemp,
                                     ayTemp, 
                                     azTemp, 
                                     x[p1],
                                     y[p1], 
                                     z[p1],
                                     gravitationalConstant,
                                     haloMass, 
                                     haloScaleRadius );
        
        ax[p1] = axTemp;
        ay[p1] = ayTemp;
        az[p1] = azTemp;
    }

}



void EngineCUDA::ComputeAccelerationsAllPairs()
{
    const int blocks = (m_particlesDevice.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    const int sharedMemSize = 4 * BLOCK_SIZE * sizeof(floatType);
    ComputeAccelerationsAllPairs_kernel<<<blocks, BLOCK_SIZE, sharedMemSize, m_stream>>>
    ( 
        m_particlesDevice.accel[0], 
        m_particlesDevice.accel[1],
        m_particlesDevice.accel[2],
        m_particlesDevice.pos[0],
        m_particlesDevice.pos[1],
        m_particlesDevice.pos[2],
        m_particlesDevice.mass,
        m_inputData.gravitationalConstant,
        m_inputData.softeningLength,
        m_inputData.haloMass,
        m_inputData.haloScaleRadius,
        m_particlesDevice.count
    );

    CUDA_CHECK(cudaGetLastError());
}



void EngineCUDA::ReallocateTreeNodesOnDevice()
{
    // Only reallocate if device buffer is too small
    if ( m_treeNodesDevice.allocCount >= m_tree.nodes.count )
        return;

    // Free the old memory
    cudaFree(m_treeNodesDevice.mass);
    for ( intType i = 0; i != 3; i++ ) {
        cudaFree(m_treeNodesDevice.centerOfMass[i]);
    }
    cudaFree(m_treeNodesDevice.width);
    cudaFree(m_treeNodesDevice.childNodeIndices);
    cudaFree(m_treeNodesDevice.leafParticleIdx);
    cudaFree(m_treeNodesDevice.isLeaf);
    m_treeNodesDevice.allocCount = 0;

    // Reset pointers to null
    m_treeNodesDevice.mass = nullptr;
    for ( intType i = 0; i != 3; i++ ) {
        m_treeNodesDevice.centerOfMass[i] = nullptr;
    }
    m_treeNodesDevice.width            = nullptr;
    m_treeNodesDevice.childNodeIndices = nullptr;
    m_treeNodesDevice.leafParticleIdx  = nullptr;
    m_treeNodesDevice.isLeaf           = nullptr;

    // Allocate an extra 25% memory to avoid potential reallocations later
    intType allocCount = m_tree.nodes.count + m_tree.nodes.count / 4;

    CUDA_CHECK( cudaMalloc(&m_treeNodesDevice.mass  , allocCount * sizeof(floatType)) );
    for ( intType i = 0; i < 3; i++ ) {
        CUDA_CHECK( cudaMalloc(&m_treeNodesDevice.centerOfMass[i]  , allocCount * sizeof(floatType)) );
    }
    CUDA_CHECK( cudaMalloc(&m_treeNodesDevice.width             , allocCount   * sizeof(floatType)) );
    CUDA_CHECK( cudaMalloc(&m_treeNodesDevice.childNodeIndices  , 8*allocCount * sizeof(intType)) );
    CUDA_CHECK( cudaMalloc(&m_treeNodesDevice.leafParticleIdx , allocCount   * sizeof(intType)) );
    CUDA_CHECK( cudaMalloc(&m_treeNodesDevice.isLeaf            , allocCount   * sizeof(intType)) );

    m_treeNodesDevice.allocCount = allocCount;
}



void EngineCUDA::CopyTreeNodesToDevice()
{
    m_treeNodesDevice.count = m_tree.nodes.count;

    // Synchronous copy for now
    const intType count = m_treeNodesDevice.count;
    CUDA_CHECK( cudaMemcpy( m_treeNodesDevice.mass   , m_tree.nodes.mass.data() , count * sizeof(floatType), cudaMemcpyHostToDevice ) );
    for ( intType i = 0; i < 3; i++ ) {
        CUDA_CHECK( cudaMemcpy( m_treeNodesDevice.centerOfMass[i], m_tree.nodes.centerOfMass[i].data(), count * sizeof(floatType), cudaMemcpyHostToDevice ) );
    }
    CUDA_CHECK( cudaMemcpy( m_treeNodesDevice.width           , m_tree.nodes.width.data()           , count   * sizeof(floatType), cudaMemcpyHostToDevice ) );
    CUDA_CHECK( cudaMemcpy( m_treeNodesDevice.childNodeIndices, m_tree.nodes.childNodeIndices.data(), 8*count * sizeof(intType)  , cudaMemcpyHostToDevice ) );
    CUDA_CHECK( cudaMemcpy( m_treeNodesDevice.leafParticleIdx , m_tree.nodes.leafParticleIdx.data() , count   * sizeof(intType)  , cudaMemcpyHostToDevice ) );
    CUDA_CHECK( cudaMemcpy( m_treeNodesDevice.isLeaf          , m_tree.nodes.isLeaf.data()          , count   * sizeof(intType)  , cudaMemcpyHostToDevice ) );
}



__device__ void TraverseBarnesHutTree_kernel( // Particle data
                                              intType thisParticleIdx,
                                              floatType &axi,
                                              floatType &ayi,
                                              floatType &azi,
                                              floatType xi,
                                              floatType yi,
                                              floatType zi,
                                              floatType gravitationalConstant, 
                                              floatType softeningLength,
                                              intType nParticles,

                                              // Tree data
                                              const floatType* __restrict__ node_mass,
                                              const floatType* __restrict__ node_comx,
                                              const floatType* __restrict__ node_comy,
                                              const floatType* __restrict__ node_comz,
                                              const floatType* __restrict__ node_width,
                                              const intType*   __restrict__ node_childNodeIndices,
                                              const intType*   __restrict__ node_leafParticleIdx,
                                              const intType*   __restrict__ node_isLeaf,
                                              floatType maxOpeningAngle,
                                              intType nNodes )
{


    // Add nodes to a stack, which contains indices of nodes
    int stack[128];
    int top = 0;       // Index in stack for the top element
    stack[top] = 0;   // Root node has index zero
    top++;

    const floatType softeningLength2 = softeningLength * softeningLength;

    while ( top > 0 ) {

        top--;
        int nodeIdx = stack[top];

        const floatType nodeWidth = node_width[nodeIdx];
        const floatType dx = node_comx[nodeIdx] - xi,
                        dy = node_comy[nodeIdx] - yi,
                        dz = node_comz[nodeIdx] - zi;
        const floatType nodeDistance2 = dx*dx + dy*dy + dz*dz;
        const floatType theta = nodeWidth / sqrt( nodeDistance2 );

        // Calculate critetion
        const bool calculateForceOnThisNode = node_isLeaf[nodeIdx]
                                           || theta < maxOpeningAngle;

        if ( calculateForceOnThisNode ) {

            // Avoid force calcuation with self
            if ( !( node_isLeaf[nodeIdx] && node_leafParticleIdx[nodeIdx] == thisParticleIdx ) ) {

                const floatType R2 = nodeDistance2 + softeningLength2;
                const floatType R3 = R2 * sqrt( R2 );
                const floatType K  = gravitationalConstant * node_mass[nodeIdx] / R3;

                axi += K * dx;
                ayi += K * dy;
                azi += K * dz;
            }

        } else {

            // Push children to the stack
            for ( intType c = 0; c < 8; c++ ) {
                int childIdx = node_childNodeIndices[ 8*nodeIdx + c ];
                if ( childIdx >= 0 ) {
                    stack[top] = childIdx;
                    top++;
                }
            }

        }

    }


}



__global__ void ComputeAccelerationsBarnesHut_kernel( // Particle data
                                                      floatType* __restrict__ ax,
                                                      floatType* __restrict__ ay,
                                                      floatType* __restrict__ az,
                                                      const floatType* __restrict__ x,
                                                      const floatType* __restrict__ y,
                                                      const floatType* __restrict__ z,
                                                      floatType gravitationalConstant, 
                                                      floatType softeningLength,
                                                      floatType haloMass, 
                                                      floatType haloScaleRadius,
                                                      intType nParticles,

                                                      // Tree data
                                                      const floatType* __restrict__ node_mass,
                                                      const floatType* __restrict__ node_comx,
                                                      const floatType* __restrict__ node_comy,
                                                      const floatType* __restrict__ node_comz,
                                                      const floatType* __restrict__ node_width,
                                                      const intType*   __restrict__ node_childNodeIndices,
                                                      const intType*   __restrict__ node_leafParticleIdx,
                                                      const intType*   __restrict__ node_isLeaf,
                                                      floatType maxOpeningAngle,
                                                      intType nNodes )
{

    const int i = threadIdx.x + blockDim.x * blockIdx.x;

    if ( i >= nParticles )
        return;

    floatType axi = 0.0f, 
              ayi = 0.0f,
              azi = 0.0f;

    TraverseBarnesHutTree_kernel
    (
        i,
        axi, 
        ayi,
        azi,
        x[i],
        y[i], 
        z[i],
        gravitationalConstant,
        softeningLength,
        nParticles,

        node_mass,
        node_comx,
        node_comy,
        node_comz,
        node_width,
        node_childNodeIndices,
        node_leafParticleIdx,
        node_isLeaf,
        maxOpeningAngle,
        nNodes
    );

    // Add acceleration due to Hernquist halo
    ComputeHernquistHalo_kernel( axi,
                                 ayi, 
                                 azi, 
                                 x[i],
                                 y[i], 
                                 z[i],
                                 gravitationalConstant,
                                 haloMass, 
                                 haloScaleRadius );

    ax[i] = axi;
    ay[i] = ayi;
    az[i] = azi;
    
}



void EngineCUDA::ComputeAccelerationsBarnesHut()
{
    CopyOnlyPosAndMassDeviceToHost();   // Dont need velocity and acceleration on CPU for tree building

    m_tree.Build( m_particles );

    ReallocateTreeNodesOnDevice();

    CopyTreeNodesToDevice();

    const int blocks = (m_particlesDevice.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    ComputeAccelerationsBarnesHut_kernel<<<blocks, BLOCK_SIZE, 0, m_stream>>>
    (  
        m_particlesDevice.accel[0], 
        m_particlesDevice.accel[1],
        m_particlesDevice.accel[2],
        m_particlesDevice.pos[0], 
        m_particlesDevice.pos[1],
        m_particlesDevice.pos[2],
        m_inputData.gravitationalConstant,
        m_inputData.softeningLength,
        m_inputData.haloMass,
        m_inputData.haloScaleRadius,
        m_particlesDevice.count,

        m_treeNodesDevice.mass,
        m_treeNodesDevice.centerOfMass[0],
        m_treeNodesDevice.centerOfMass[1],
        m_treeNodesDevice.centerOfMass[2],
        m_treeNodesDevice.width,
        m_treeNodesDevice.childNodeIndices,
        m_treeNodesDevice.leafParticleIdx,
        m_treeNodesDevice.isLeaf,
        m_inputData.maxOpeningAngle,
        m_treeNodesDevice.count
    );

    CUDA_CHECK(cudaGetLastError());

}



__global__ void Kick_kernel( floatType* __restrict__ vx,
                             floatType* __restrict__ vy,
                             floatType* __restrict__ vz,
                             const floatType* __restrict__ ax,
                             const floatType* __restrict__ ay,
                             const floatType* __restrict__ az,
                             intType nParticles, 
                             floatType dt )
{
    intType i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= nParticles) 
        return;

    vx[i] += 0.5f * ax[i] * dt;
    vy[i] += 0.5f * ay[i] * dt;
    vz[i] += 0.5f * az[i] * dt;
}



void EngineCUDA::Kick()
{
    int blocks = (m_particlesDevice.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    Kick_kernel<<<blocks, BLOCK_SIZE, 0, m_stream>>>( m_particlesDevice.vel[0], 
                                                      m_particlesDevice.vel[1],
                                                      m_particlesDevice.vel[2],
                                                      m_particlesDevice.accel[0],
                                                      m_particlesDevice.accel[1],
                                                      m_particlesDevice.accel[2],
                                                      m_particlesDevice.count,
                                                      m_inputData.timeStepSize );
    CUDA_CHECK(cudaGetLastError());
}



__global__ void Drift_kernel( floatType* __restrict__ x,
                              floatType* __restrict__ y,
                              floatType* __restrict__ z,
                              const floatType* __restrict__ vx,
                              const floatType* __restrict__ vy,
                              const floatType* __restrict__ vz,
                              intType nParticles, 
                              floatType dt )
{
    intType i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= nParticles) 
        return;

    x[i] += vx[i] * dt;
    y[i] += vy[i] * dt;
    z[i] += vz[i] * dt;
}



void EngineCUDA::Drift()
{
    int blocks = (m_particlesDevice.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    Drift_kernel<<<blocks, BLOCK_SIZE, 0, m_stream>>>( m_particlesDevice.pos[0], 
                                                       m_particlesDevice.pos[1],
                                                       m_particlesDevice.pos[2],
                                                       m_particlesDevice.vel[0],
                                                       m_particlesDevice.vel[1],
                                                       m_particlesDevice.vel[2],
                                                       m_particlesDevice.count,
                                                       m_inputData.timeStepSize );
    CUDA_CHECK(cudaGetLastError());
}


}   // end namespace GALAXY
