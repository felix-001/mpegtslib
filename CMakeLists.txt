cmake_minimum_required (VERSION 2.8)

set( APPNAME tsmuxer )
project( ${APPNAME} )
add_definitions( "-Wall -g" )
include_directories( ./src )
AUX_SOURCE_DIRECTORY( ./src DIR_SRCS)
ADD_EXECUTABLE( ${APPNAME} ${DIR_SRCS} )
target_link_libraries( ${APPNAME} pthread )
