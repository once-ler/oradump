cmake_minimum_required(VERSION 3.2)

project(oradump VERSION 0.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "-fno-stack-protector")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Threads)
find_package(Boost 1.62.0 COMPONENTS thread date_time program_options filesystem system regex REQUIRED)

MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR} )

set(OCI_INCLUDE /usr/include/oracle/18.3/client64)
# Assume ocilib has been installed in /usr/local so no need to register.

include_directories(
    "${PROJECT_SOURCE_DIR}/include"
    "${OCI_INCLUDE}"
    )

#[[
1. Make sure LD_LIBRARY_PATH has the Oracle lib directory with libclntsh.so and libocilib.so during runtime.
2. Make sure to add tnsnames.ora and update TNS_ADMIN in your environment.
   sudo mkdir -p $ORACLE_HOME/admin
   sudo cp ./tnsnames.ora $ORACLE_HOME/admin
   sudo chmod -R a+rx $ORACLE_HOME/admin

In your .bashrc for example:

export ORACLE_HOME=/usr/lib/oracle/18.3/client64
export PATH=$PATH:$ORACLE_HOME/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ORACLE_HOME/lib:/usr/local/lib

export TNS_NAMES=$ORACLE_HOME/admin

libclntsh.so requires libaio.so
Ubuntu install:
sudo apt-get install libaio1

Redhat install:
sudo yum install libaio-devel
]]

set(OCI_LIB_DIR /usr/lib/oracle/18.3/client64/lib)

link_directories(${OCI_LIB_DIR})

set(CPP_SOURCES ${PROJECT_SOURCE_DIR}/index.cpp)

MESSAGE( STATUS "CPP_SOURCES: " ${CPP_SOURCES} )

# Make static
# set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(oradump ${CPP_SOURCES})

target_link_libraries(
    oradump
    ${Boost_LIBRARIES}
    ocilib
    clntsh       
)
