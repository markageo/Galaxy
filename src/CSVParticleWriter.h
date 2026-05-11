#ifndef GALAXY_CSV_PARTILCE_WRITER
#define GALAXY_CSV_PARTILCE_WRITER

#include "Particles.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>

#define GALAXY_CSV_FILE_WRITE_PRECISION 15

namespace GALAXY
{


namespace internal
{

class CSVFile
{

    public:
        CSVFile( const std::string &filename, 
                 const int precision = GALAXY_CSV_FILE_WRITE_PRECISION ) :
            m_fileStream( filename ),
            m_precision( precision )
        {           

            if ( !m_fileStream ) {
                std::cout << "CSV File ERROR: Could not open/create file! \n" << std::endl;
    }

            m_fileStream << std::setprecision( m_precision ) << std::scientific;
        }

        template <class ...Args> 
        void WriteLine( Args... args )
        {
            const int nArgs = sizeof...(args);
            int i = 0;
        
            // Fold expression to loop through args. This is needed since each arg can have a different type
            ( [&] {

                i++;
                m_fileStream << std::left << args;
                if ( i != nArgs )
                    m_fileStream << ",";

            } (), ... );

            m_fileStream << std::endl;  // flush to write immediately to file
        }

    private:
        std::ofstream m_fileStream;
        int m_precision;
};


}   // end namespace internal




void WriteParticleStateToFile( const Particles &particles,
                               const std::string &filename, 
                               const int precision = GALAXY_CSV_FILE_WRITE_PRECISION )
{
    using namespace internal;

    CSVFile csvFile( filename, precision );
    
    // Write header
    csvFile.WriteLine( "x", 
                       "y", 
                       "z",
                       "vx",
                       "vy", 
                       "vz" );

    // Write particles data
    for ( intType i = 0; i != particles.count; i++ ) {
        csvFile.WriteLine( particles.pos[0][i],
                           particles.pos[1][i],
                           particles.pos[2][i],
                           particles.vel[0][i],
                           particles.vel[1][i],
                           particles.vel[2][i] );
    }
}



}   // end namespace GALAXY

#endif  // GALAXY_CSV_PARTILCE_WRITER