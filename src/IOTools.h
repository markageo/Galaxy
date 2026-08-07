#ifndef GALAXY_IOTOOLS
#define GALAXY_IOTOOLS

#include <string>
#include <filesystem>

namespace GALAXY
{

namespace IOTOOLS
{

    // Convert string to given numeric type T.
    template <typename T> T 
    String2Type(const std::string &str)
    {
        // NOTE: This does not work for ints in scientific notation.
        std::istringstream strstream(str);
        T num;
        strstream >> num;
        return num;
    }


    // Return string with no whitespace
    inline std::string RemoveWhitespace( std::string str )
    {
        str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
        return str;
    }


    // Return relative path to given filename (LINUX FILESYSTEMS ONLY)
    inline std::string RelativePath( const std::string &filename ) 
    { 
        std::string::const_iterator stringIterator = filename.end();
        for ( /* NULL */ ; *stringIterator != '/' && stringIterator != filename.begin(); stringIterator--) {};
        return std::string( filename.begin(), stringIterator );
    }


    // Remove file extension
    inline std::string RemoveFileExtension( const std::string &filename,
                                            const std::string &fileExtension )
    {
        size_t lastPointPosition = filename.find_last_of(".");
        if ( lastPointPosition == std::string::npos ) {
            return filename;
        }
        if ( filename.substr( lastPointPosition ) == fileExtension ) {
            return filename.substr( 0, lastPointPosition );
        }
        return filename;
    }


    // Add file extension to filename
    inline std::string AddFileExtension( const std::string &filename,
                                         const std::string &fileExtension )
    {   
        // Don't double-add the file extension
        return RemoveFileExtension( filename, fileExtension ) + fileExtension;
    }


    // If a path is not an absolute path, add the given directory to it
    inline void PrependRelativePath( std::string &pathString,
                                     const std::string &directoryToPrependString )
    {
        std::filesystem::path path( pathString );

        if ( !path.is_absolute() ) {
            if ( !directoryToPrependString.empty() )
                pathString = directoryToPrependString + "/" + pathString;
        }
    }


    // Return the filename with the path stripped
    inline std::string StripPath( const std::string &filename )
    {
        return std::filesystem::path(filename).filename().string();
    }


}   // end namespace IOTOOLS

}   // end namespace GALAXY





#endif // GALAXY_IOTOOLS
