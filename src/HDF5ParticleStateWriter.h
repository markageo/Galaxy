#ifndef GALAXY_HDF5_PARTILCE_WRITER
#define GALAXY_HDF5_PARTILCE_WRITER

#include "Particles.h"
#include "IOTools.h"

#include <hdf5.h>

#include <string>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <vector>



namespace GALAXY
{


namespace internal
{

// Maps C++ scalar type to HDF5 type tage, and byte width.
template <typename T>
struct hdf5_type; 
 
template <>
struct hdf5_type<float> {
    static hid_t native() { return H5T_NATIVE_FLOAT; }
    static constexpr int xdmfPrecision = 4;
};
 
template <>
struct hdf5_type<double> {
    static hid_t native() { return H5T_NATIVE_DOUBLE; }
    static constexpr int xdmfPrecision = 8;
};
 


// HDF5 data file, one flat dataset per field
template <typename T>
inline void WriteSnapshotHDF5( const std::string &filename,
                               const Particles &particles)
{ 
    const hid_t nativeType = hdf5_type<T>::native();
 
    std::string filenameWithExtension = GALAXY::IOTOOLS::AddFileExtension( filename, ".h5" );

    hid_t file = H5Fcreate(filenameWithExtension.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0) throw std::runtime_error("writeSnapshotHDF5: cannot create " + filenameWithExtension);
 
    hsize_t n = static_cast<hsize_t>(particles.count);
    hid_t dataspace = H5Screate_simple(1, &n, nullptr);
 
    auto writeDataset = [&](const char* name, const T* dataPointer) {
        hid_t dset = H5Dcreate2(file, name, nativeType, dataspace,
                                 H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dset < 0) throw std::runtime_error(std::string("writeSnapshotHDF5: cannot create dataset ") + name);
        herr_t status = H5Dwrite(dset, nativeType, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataPointer);
        H5Dclose(dset);
        if (status < 0) throw std::runtime_error(std::string("writeSnapshotHDF5: write failed for ") + name);
    };
 
    writeDataset("x",    particles.pos[0]);
    writeDataset("y",    particles.pos[1]);
    writeDataset("z",    particles.pos[2]);
    writeDataset("vx",   particles.vel[0]);
    writeDataset("vy",   particles.vel[1]);
    writeDataset("vz",   particles.vel[2]);
 
    H5Sclose(dataspace);
 
    // // Use double precision for time for simplicity
    // hid_t root = H5Gopen2(file, "/", H5P_DEFAULT);
    // hid_t scalarSpace = H5Screate(H5S_SCALAR);
    // hid_t attr = H5Acreate2(root, "Time", H5T_NATIVE_DOUBLE, scalarSpace,
    //                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // H5Awrite(attr, H5T_NATIVE_DOUBLE, &time);
    // H5Aclose(attr);
    // H5Sclose(scalarSpace);
    // H5Gclose(root);
 
    H5Fclose(file);
}
 


// Per-state XDMF so can be read into ParaView.
template <typename T>
inline void WriteSnapshotXDMF( const std::string &filename, 
                               std::size_t numParticles,
                               double time)
{
    constexpr int precision = hdf5_type<T>::xdmfPrecision;
 
    // remove the path of the hdf5Filename
    std::string h5Filename   = GALAXY::IOTOOLS::AddFileExtension( GALAXY::IOTOOLS::StripPath( filename ), 
                                                                  ".h5" );
    std::string xdmfFilename = GALAXY::IOTOOLS::AddFileExtension( filename, 
                                                                  ".xdmf" );

    std::ofstream out(xdmfFilename);
    if (!out) throw std::runtime_error("writeSnapshotXDMF: cannot open " + xdmfFilename);
 
    out << "<?xml version=\"1.0\" ?>\n";
    out << "<Xdmf Version=\"3.0\">\n";
    out << "  <Domain>\n";
    out << "    <Grid Name=\"particles\" GridType=\"Uniform\">\n";
    out << "      <Time Value=\"" << time << "\"/>\n";
    out << "      <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"" << numParticles << "\"/>\n";
    out << "      <Geometry GeometryType=\"X_Y_Z\">\n";
    out << "        <DataItem Dimensions=\"" << numParticles << "\" NumberType=\"Float\" Precision=\"" << precision << "\" Format=\"HDF\">" << h5Filename << ":/x</DataItem>\n";
    out << "        <DataItem Dimensions=\"" << numParticles << "\" NumberType=\"Float\" Precision=\"" << precision << "\" Format=\"HDF\">" << h5Filename << ":/y</DataItem>\n";
    out << "        <DataItem Dimensions=\"" << numParticles << "\" NumberType=\"Float\" Precision=\"" << precision << "\" Format=\"HDF\">" << h5Filename << ":/z</DataItem>\n";
    out << "      </Geometry>\n";
 
    auto attribute = [&](const char* name, const char* dataset) {
        out << "      <Attribute Name=\"" << name << "\" AttributeType=\"Scalar\" Center=\"Node\">\n";
        out << "        <DataItem Dimensions=\"" << numParticles << "\" NumberType=\"Float\" Precision=\"" << precision << "\" Format=\"HDF\">" << h5Filename << ":" << dataset << "</DataItem>\n";
        out << "      </Attribute>\n";
    };

    attribute("vx", "/vx");
    attribute("vy", "/vy");
    attribute("vz", "/vz");
 
    out << "    </Grid>\n";
    out << "  </Domain>\n";
    out << "</Xdmf>\n";
}

}   // end namespace internal




void WriteParticleStateToHDF5File( const Particles &particles,
                                   const std::string &filename,
                                   intType timeStep )
{
    using namespace internal;

    WriteSnapshotHDF5<floatType>(filename, particles);
    WriteSnapshotXDMF<floatType>(filename, particles.count, static_cast<floatType>(timeStep));  // Just make time the timestep
}


}   // end namespace GALAXY

#endif  // GALAXY_HDF5_PARTILCE_WRITER