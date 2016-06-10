cmake_minimum_required(VERSION 2.6)
find_package(PkgConfig)

PROJECT(shekedr C)

pkg_search_module(GSTREAMER1 gstreamer-1.0)

ADD_DEFINITIONS(-Os -Wall --std=gnu99 -g3 -Wmissing-declarations)

ADD_EXECUTABLE(shekedr cam_encoding.c  configparse.c  main.c  record.c)
TARGET_LINK_LIBRARIES(shekedr ${GSTREAMER1_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(shekedr PUBLIC ${GSTREAMER1_INCLUDE_DIRS})
TARGET_COMPILE_OPTIONS(shekedr PUBLIC ${GSTREAMER1_CFLAGS})

INSTALL(TARGETS shekedr RUNTIME DESTINATION sbin)
