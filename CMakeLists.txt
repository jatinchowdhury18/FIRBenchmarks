cmake_minimum_required(VERSION 3.15)
project(FIRTesting VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_console_app(FIRTesting
    PRODUCT_NAME "FIRTesting")

# JUCE stuff
juce_generate_juce_header(FIRTesting)

add_subdirectory(src)

target_compile_definitions(FIRTesting PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(FIRTesting PRIVATE
    juce::juce_dsp)

if(WIN32)
    target_link_libraries(FIRTesting PRIVATE userenv)
endif()
