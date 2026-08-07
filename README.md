# Galaxy
Simulates the gravitational N-body problem using either OpenMP or CUDA backends. Can perform force calculation using either an all-pairs algorithm or the Barnes-Hut algorithm. For the Barnes-Hut Algorithm, the tree is constructed on the CPU, even if a CUDA backend is used. Tree traversal and force calculation is done on the GPU however. A simple exponential disk galaxy is used as the initial condition with a Hernquist dark matter halo. Snapshots of particle state at each time can be written to CSV or HDF5 files for visualisation (using [Paraview](https://www.paraview.org/) for example). Simulation parameters, including which backend (CUDA or OpenMP) and which force calculation algorithm to use can be set in the input file (see `example.inp` in the `cases` directory). 


### Prerequisites

The following are required to build and use the GALAXY application:

* A C++ compiler which supports C++20. Clang and GCC have be tested to work.
* (Optional) A CUDA capable GPU with CUDA installed in order to use the CUDA backend.
* [CMake](https://cmake.org/) - To build the project.
* (Optional) [HDF5](https://www.hdfgroup.org/solutions/hdf5/) to output particle state snapshots in binary format. If HDF5 is not available, particle state snapshots can be written to CSV files using an internal reader, but this is less efficient and results in larger file sizes.


### Build and usage

After cloing the repository, the project can be build like any other CMake project:

    mkdir build
    cd build
    cmake ../
    make 
Which will create an executable called `galaxy`. This can then be executed with an input file as:

    ./galaxy cases/example.inp



