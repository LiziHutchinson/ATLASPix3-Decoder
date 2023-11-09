#ifndef __FILEOPERATIONS
#define __FILEOPERATIONS

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#if !defined(__linux__)
#include <process.h>
#endif

//#include "TGraphErrors.h"
//#include "TFile.h" //used for creating ROOT files

//#define FILEOP_DEBUG

/**
 * @brief ConvertToWinPath replaces the unix style "/" in file paths by
 *      the "\" for windows to work with
 * @param path              - the path to convert to "Windows-Style"
 * @return                  - the path with "/" replaced by "\"
 */
std::string ConvertToWinPath(std::string path);

/**
 * @brief DirectoryExists checks on file system whether a file at `path` exists
 * @param path              - path of the directory to test for existence
 * @return                  - true if the directory exists, false if not
 */
bool DirectoryExists(std::string path);
/**
 * @brief FileExists checks on file system whether a file at `path` exists
 * @param path              - path of the file to test for existence
 * @return                  - true if the file exists (and is actually a file), false if not
 */
bool FileExists(std::string path);


/**
 * @brief CreateDirectory creates a directory at path. It does not support creating
 *      subdirectories, so call it several times to create nested directories
 * @param path              - path for the directory to create
 * @return                  - true on successful creation of the directory, false on an error
 */
bool CreateDirectory(std::string path);

/**
 * @brief WriteToFile opens the file at `filename` to write `data` to it and close it again
 * @param filename          - file to write the data to
 * @param data              - the data to write to the file
 * @param fileflags         - file flage for opening (in addition to std::ios::out)
 * @return                  - true on successful writing, false on an error
 */
bool WriteToFile(std::string filename, std::string data,
                    std::ios::openmode fileflags = std::ios::app | std::ios::binary);
                    
/**
 * @brief StripSpaces removes leading and trailing spaces and tab characters from the passed
 *          string
 * @param text              - the text to be stripped from leading and trailing spaces and tabs
 * @return                  - a new string containing the data from `text` but without space
 *                              padding
 */
std::string StripSpaces(std::string text);

/**
 * @brief ToLowerCase replaces all english capital letters ('A' to 'Z') with the lower case ones
 *          ('a' to 'z') for easier pattern matching.
 * @param text               - the text to be converted
 * @return                   - the string with all capital letters replaced by lower case ones
 */
std::string ToLowerCase(const std::string text);

typedef std::vector<std::pair<std::string, std::string> > StringPairs;
typedef std::vector<std::pair<std::string, int> >         StringIntPairs;
typedef std::vector<std::pair<std::string, double> >      StringDoublePairs;

/**
 * @brief FindKey searches the passed container for the given key and returns its value or the
 *      fail value
 * @param data              - the container with the key-value pairs
 * @param key               - the key to search for
 * @param fail              - the value to return on failed search
 * @return                  - returns the value for the passed key from data or the fail value
 *                              on an error
 */
std::string FindKey(StringPairs& data, std::string key, const std::string fail = "");
/**
 * @brief FindKeyBool searches the passed container for the given key and returns its value or the
 *      fail value. Since the value string has to be converted, any other string as "true" will be
 *      casted to false (not case sensitive)
 * @param data              - the container with the key-value pairs
 * @param key               - the key to search for
 * @param fail              - the value to return on failed search
 * @return                  - returns the value for the passed key from data or the fail value
 *                              on an error
 */
bool        FindKeyBool(StringPairs& data, std::string key, const bool fail = false);
/**
 * @brief FindKeyInt searches the passed container for the given key and returns its value or the
 *      fail value. Since the value string has to be converted, also on a failed conversion the 
 *      fail value will be returned.
 * @param data              - the container with the key-value pairs
 * @param key               - the key to search for
 * @param fail              - the value to return on failed search
 * @return                  - returns the value for the passed key from data or the fail value
 *                              on an error
 */
int         FindKeyInt(StringPairs& data, std::string key, const int fail = -1);
/**
 * @brief FindKeyDouble searches the passed container for the given key and returns its value or the
 *      fail value. Since the value string has to be converted, also on a failed conversion the 
 *      fail value will be returned.
 * @param data              - the container with the key-value pairs
 * @param key               - the key to search for
 * @param fail              - the value to return on failed search
 * @return                  - returns the value for the passed key from data or the fail value
 *                              on an error
 */
double      FindKeyDouble(StringPairs& data, std::string key, const double fail = -1e10);


