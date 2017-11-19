#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.12)

set(SmartHive.ZWaveGateway_sources
    ./ZWaveGateway.cpp
	./module_service_config_disabled.c
)

set(SmartHive.ZWaveGateway_headers
    ./ZWaveGateway.h
)
if(WIN32)
    set(SmartHive.ZWaveGateway_sources 
        ${SmartHive.ZWaveGateway_sources}
        ./zwave_device_cloud_upload_win.json
    )
    set_source_files_properties(./zwave_device_cloud_upload_win.json PROPERTIES HEADER_FILE_ONLY ON)
else()
    set(SmartHive.ZWaveGateway_sources 
        ${SmartHive.ZWaveGateway_sources}
        ./zwave_device_cloud_upload_linx64.json
    )
    set_source_files_properties(./zwave_device_cloud_upload_linx64.json PROPERTIES HEADER_FILE_ONLY ON)
endif()


include_directories(./inc ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${GW_INC})
include_directories(../../modules/common)

add_executable(SmartHive.ZWaveGateway ${SmartHive.ZWaveGateway_sources} ${SmartHive.ZWaveGateway_headers})

add_dependencies(SmartHive.ZWaveGateway SmartHive.ZWaveModule SmartHive.ZWaveMappingModule iothub logger)

target_link_libraries(SmartHive.ZWaveGateway gateway)
linkSharedUtil(SmartHive.ZWaveGateway)
linkHttp(SmartHive.ZWaveGateway)
install_broker(SmartHive.ZWaveGateway ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_gateway_dll(SmartHive.ZWaveGateway ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
copy_iothub_client_dll(SmartHive.ZWaveGateway ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )

add_sample_to_solution(SmartHive.ZWaveGateway)

if(install_executables)
    install(TARGETS SmartHive.ZWaveGateway DESTINATION bin)
endif()