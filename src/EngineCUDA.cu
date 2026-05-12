#include "Engine.h"

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
    Particles m_particles,
              m_particlesPinned,
              m_particlesDevice;
    const InputData &m_inputData;

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

    private: 

        void AllocateDeviceMemory();
        void AllocateHostPinnedMemory();
        void FreeMemory();
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

    // Device memory
    for ( intType i = 0; i != 3; i++ ) {
        cudaFree(m_particlesPinned.pos[i]);
        cudaFree(m_particlesPinned.vel[i]);
        cudaFree(m_particlesPinned.accel[i]);
    }
    cudaFree(m_particlesPinned.mass);
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
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.pos[i]  , m_particlesDevice.pos[i]  , bytes, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.vel[i]  , m_particlesDevice.vel[i]  , bytes, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(m_particlesPinned.accel[i], m_particlesDevice.accel[i], bytes, cudaMemcpyHostToDevice));
    }
    CUDA_CHECK(cudaMemcpy(m_particlesPinned.mass, m_particlesDevice.mass, bytes, cudaMemcpyHostToDevice));


    // Copy from pinned memory to heap
    for ( intType i = 0; i != 3; i++ ) {
        std::memcpy(m_particles.pos[i]  , m_particlesPinned.pos[i]  , bytes);
        std::memcpy(m_particles.vel[i]  , m_particlesPinned.vel[i]  , bytes);
        std::memcpy(m_particles.accel[i], m_particlesPinned.accel[i], bytes);
    }
    std::memcpy(m_particles.mass, m_particlesPinned.mass, bytes);
}



__global__ void ComputeAccelerations_kernel( floatType* __restrict__ ax,
                                             floatType* __restrict__ ay,
                                             floatType* __restrict__ az,
                                             const floatType* __restrict__ x,
                                             const floatType* __restrict__ y,
                                             const floatType* __restrict__ z,
                                             const floatType* __restrict__ mass,
                                             floatType gravitationalConstant, 
                                             floatType softeningLength,
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
                if ( p2Global > nParticles )
                    continue;

                if ( p2Global == p2 )
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
        ax[p1] = axTemp;
        ax[p1] = ayTemp;
        az[p1] = azTemp;
    }

}



void EngineCUDA::ComputeAccelerations()
{
    int blocks = (m_particlesDevice.count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    ComputeAccelerations_kernel<<<blocks, BLOCK_SIZE, 4 * BLOCK_SIZE * sizeof(floatType), m_stream>>>
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
        m_particlesDevice.count
    );

    CUDA_CHECK(cudaGetLastError());
}



__global__ void Kick_kernel( floatType* __restrict__ vx,
                             floatType* __restrict__ vy,
                             floatType* __restrict__ vz,
                             const floatType* __restrict__ ax,
                             const floatType* __restrict__ ay,
                             const floatType* __restrict__ az,
                             floatType nParticles, 
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
                              floatType nParticles, 
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