/**
 * @brief LoadParameterFileHeader loads an optional title field for the passed header. The title
 *              can be introduced by a line starting with "# Title:". The rest of the line (until a
 *              comment start) is treated as title. The text can be enclosed in double quotation
 *              marks.
 * @param filename          - the file to load the data from
 * @param header            - with "##" at the beginning of the line, headers can be introduced to
 *                              put several lists in one file. The headers have to be prefix-free.
 * @return                  - the title specified for the passed header or an empty string if the
 *                              title was not specified for the header or header or file were not
 *                              found
 */
std::string LoadParameterFileHeader(std::string filename, std::string header);
/**
 * @brief LoadParameterFile opens a file and returns the content as key value pairs with one pair
 *                per line. A "#" character makes the rest of the line a comment. The key is the 
 *                beginning of the line until the first separation character (space or tab) or - if
 *                started with double quotation marks - until the quotation marks are closed. The 
 *                value is the rest of the line until the first comment starting character ('#'). The
 *                value can be enclosed between double quotation marks, too. In this case, comment 
 *                markers inside the enclosed text will be ignored.
 * @param filename           - the file to load the data from
 * @param header             - with "##" at the beginning of the line, headers can be introduced to
 *                              put several lists in one file. The headers have to be prefix-free.
 * @param printerrors        - errors will be written to std::cerr if set to true
 * @return                   - a vector of key value pairs in the order of occurrence in the file.
 *                               commented lines or lines with broken syntax will be excluded.
 */
StringPairs LoadParameterFile(std::string filename, std::string header = "",
                              bool printerrors = false);
/**
 * @brief LoadParameterFileInt is the same as LoadParameterFile (and calls this function 
 *                internally), but the values are converted to integer numbers. Lines that produce an
 *                error in this conversion will be missing in the resulting list.
 * @param filename           - the file to load the data from
 * @param header             - with "##" at the beginning of the line, headers can be introduced to
 *                              put several lists in one file. The headers have to be prefix-free.
 * @param printerrors        - errors during loading and converting the values to integer numbers
 *                               will be written to std::cerr
 * @return                   - a vector of key value pairs in the order of occurrence in the file.
 *                               Lines with broken syntax or values that can not be converted to int
 *                               will be missing in the output
 */
StringIntPairs LoadParameterFileInt(std::string filename, std::string header = "",
                                    bool printerrors = false);
/**
 * @brief LoadParameterFileDouble is the same as LoadParameterFile (and calls this function 
 *                internally), but the values are converted to double numbers. Lines that produce an
 *                error in this conversion will be missing in the resulting list.
 * @param filename           - the file to load the data from
 * @param header             - with "##" at the beginning of the line, headers can be introduced to
 *                              put several lists in one file. The headers have to be prefix-free.
 * @param printerrors        - errors during loading and converting the values to double numbers
 *                               will be written to std::cerr
 * @return                   - a vector of key value pairs in the order of occurrence in the file.
 *                               Lines with broken syntax or values that can not be converted to 
 *                               double will be missing in the output
 */
StringDoublePairs LoadParameterFileDouble(std::string filename, std::string header = "",
                                          bool printerrors = false);

template<class T>
std::map<std::string, T> ToMap(std::vector<std::pair<std::string, T> > data)
{
    std::map<std::string, T> mapdata;

    for(const auto& it : data)
        mapdata.insert(it);

    return mapdata;
}

///**
// * @brief LoadDataFile loads a text file with space or tab separated values and converts the
// *              content into a TGraphErrors. The columns to use for the axes and errors can be
// *              selected and commented lines are skipped.
// *              The use of the LoadParameterFile function also enables storing several data
// *              packages in one file separated by headers starting with "##"
// * @param filename           - the file to load the data from
// * @param header             - the header after which the data is to be taken. If it is left empty
// *                               the part before the first header is used (also applies for files
// *                               without headers)
// * @param printerrors        - errors during loading and converting the values to double numbers
// *                               will be written to std::cerr
// * @param values             - a string describing the position of the values to use. The columns
// *                               are separated by spaces and ignored columns are marked with an "i"
// *                               to keep track of the counting. The content after the last used
// *                               column can be omitted. The axes are labelled with "x" and "y", the
// *                               respective errors with "xe" and "ye".
// * @return                   - a TGraphErrors object with the data selected from the file or a
// *                               nullptr in case of an error
// *
// */
//TGraphErrors* LoadDataFile(std::string filename, std::string header = "", bool printerrors = false,
//                           std::string values = "x,y,ye");

#endif //__FILEOPERATIONS
