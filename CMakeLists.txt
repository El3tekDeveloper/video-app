cmake_minimum_required(VERSION 3.20)
project(video-app C CXX)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DGL_SILENCE_DEPRECATION)

# FFmpeg setup for Linux
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/thirdparty/ffmpeg)
add_library(FFmpeg INTERFACE)

target_include_directories(FFmpeg INTERFACE
    ${FFMPEG_DIR}/include
)

target_link_directories(FFmpeg INTERFACE
    ${FFMPEG_DIR}/lib
)

target_link_libraries(FFmpeg INTERFACE
    avformat
    avcodec
    swresample
    swscale
    avutil
)

if(UNIX AND NOT APPLE)
    target_link_libraries(FFmpeg INTERFACE
        m pthread dl
    )
endif()

# GLFW setup - building from source
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/thirdparty/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR})

# GLAD setup
add_library(glad STATIC thirdparty/glad/src/glad.c)
target_include_directories(glad PUBLIC thirdparty/glad/include)

# GLM setup (header-only library)
set(GLM_DIR ${CMAKE_SOURCE_DIR}/thirdparty/glm)
add_library(GLM INTERFACE)

target_include_directories(GLM INTERFACE
    ${GLM_DIR}
)

# FreeType setup
set(FREETYPE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/freetype)

# Core FreeType sources
set(FREETYPE_SOURCES
    "${FREETYPE_DIR}/src/autofit/autofit.c"
    "${FREETYPE_DIR}/src/base/ftbase.c"
    "${FREETYPE_DIR}/src/base/ftbbox.c"
    "${FREETYPE_DIR}/src/base/ftbdf.c"
    "${FREETYPE_DIR}/src/base/ftbitmap.c"
    "${FREETYPE_DIR}/src/base/ftcid.c"
    "${FREETYPE_DIR}/src/base/ftfstype.c"
    "${FREETYPE_DIR}/src/base/ftgasp.c"
    "${FREETYPE_DIR}/src/base/ftglyph.c"
    "${FREETYPE_DIR}/src/base/ftgxval.c"
    "${FREETYPE_DIR}/src/base/ftinit.c"
    "${FREETYPE_DIR}/src/base/ftmm.c"
    "${FREETYPE_DIR}/src/base/ftotval.c"
    "${FREETYPE_DIR}/src/base/ftpatent.c"
    "${FREETYPE_DIR}/src/base/ftpfr.c"
    "${FREETYPE_DIR}/src/base/ftstroke.c"
    "${FREETYPE_DIR}/src/base/ftsynth.c"
    "${FREETYPE_DIR}/src/base/ftsystem.c"
    "${FREETYPE_DIR}/src/base/fttype1.c"
    "${FREETYPE_DIR}/src/base/ftwinfnt.c"
    "${FREETYPE_DIR}/src/base/ftdebug.c"
    "${FREETYPE_DIR}/src/bdf/bdf.c"
    "${FREETYPE_DIR}/src/cache/ftcache.c"
    "${FREETYPE_DIR}/src/cff/cff.c"
    "${FREETYPE_DIR}/src/cid/type1cid.c"
    "${FREETYPE_DIR}/src/gzip/ftgzip.c"
    "${FREETYPE_DIR}/src/lzw/ftlzw.c"
    "${FREETYPE_DIR}/src/pcf/pcf.c"
    "${FREETYPE_DIR}/src/pfr/pfr.c"
    "${FREETYPE_DIR}/src/psaux/psaux.c"
    "${FREETYPE_DIR}/src/pshinter/pshinter.c"
    "${FREETYPE_DIR}/src/psnames/psnames.c"
    "${FREETYPE_DIR}/src/raster/raster.c"
    "${FREETYPE_DIR}/src/sdf/sdf.c"
    "${FREETYPE_DIR}/src/sfnt/sfnt.c"
    "${FREETYPE_DIR}/src/smooth/smooth.c"
    "${FREETYPE_DIR}/src/svg/svg.c"
    "${FREETYPE_DIR}/src/truetype/truetype.c"
    "${FREETYPE_DIR}/src/type1/type1.c"
    "${FREETYPE_DIR}/src/type42/type42.c"
    "${FREETYPE_DIR}/src/winfonts/winfnt.c"
)

add_library(freetype STATIC ${FREETYPE_SOURCES})

target_include_directories(freetype PUBLIC
    ${FREETYPE_DIR}/include
)

target_compile_definitions(freetype PRIVATE
    FT2_BUILD_LIBRARY
)

target_link_libraries(freetype PRIVATE z)

# miniaudio (header-only)
add_library(miniaudio INTERFACE)

target_include_directories(miniaudio INTERFACE
    ${CMAKE_SOURCE_DIR}/thirdparty/miniaudio
)

if(UNIX AND NOT APPLE)
    target_link_libraries(miniaudio INTERFACE dl m pthread)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/thirdparty
    ${PROJECT_SOURCE_DIR}/gui
)

# Sources
set(SOURCES
    src/main.cpp
    src/VideoReader.cpp
    src/AudioReader.cpp
    src/VideoRenderer.cpp
    src/miniaudio_impl.cpp
    gui/UI.cpp
    gui/UIRenderer.cpp
)

add_executable(video-app ${SOURCES})

find_package(Threads REQUIRED)

target_link_libraries(video-app PRIVATE 
    glad
    FFmpeg 
    glfw
    GLM
    freetype
    miniaudio
    GL
    ${CMAKE_DL_LIBS}
    Threads::Threads
)

# Copy Assets
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

set(DEST_DIR "${CMAKE_BINARY_DIR}/assets")

add_custom_command(
    OUTPUT "${DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SOURCE_DIR}" "${DEST_DIR}"
    COMMENT "Copying assets folder to build directory"
    VERBATIM
)

add_custom_target(copy_assets ALL DEPENDS "${DEST_DIR}")
