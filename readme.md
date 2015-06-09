# Hugin

Street network import for Mimir (http://fr.wikipedia.org/wiki/M%C3%ADmir), the Navitia autocomplete service (https://github.com/CanalTP/navitia/)

Hugin loads OSM and Bano data into Mimir


# Architecture
Mimir, the autocomplete service is based on ElasicSearch (https://www.elastic.co/).

TODO

# Build instructions

- Get the submodules: at the root of project :

   ``git submodule update --init``

- Get Casablanca

    you need casablanca (https://casablanca.codeplex.com/) to build Hugin.

    If your distribution provides Cablanca, get it from there (it's libcpprest-dev in debian)

    Else if you can, get it from a CanalTP repository
    
    Else build it, it's quite easy (cmake source && make -j4)

- With CMake you can build in a repository different than the source directory.

   By convention, you can have one build repository for each kind of build.
   Create a directory where everything will be built and enter it
   ``mkdir release``
   ``cd release``

- Run cmake

   ``cmake ../source``
   Note: it will build in debug mode. If you want to compile it as a release run
   ``cmake -DCMAKE_BUILD_TYPE=Release ../source``

- Compile

   ``make -j4``
   Note: adjust -jX according to the number of cores you have


# How to use

TODO
