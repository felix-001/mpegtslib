cmake_minimum_required (VERSION 2.8)

set( APPNAME tsmuxer-sample )
project( ${APPNAME} )
add_definitions( "-Wall -g" )
include_directories( ./src/tslib/ )
include_directories( ./src/sample )
AUX_SOURCE_DIRECTORY( ./src/tslib DIR_SRCS)
add_library( ts ${DIR_SRCS} )

AUX_SOURCE_DIRECTORY( ./src/sample DIR_SAMPLE )
ADD_EXECUTABLE( ${APPNAME} ${DIR_SAMPLE} )
target_link_libraries( ${APPNAME} ts pthread )
