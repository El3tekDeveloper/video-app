cmake_minimum_required(VERSION 3.20)
project(video-app C CXX)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DGLFW_INCLUDE_NONE)
add_definitions(-DGL_SILENCE_DEPRECATION)

# FFmpeg setup
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/thirdparty/ffmpeg)
add_library(FFmpeg INTERFACE)

target_include_directories(FFmpeg INTERFACE
    ${FFMPEG_DIR}/include
)

target_link_directories(FFmpeg INTERFACE
    ${FFMPEG_DIR}/lib
)

target_link_libraries(FFmpeg INTERFACE
    avcodec
    avformat
    avutil
    swscale
    swresample
)

# GLFW setup (pre-built)
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/thirdparty/glfw)
add_library(GLFW INTERFACE)

target_include_directories(GLFW INTERFACE
    ${GLFW_DIR}/include
)

target_link_directories(GLFW INTERFACE
    ${GLFW_DIR}/lib
)

target_link_libraries(GLFW INTERFACE
    glfw3dll
)

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

# Core FreeType sources - now including gzip, lzw, and svg modules
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

# Link zlib for gzip compression support
target_link_libraries(freetype PRIVATE z)

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
    src/VideoRenderer.cpp
    gui/UI.cpp
    gui/UIRenderer.cpp
)

add_executable(video-app ${SOURCES})

target_link_libraries(video-app PRIVATE 
    glad
    FFmpeg 
    GLFW
    GLM
    freetype
    opengl32 
    gdi32
)

# Copy DLLs to output directory
if (WIN32)
    file(GLOB FFMPEG_DLLS "${FFMPEG_DIR}/bin/*.dll")
    
    add_custom_command(TARGET video-app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS}
            $<TARGET_FILE_DIR:video-app>
    )

    add_custom_command(TARGET video-app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GLFW_DIR}/bin/glfw3.dll
            $<TARGET_FILE_DIR:video-app>
    )
endif()