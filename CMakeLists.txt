cmake_minimum_required(VERSION 3.22)

include(CMakePackageConfigHelpers)

project(ocwm)

#[==[==========================================================
                        CMake Config
==========================================================#]==]

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS ON)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    include(CTest)

    find_package(Doxygen)
    if(Doxygen_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, skipping documentation generation")
    endif()
    
    add_library(coverage_config INTERFACE)
    option(CODE_COVERAGE "Enable coverage reporting" OFF)
    if(CODE_COVERAGE)
        target_compile_options(coverage_config INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags
        )
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
            target_link_options(coverage_config INTERFACE --coverage)
        else()
            target_link_libraries(coverage_config INTERFACE --coverage)
        endif()
    endif()

endif()

#[==[==========================================================
                        Lnx WM
==========================================================#]==]
find_package(X11)

add_executable(ocwm  source/main.cpp source/manager.cpp source/command.cpp)

target_include_directories(ocwm PUBLIC include ${X11_INCLUDE_DIR})
target_link_libraries(ocwm PUBLIC ${X11_LIBRARIES})
target_compile_options(ocwm PUBLIC -Wall)

#[==[==========================================================
                    Build Examples / Tests
==========================================================#]==]
# add_subdirectory(example)
# add_subdirectory(test)
