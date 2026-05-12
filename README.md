# Galaxy
Simulates the gravitational N-body problem. Currently uses an all pairs approach for computing the gravitational force between particles. Time stepping is done using Leapfrog itegration. A Barnes-Hut algorithm will be added in future. A simple exponential disk galaxy is used as the initial condition. Particle positions with time are written to csv files for visualisation (using [Paraview](https://www.paraview.org/) for example). Integration and force calculations can be done using either OpenMP or CUDA backends, which are selected in the input file (see `example.inp` in the `cases` directory). 

### Build and usage

After cloing the repository, the project can be build like any other CMake project:

    mkdir build
    cd build
    cmake ../
    make 
Which will create an executable called `galaxy`. This can then be executed with an input file as:

    ./galaxy cases/example.inp

Where `example.inp` is the input file used to run the code. All solver settings are controlled through this single input file. 


