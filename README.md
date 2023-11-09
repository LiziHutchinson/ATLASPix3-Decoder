If qmake not available, use a direct call of g++. There is a CMakeLists file for this purpose. 

The script is embedded in a qt project for easier debugging. Nothing but qmake is used.
Instead of using qmake, the project can also be compiled using a direct call of g++.

The script takes one parameter which is the path of a configuration file.
The file contains the information about how to decode the data and where the data is located, as well as the output path and optional time shifts for each layer.
The previously used command line parameters are not supported any more as they are less easy to reproduce and the function call got too complicated.
