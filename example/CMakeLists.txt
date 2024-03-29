# 指定cmake最低编译版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
# 指定工程的名称
PROJECT(DoubleBufferExample)
set(CMAKE_CXX_STANDARD 17)
#指定头文件目录位置
INCLUDE_DIRECTORIES(../)
# INCLUDE_DIRECTORIES(/usr/local/Cellar/protobuf/3.15.5/include)
# #添加共享库搜索路径
# LINK_DIRECTORIES(/usr/local/Cellar/protobuf/3.15.5/lib)
#GTEST
# INCLUDE_DIRECTORIES(/usr/local/include)
# LINK_DIRECTORIES(/usr/local/lib)

cmake_policy(SET CMP0077 NEW)


include(FetchContent)
# FetchContent_GetProperties(googletest)
fetchcontent_declare(yaml
    # use auto sync mirror repo maintained by ourself
    # GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_REPOSITORY https://e.coding.net/muca/mirrorlib/yaml-cpp.git
    GIT_TAG yaml-cpp-0.6.3
    )

option(YAML_CPP_BUILD_TESTS "Enable testing" OFF)
option(YAML_CPP_BUILD_TOOLS "Enable parse tools" OFF)
option(YAML_CPP_BUILD_CONTRIB "Enable contrib stuff in library" OFF)
option(YAML_CPP_INSTALL "Enable generation of install target" OFF)
# fetchcontent_makeavailable(yaml)
if(NOT yaml_POPULATED)
  FetchContent_Populate(yaml)
  add_subdirectory(${yaml_SOURCE_DIR} ${yaml_BINARY_DIR})
endif()


AUX_SOURCE_DIRECTORY(../ DIR_SRCS)

SET(SRC
${DIR_SRCS}
)

#生成可执行文件
ADD_EXECUTABLE(${PROJECT_NAME} example.cpp ${SRC})
# ADD_EXECUTABLE(${PROJECT_NAME} testcpp.cpp ${SRC})
#为hello添加共享库链接
IF (APPLE)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} yaml-cpp pthread)
	MESSAGE(STATUS "Now is Apple")
ELSEIF (UNIX)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} yaml-cpp -ldl pthread)
	MESSAGE(STATUS "Now is UNIX-like OS's.")
ENDIF ()



