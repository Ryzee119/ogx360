cmake_minimum_required(VERSION 3.5.1)

## AVR Chip Configuration
set(F_CPU 16000000UL)
set(MCU atmega32u4)
set(ARDUINO 185)

# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)
set(OPTIMIZATION_LEVEL "-Os")
set(DEFAULT_BUILD_TYPE "Release")

# Define project
project(ogx360)
enable_language(C CXX)

# Default common compiler flags
set(AVR_FLAGS "-mmcu=${MCU} -DMCU=${MCU} -DF_CPU=${F_CPU} -D__COMPILING_AVR_LIBC__ -ffunction-sections -fdata-sections")
set(COMMON_FLAGS "${OPTIMIZATION_LEVEL} -DARDUINO=${ARDUINO} -DUSE_LUFA_CONFIG_HEADER ")
set(CMAKE_C_FLAGS "${AVR_FLAGS} ${COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${AVR_FLAGS} ${COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -DRELEASE" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -DRELEASE" CACHE STRING "" FORCE)
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections" CACHE STRING "" FORCE)

# Set default build type if not specified in the cmake command
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# Add the Arduino library
file(GLOB LIB_ARDUINO_FILES 
    "${CMAKE_SOURCE_DIR}/lib/Arduino/*.cpp"
    "${CMAKE_SOURCE_DIR}/lib/Arduino/*.c"
    "${CMAKE_SOURCE_DIR}/lib/Arduino/*.h"
)
add_library(Arduino STATIC "${LIB_ARDUINO_FILES}") 
target_include_directories(Arduino PUBLIC "${CMAKE_SOURCE_DIR}/lib/Arduino")

# Add LUFA Library
file(GLOB_RECURSE LIB_LUFA_FILES 
    "${CMAKE_SOURCE_DIR}/lib/LUFA/*.c" 
    "${CMAKE_SOURCE_DIR}/lib/LUFA/*.h"
)
add_library(LUFA STATIC "${LIB_LUFA_FILES}") 
target_include_directories(LUFA PUBLIC "${CMAKE_SOURCE_DIR}/lib/LUFA")
target_link_libraries(LUFA Arduino)
add_dependencies(LUFA Arduino)

# Add USB Host Shield Library
file(GLOB LIB_USB_HOST_SHIELD_FILES
    "${CMAKE_SOURCE_DIR}/lib/USB Host Shield/*.cpp"
    "${CMAKE_SOURCE_DIR}/lib/USB Host Shield/*.h"
)
add_library(USB_Host_Shield STATIC "${LIB_USB_HOST_SHIELD_FILES}") 
target_include_directories(USB_Host_Shield PUBLIC "${CMAKE_SOURCE_DIR}/lib/USB Host Shield")
target_link_libraries(USB_Host_Shield Arduino)
add_dependencies(USB_Host_Shield Arduino)

# Add the main firmware files
file(GLOB FIRMWARE_FILES
    "${CMAKE_SOURCE_DIR}/*.cpp"
    "${CMAKE_SOURCE_DIR}/*.c"
    "${CMAKE_SOURCE_DIR}/*.h"
)

# Build the master firmware (without Steel Battalion controller support)
add_executable(ogx360_32u4_master.elf "${FIRMWARE_FILES}")
target_compile_definitions(ogx360_32u4_master.elf PRIVATE "-DDISABLE_BATTALION ")
add_dependencies(ogx360_32u4_master.elf Arduino)
add_dependencies(ogx360_32u4_master.elf LUFA)
add_dependencies(ogx360_32u4_master.elf USB_Host_Shield)
target_link_libraries(ogx360_32u4_master.elf Arduino)
target_link_libraries(ogx360_32u4_master.elf LUFA)
target_link_libraries(ogx360_32u4_master.elf USB_Host_Shield)

# Build the Steel Battalion master firmware (without Xbox One controller support)
add_executable(ogx360_32u4_master_steelbattalion.elf "${FIRMWARE_FILES}")
target_compile_definitions(ogx360_32u4_master_steelbattalion.elf PRIVATE "-DDISABLE_WIREDXBOXONE")
add_dependencies(ogx360_32u4_master_steelbattalion.elf Arduino)
add_dependencies(ogx360_32u4_master_steelbattalion.elf LUFA)
add_dependencies(ogx360_32u4_master_steelbattalion.elf USB_Host_Shield)
target_link_libraries(ogx360_32u4_master_steelbattalion.elf Arduino)
target_link_libraries(ogx360_32u4_master_steelbattalion.elf LUFA)
target_link_libraries(ogx360_32u4_master_steelbattalion.elf USB_Host_Shield)

# Build the slave firmware
add_executable(ogx360_32u4_slave.elf "${FIRMWARE_FILES}")
target_compile_definitions(ogx360_32u4_slave.elf PRIVATE "-DCOMPILE_SLAVE")
add_dependencies(ogx360_32u4_slave.elf Arduino)
add_dependencies(ogx360_32u4_slave.elf LUFA)
add_dependencies(ogx360_32u4_slave.elf USB_Host_Shield)
target_link_libraries(ogx360_32u4_slave.elf Arduino)
target_link_libraries(ogx360_32u4_slave.elf LUFA)
target_link_libraries(ogx360_32u4_slave.elf USB_Host_Shield)
