What is CMake?
==============

CMake is a cross platform build system, that can be used to replace the old 
auto-tools, providing a nice building environment and advanced features.

Some of these features are:
* Out of sources build: CMake allows you to build your software into a directory 
  different to the source tree. You can safely delete the build directory and 
  all its contents once you are done.
* Multiple generators: classic makefiles can be generated for Unix and MinGW, 
  but also Visual Studio, XCode and Eclipse CDT projects among other types.
* Graphic front-ends for configuration and build options.

More information and documentation is available at the CMake project site:
    http://www.cmake.org

CMake is free software. You can get the sources and pre-compiled packages for
Linux and other systems at:
     http://www.cmake.org/cmake/resources/software.html
     
How to use it?
==============

1. You need CMake 3.10 or newer to build samplv1

2. Unpack the samplv1 sources somewhere, or checkout the repository, 
   and create a build directory. For instance, using a command line shell:
   
$ tar -xvzf Downloads/samplv1-x.y.z.tar.gz
$ cd samplv1-x.y.z
$ mkdir build

2. Execute CMake from the build directory, providing the source directory 
   location and optionally, the build options. There are several ways.

* From a command line shell:

$ pwd
samplv1-x.y.z
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=debug ..

3. Execute the build command. If you used the Makefiles generator (the default
   in Linux and other Unix systems) then execute make, gmake, or mingw32-make.
   If you generated a project file, use your IDE to build it.

Compiling with make
===================

There are many targets available. To see a complete list of them, type:

$ make help

The build process usually hides the compiler command lines, to show them:

$ make VERBOSE=1

There is a "clean" target, but not a "distclean" one. You should use a build
directory different to the source tree. In this case, the "distclean" target 
would be equivalent to simply removing the build directory. 

If something fails
==================

If there is an error message while executing CMake, this probably means that a
required package is missing in your system. You should install the missing 
component and run CMake again.

If there is an error executing the build process, after running a flawless CMake
configuration process, this means that there may be an error in the source code, 
or in the build system, or something incompatible in 3rd party libraries.
