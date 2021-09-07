cmake_minimum_required(VERSION 3.15)
project(FIRTesting VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(modules/JUCE)
juce_add_modules(modules/chowdsp_utils)

juce_add_console_app(FIRTesting
    PRODUCT_NAME "FIRTesting"
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/cmake/")

enable_language(Rust)
include(CMakeCargo)
    
add_subdirectory(src/rustfirlib)
add_custom_command(TARGET FIRTesting PRE_BUILD
    COMMAND cbindgen -d --lang c++ -o rust_fir.h .
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/rustfirlib/"
    COMMENT "Generating Rust to C++ bindings..."
)

# JUCE stuff
juce_generate_juce_header(FIRTesting)
add_subdirectory(src)

target_compile_definitions(FIRTesting PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    CHOWDSP_USE_FOLEYS_CLASSES=0
    JucePlugin_Manufacturer="chowdsp"
    JucePlugin_VersionString="1.0.0"
)

target_link_libraries(FIRTesting PRIVATE
    juce::juce_dsp
    chowdsp_utils
    rustfirlib
)

if(WIN32)
    target_link_libraries(FIRTesting PRIVATE userenv)
endif()

add_custom_command(TARGET FIRTesting
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E echo "copying $<TARGET_FILE:FIRTesting> to ${PROJECT_BINARY_DIR}/FIRTesting"
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:FIRTesting> ${PROJECT_BINARY_DIR}/FIRTesting)

# EXPERIMENTAL: try compiling with AVX SIMD support...
include(CheckCXXCompilerFlag)
if(MSVC)
    CHECK_CXX_COMPILER_FLAG("/arch:AVX2" COMPILER_OPT_ARCH_AVX_SUPPORTED)
    if(COMPILER_OPT_ARCH_AVX_SUPPORTED)
        target_compile_options(FIRTesting PUBLIC /arch:AVX2)
    endif()
else()
    CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
    if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
        target_compile_options(FIRTesting PUBLIC -march=native)
    endif()
endif()
