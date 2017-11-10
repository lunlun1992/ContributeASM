cmake_minimum_required(VERSION 2.8)
project(ContributeASM)

message("DEBUG: CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message("DEBUG: CMAKE_SYSTEM_NAME      = ${CMAKE_SYSTEM_NAME}")
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#find asm compiler
find_package (Yasm)
if(NOT YASM_FOUND)
    message(FATAL_ERROR "Error: Yasm must be installed to compile assembly files")
endif()

# System architecture detection
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSPROC)
set(X86_ALIASES x86 i386 i686 x86_64 amd64)
set(ARM_ALIASES armv6l armv7l)
list(FIND X86_ALIASES "${SYSPROC}" X86MATCH)
list(FIND ARM_ALIASES "${SYSPROC}" ARMMATCH)
set(POWER_ALIASES ppc64 ppc64le)
list(FIND POWER_ALIASES "${SYSPROC}" POWERMATCH)
if("${SYSPROC}" STREQUAL "" OR X86MATCH GREATER "-1")
    if("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
        set(X86_64 TRUE)
        message(STATUS "Detected x86_64 target processor")
    else()
        set(X86_32 TRUE)
        message(STATUS "Detected x86_32 target processor")
    endif()
elseif(POWERMATCH GREATER "-1")
    message(STATUS "Detected POWER target processor")
    set(POWER 1)
    if("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
        set(PPC64 1)
        message(STATUS "Detected POWER PPC64 target processor")
    endif()
elseif(ARMMATCH GREATER "-1")
    message(STATUS "Detected ARM target processor")
    set(ARM 1)
else()
    message(STATUS "CMAKE_SYSTEM_PROCESSOR value `${CMAKE_SYSTEM_PROCESSOR}` is unknown")
    message(STATUS "Please add this value near ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}")
endif()
if(X86_32)
    message(FATAL_ERROR "Error: This Encoder can only be built and used on 64bit machine") 
endif()

if(NOT ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l") )
    if(YASM_FOUND)
        #define asm sources
        set(YASM_NAMES
            intrapred8.asm
            const-a.asm
        )
        if(WIN32)
            set(YASM_ARGS -f win64 -m amd64 -DPIC -DARCH_X86_64=1 -DHAVE_ALIGNED_STACK=0 -DX265_NS=lent -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8)
        elseif(APPLE)
            set(YASM_ARGS -f macho64 -m amd64 -DPIC -DARCH_X86_64=1 -DHAVE_ALIGNED_STACK=0 -DX265_NS=lent -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8)
        else()#Linux
            set(YASM_ARGS -f elf64 -m amd64 -DPIC -DARCH_X86_64=1 -DHAVE_ALIGNED_STACK=0 -DX265_NS=lent -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8)
        endif()

        #compile all asm files
        foreach(_asm_file ${YASM_NAMES})
            set(YASM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${_asm_file}")
            get_filename_component(BASENAME ${YASM_SRC} NAME_WE)
            set(YASM_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}.o")
            add_custom_command(
                OUTPUT ${YASM_OBJ}
                COMMAND "${YASM_EXECUTABLE}"
                ARGS ${YASM_ARGS} -o ${YASM_OBJ} ${YASM_SRC}
                DEPENDS ${YASM_SRC}
            )
            set(YASM_OBJECTS ${YASM_OBJECTS} ${YASM_OBJ})
        endforeach()
    endif(YASM_FOUND)
endif(NOT ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "armv7l") )

set(SOURCE C_functions.c)

if(WIN32)
    add_definitions(-D_MBCS -D_CRT_SECURE_NO_WARNINGS)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -O2 -fPIC -mavx2 -msse4.1 -fvisibility=hidden -DNDEBUG -Wno-unused-result -Wl,-Bsymbolic,-znoexecstack")
endif()


add_library(libCFuncs STATIC ${SOURCE} ${YASM_OBJECTS})
add_executable(contribute main.c)
target_link_libraries(contribute libCFuncs)


