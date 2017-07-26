#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)
project (ZWaveModule)

#include("dependencies.cmake")

#set(MODULES_DIR ./dependencies} CACHE INTERNAL "Modules include directory" FORCE)

set(zwave_module_cpp_files    
    ./src/MinOZW.cpp
)
set(zwave_module_h_files    
    ./src/MinOZW.h
)

add_executable(MinOZW MinOZW.cpp)

