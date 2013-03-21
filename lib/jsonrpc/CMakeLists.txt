file(GLOB_RECURSE jsonrpc_source *.cpp)
file(GLOB jsonrpc_header *.h)
file(GLOB connector_header connectors/*.h)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package( Boost 1.44.0 COMPONENTS system )
find_package(CURL REQUIRED)

add_library(jsonrpc SHARED ${jsonrpc_source})

target_link_libraries(jsonrpc json mongoose ${CURL_LIBRARIES} ${Boost_LIBRARIES})

install(FILES ${jsonrpc_header} DESTINATION include/jsonrpc) 
install(FILES ${connector_header} DESTINATION include/jsonrpc/connectors) 

install(TARGETS jsonrpc LIBRARY DESTINATION lib
                      ARCHIVE DESTINATION lib
                      RUNTIME DESTINATION bin)

