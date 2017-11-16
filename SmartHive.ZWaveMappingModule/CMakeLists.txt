#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(zwave_identity_map_sources
    ./ZWaveIdentityMap.cpp
)

set(zwave_identity_map_headers
    ./ZWaveIdentityMap.h
)

include_directories(./)
include_directories(${GW_INC})

#this builds the SmartHive.ZWaveMappingModule dynamic library
add_library(SmartHive.ZWaveMappingModule MODULE ${zwave_identity_map_sources}  ${zwave_identity_map_headers})
target_link_libraries(SmartHive.ZWaveMappingModule gateway)

#this builds the SmartHive.ZWaveMappingModule static library
add_library(SmartHive.ZWaveMappingModule_static STATIC ${zwave_identity_map_sources} ${zwave_identity_map_headers})
target_compile_definitions(SmartHive.ZWaveMappingModule_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(SmartHive.ZWaveMappingModule_static gateway)

linkSharedUtil(SmartHive.ZWaveMappingModule)
linkSharedUtil(SmartHive.ZWaveMappingModule_static)

add_module_to_solution(SmartHive.ZWaveMappingModule)

if(install_modules)
    install(TARGETS SmartHive.ZWaveMappingModule LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()

if(${run_unittests})
	add_subdirectory(tests)
endif()

