# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("myPlayer")

# --------------------------------------------------------------------------
# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

IF(MSVC) # Check if we are using the Visual Studio compiler
    SET(CMAKE_CXX_FLAGS "/EHsc")  # deal with MSVC Warning C4530
ENDIF(MSVC)



##dependencies

######################################
#  SDL2 lib setting
######################################
#make sure SDL2.dll is in your PATH.
 
#set(SDL_INCLUDE_DIR "E:/ffmpeg/SDL2-2.0.12/include")
if(WIN32)
	find_path(SDL_INCLUDE_DIR SDL2/SDL.h)
else()
	find_path(SDL_INCLUDE_DIR SDL2/SDL.h)
endif()
	find_library(SDL2_LIBRARY SDL2)
	find_library(SDL_LIBRARY SDL )
	find_library(SDLmain_LIBRARY SDL2main)
#
#link_directories("E:/ffmpeg/SDL2-2.0.12/lib/x64")

######################################
#  FFmpeg lib setting
######################################
#make sure ffmpeg/bin is in your PATH.

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

find_path(AVFILTER_INCLUDE_DIR libavfilter/avfilter.h)
find_library(AVFILTER_LIBRARY avfilter)

#find_path(POSTPROC_INCLUDE_DIR libpostproc/postprocess.h)
#find_library(POSTPROC_LIBRARY postproc)

find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)




######################################
#  information
######################################
message(====PROJECT_INFO====)
message(CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR})
message(PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR})


message(====SDL_INFO====)
message(SDL_INCLUDE_DIR=${SDL_INCLUDE_DIR})
message(SDL_LIBRARY=${SDL_LIBRARY})
message(SDL2_LIBRARY=${SDL2_LIBRARY})
message(SDLmain_LIBRARY=${SDLmain_LIBRARY})

message(====FFMPEG_INFO====)
message("--------------------")
message(AVCODEC_INCLUDE_DIR=${AVCODEC_INCLUDE_DIR})
message(AVCODEC_LIBRARY=${AVCODEC_LIBRARY})
message("--------------------")
message(AVFORMAT_INCLUDE_DIR=${AVFORMAT_INCLUDE_DIR})
message(AVFORMAT_LIBRARY=${AVFORMAT_LIBRARY})
message("--------------------")
message(AVUTIL_INCLUDE_DIR=${AVUTIL_INCLUDE_DIR})
message(AVUTIL_LIBRARY=${AVUTIL_LIBRARY})
message("--------------------")
message(AVDEVICE_INCLUDE_DIR=${AVDEVICE_INCLUDE_DIR})
message(AVDEVICE_LIBRARY=${AVDEVICE_LIBRARY})
message("--------------------")
message(AVFILTER_INCLUDE_DIR=${AVFILTER_INCLUDE_DIR})
message(AVFILTER_LIBRARY=${AVFILTER_LIBRARY})
message("--------------------")
message(POSTPROC_INCLUDE_DIR=${POSTPROC_INCLUDE_DIR})
message(POSTPROC_LIBRARY=${POSTPROC_LIBRARY})
message("--------------------")
message(SWRESAMPLE_INCLUDE_DIR=${SWRESAMPLE_INCLUDE_DIR})
message(SWRESAMPLE_LIBRARY=${SWRESAMPLE_LIBRARY})
message("--------------------")
message(SWSCALE_INCLUDE_DIR=${SWSCALE_INCLUDE_DIR})
message(SWSCALE_LIBRARY=${SWSCALE_LIBRARY})



# Add source to this project's executable.
add_executable (${PROJECT_NAME} 
		src/main.cpp
		src/audio.cpp
		src/video.cpp)

# TODO: Add tests and install targets if needed.

target_include_directories( ${PROJECT_NAME}  
	PRIVATE 
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/libs
		${AVCODEC_INCLUDE_DIR}
		${AVFORMAT_INCLUDE_DIR} 
		${AVUTIL_INCLUDE_DIR} 
		${AVDEVICE_INCLUDE_DIR}
		${AVFILTER_INCLUDE_DIR}
#		${POSTPROC_INCLUDE_DIR}
		${SWRESAMPLE_INCLUDE_DIR}
		${SWSCALE_INCLUDE_DIR}
		${SDL_INCLUDE_DIR}
)

target_link_libraries( ${PROJECT_NAME}
	PRIVATE 
		${AVCODEC_LIBRARY} 
		${AVFORMAT_LIBRARY} 
		${AVUTIL_LIBRARY} 
		${AVDEVICE_LIBRARY}
		${AVFILTER_LIBRARY}
#		${POSTPROC_LIBRARY}
		${SWRESAMPLE_LIBRARY}
		${SWSCALE_LIBRARY}
		${SDL_LIBRARY}
		${SDL2_LIBRARY}
		${SDLmain_LIBRARY}
)

#add_executable (theia2-broadcaster "theia2-broadcaster.cpp" "theia2-broadcaster.h")

# TODO: Add tests and install targets if needed.
