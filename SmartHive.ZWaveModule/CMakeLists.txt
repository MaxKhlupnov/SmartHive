cmake_minimum_required(VERSION 2.8.12)
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 OLD)
endif()



set(SmartHive.ZWaveModule_headers
    ${SmartHive.ZWaveModule_headers}
    ./ZWaveModule.h
    ./OpenZWaveAdapter.h    
)

set(SmartHive.ZWaveModule_sources
     ./ZWaveModule.cpp
    ./OpenZWaveAdapter.cpp
)

if(WIN32)
    set(open_zwave_link_dirs
        ../../../open-zwave/dotnet/examples/OZWForm/src/Debug/
    )   
    set(open_zwave_libs
		C:/PROJECTS/GitHub/SmartHive/open-zwave/dotnet/examples/OZWForm/src/Debug/OpenZWave.lib       
    )
elseif(LINUX) 
		set(open_zwave_link_dirs
        ../../../open-zwave/
    )  		
	set(open_zwave_libs
		openzwave       
    )
elseif(APPLE)
		#TODO: Add clause
endif()

include_directories(
    ./
    ../../../open-zwave/cpp/src	
    ${GW_INC}
)

set(LIBS ${open_zwave_libs} gateway)
link_directories(${open_zwave_link_dirs})

#../../../include
#	../../../include/x86_64-linux-gnu/sys
#set(LIBS ${LIBS} gateway)

# build SmartHive.ZWaveModule module as a dynamic library
add_library(SmartHive.ZWaveModule MODULE ${SmartHive.ZWaveModule_sources} ${SmartHive.ZWaveModule_headers})
target_link_libraries(SmartHive.ZWaveModule ${LIBS})
linkSharedUtil(SmartHive.ZWaveModule)

# build SmartHive.ZWaveModule as a static library
add_library(SmartHive.ZWaveModule_static STATIC ${SmartHive.ZWaveModule_sources} ${SmartHive.ZWaveModule_headers})
target_compile_definitions(SmartHive.ZWaveModule_static PRIVATE BUILD_MODULE_TYPE_STATIC)
target_link_libraries(SmartHive.ZWaveModule_static ${LIBS} iothub_service_client)
linkSharedUtil(SmartHive.ZWaveModule_static)


add_module_to_solution(SmartHive.ZWaveModule)


if(install_modules)
    install(TARGETS SmartHive.ZWaveModule LIBRARY DESTINATION "${LIB_INSTALL_DIR}/modules") 
endif()
