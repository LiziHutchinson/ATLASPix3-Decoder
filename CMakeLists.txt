#attempt to turn atlaspix3_decoding.pro into a cmake file

project (atlaspix3_decoder)

set(CMAKE_CXX_FLAGS -std=c++11 -Wall)

set(SOURCES atlaspix3_decoder.cpp 
            decoder.cpp 
            atlaspix3.cpp 
            dataset.cpp 
	    fileoperations.cpp)

include_directories(/home/atlas/lizih/Documents/PhD/DESYData/atlaspix3_221013/atlaspix3_fixed_decoder/atlaspix3_telescope_decoding-fix_decoder3)
