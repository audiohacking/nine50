cmake_minimum_required(VERSION 3.16)

# Name of the plugin
project(NINE50 VERSION 1.0.0)

# Option to build tests
option(BUILD_TESTING "Build unit tests" ON)

# Use JUCE as a submodule via FetchContent
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        origin/master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(juce)

# Add the plugin
add_subdirectory(Source)
