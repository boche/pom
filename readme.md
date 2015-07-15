# Introduction

This package contains two modified version of POM. The original version can be downloaded from http://cvlab.epfl.ch/software/pom. 

POM is short for Probabilistic Occupancy Map, which is first proposed in this paper:

Fleuret, Francois, et al. "Multicamera people tracking with a probabilistic occupancy map." Pattern Analysis and Machine Intelligence, IEEE Transactions on 30.2 (2008): 267-282.

# Modification

This version of POM is different from the original version in the following ways:

* support multiple threads;
* support tilted camera;
* support camera with different resolution;
* build with the CMake;

During modification, I tried to modify minially, so the code should be pretty similar to the original version.

# Note

1. This code works only with multiple threads, you can set the thread number in *pom.cc*, the default setting is **const int THREAD_POOL_SIZE = 8**. And thanks to multiple threads, pom's speed can be sped up by almost [number of threads] times.
2. When the camera is tilted, i.e., a standing person would be projected to a tilted rectangle, you are suggested to use *pom-mt-tilted* in this case, while *pom-mt-vertical* works for the  vertical rectangles. The main trick is instead of integral image, integral stripe is used in *pom-mt-tilted* as an efficient approximation.

# Build

* mkdir release
* cd release
* cmake ..
* make
* software tested under OS X and linux;

# License

The original code of POM is distrbuted with GPL v3.0 license, so will this software.