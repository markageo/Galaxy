#include "Types.h"
#include "InputData.h"
#include "InputParser.h"
#include "IOTools.h"

#include <boost/property_tree/ptree.hpp>

#include <utility>
#include <optional>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <filesystem>
#include <memory>

#define VECTOR_START_CHAR       '('
#define VECTOR_END_CHAR         ')'
#define VECTOR_DELIMITER_CHAR   ','
#define MULTI_DELIMITER_CHAR    ','

namespace pt = boost::property_tree;


/*-------------------------------------------------------------------------------------*\
                                      Translators
\*-------------------------------------------------------------------------------------*/


// Parse vector string into an std::vector
template<typename T>
std::vector<T> ParseVectorString( const std::string &vecString )
{
    std::vector<T> vec;
    std::string::const_iterator stringIterator = vecString.begin();
    std::string valueString;

    if (*stringIterator != VECTOR_START_CHAR) {
        throw std::runtime_error( "'" + vecString + "' is not a valid input. Expected a vector." );
    }
    ++stringIterator;
    
    while ( stringIterator != vecString.end() ) {
        if ( *stringIterator == VECTOR_END_CHAR ) {
            vec.push_back( String2Type<T>(valueString) );
            break;
        }

        if ( *stringIterator == VECTOR_DELIMITER_CHAR ) {
            vec.push_back( String2Type<T>(valueString) );
            valueString.clear();
        } else {
            valueString += *stringIterator;
        }
        stringIterator++;
    }

    if (*stringIterator != VECTOR_END_CHAR) {
        throw std::runtime_error( "'" + vecString + "' vector not closed." );
    }

    return vec;
}


template< typename T >
struct VectorTranslator
{
    typedef std::string     internal_type;
    typedef std::vector<T>  external_type;

    external_type get_value( internal_type const &s ) {
        return ParseVectorString<T>( s );
    }
};



// Specialization allows the translator to be used with ptree internally
namespace boost { namespace property_tree {

    template< typename T > 
    struct translator_between< std::string, std::vector< T > > 
    {
        typedef VectorTranslator< T > type;
    };

}   // end namespace property_tree  
}   // end namespace boost



namespace GALAXY
{

// Parse input file and read into InputData structure
InputData ReadInputData( const std::string &inputFilename ) 
{
    pt::ptree tree = INP::ParseFile(inputFilename);

    InputData inputData;
    std::string inputFileDirectory = IOTOOLS::RelativePath( inputFilename );

    inputData.numberOfInitialParticles = tree.get<intType>( "numberOfInitialParticles" );
    inputData.gravitationalConstant    = tree.get<floatType>( "gravitationalConstant" );
    inputData.particleMass             = tree.get<floatType>( "particleMass" );
    inputData.softeningLength          = tree.get<floatType>( "softeningLength" );
    inputData.timeStepSize             = tree.get<floatType>( "timeStepSize" );
    inputData.numberOfTimeSteps        = tree.get<intType>( "numberOfTimeSteps" );

    inputData.outputPath = tree.get<std::string>("outputFilePath");
    IOTOOLS::PrependRelativePath( inputData.outputPath, inputFileDirectory );


    return inputData;
}


// Read command line input for input file name
InputData ReadInputDataFromCommandLine(int argc, char const *argv[])
{
    // InputData object to return
    InputData inputData;

    // Input file from first command line argument
    std::string inputFilename;
    if (argc == 1) {
        std::cout << "Please enter input file name: "
                  << "\n";
        std::cin >> inputFilename;
        std::cout << "\n";
        std::cin.ignore();

    }
    else if (argc == 2) {
        inputFilename = argv[1];

    }
    else {
        throw std::invalid_argument("Invalid command line options.");

    }

    // User input data
    std::string inputFileRetryChoice;
    while (true)
    {
        try
        {
            std::cout << "Reading input file '" + inputFilename + "' ... ";
            inputData = ReadInputData(inputFilename);
            std::cout << "Success."
                      << "\n\n";
            break;
        } 
        catch (std::runtime_error &e) 
        {
            std::cout << "Failed. \n"
                      << e.what()
                      << "\n\n";

            std::cout << "Would you like to try again? (y/n)"
                      << "\n";
            std::cin >> inputFileRetryChoice;
            if (inputFileRetryChoice != "y")
                exit(-1);
            std::cout << "\n";

            std::cout << "Please enter input file name: "
                      << "\n";
            std::cin >> inputFilename;
            std::cout << "\n";
            std::cin.ignore();
        }
    }
    
    return inputData;
}

}   // end namespace GALAXY