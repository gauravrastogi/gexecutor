cmake_minimum_required (VERSION 2.6)
set (CMAKE_CXX_COMPILER  /usr/bin/g++)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Werror")
set (GEXECUTOR_VERSION 1.1.0)

project(gexecutorlib)

include_directories(${PROJECT_SOURCE_DIR}/../)
set (GEXECUTOR_COMMON_SOURCES gexecutor.cc gtaskq.cc)
add_library(gexecutor-common SHARED ${GEXECUTOR_COMMON_SOURCES})
target_link_libraries(gexecutor-common pthread)
set_target_properties(gexecutor-common PROPERTIES VERSION ${GEXECUTOR_VERSION} SOVERSION 1)

set (GEXECUTOR_SOURCES gasync_executor.cc gsync_executor.cc 
		gsync_worker_thread.cc gexecutor_service_base.cc gexecutor_service.cc)
add_library(gexecutor SHARED ${GEXECUTOR_SOURCES})
target_link_libraries(gexecutor gexecutor-common event pthread)
set_target_properties(gexecutor PROPERTIES VERSION ${GEXECUTOR_VERSION} SOVERSION 1)

install (FILES gexecutor.h gexecutor_service.h gtaskq.h gexecutor_service_base.h
				gexecutor_common.h deferred_task.h
         DESTINATION include/gexecutor)

install(TARGETS gexecutor
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib/static)

install(TARGETS gexecutor-common
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib/static)

add_subdirectory(test)
add_subdirectory(asio)

