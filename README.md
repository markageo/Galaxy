# Galaxy
Simulates the gravitational N-body problem using either OpenMP or CUDA backends. Can perform force calculation using either an all-pairs algorithm or the Barnes-Hut algorithm. For the Barnes-Hut Algorithm, the tree is constructed on the CPU, even if a CUDA backend is used. Tree traversal and force calculation is done on the GPU however. A simple exponential disk galaxy is used as the initial condition with a Hernquist dark matter halo. Particle positions with time are written to csv files for visualisation (using [Paraview](https://www.paraview.org/) for example). Simulation parameters, including which backend (CUDA or OpenMP) and which force calculation algorithm to use can be set in the input file (see `example.inp` in the `cases` directory). 

### Build and usage

After cloing the repository, the project can be build like any other CMake project:

    mkdir build
    cd build
    cmake ../
    make 
Which will create an executable called `galaxy`. This can then be executed with an input file as:

    ./galaxy cases/example.inp



