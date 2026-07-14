cmake_minimum_required(VERSION 3.20)
project(QuarkStoryXT VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
# nlohmann/json is header-only, include manually
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Quark XTensions SDK paths (adjust to your installation)
set(QUARK_SDK_INC "C:/Quark/XTensionsSDK/include")
set(QUARK_SDK_LIB "C:/Quark/XTensionsSDK/lib")
include_directories(${QUARK_SDK_INC})
link_directories(${QUARK_SDK_LIB})

# Source files
set(SOURCES
    src/ApiClient.cpp
    src/StoryPalette.cpp
    src/XTMain.cpp
)

# Windows resource file
if(WIN32)
    set(SOURCES ${SOURCES} src/dialog.rc)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    CURL::libcurl
    XTInterface       # Quark SDK library
    # Additional Windows libs
    $<$<PLATFORM_ID:Windows>:ws2_32>
)

# Advanced operations module (Phase 2 stub)
add_library(StoryOperations STATIC
    src/StoryOperationsImpl.cpp
)
target_include_directories(StoryOperations PUBLIC include)
target_link_libraries(StoryOperations PRIVATE nlohmann_json::nlohmann_json)

# Link all libraries to main extension
target_link_libraries(${PROJECT_NAME} PRIVATE StoryOperations)

# Set output name (Quark expects .xnt extension on Windows)
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".xnt"
)