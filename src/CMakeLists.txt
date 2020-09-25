
file(GLOB SRC *.cpp)
file(GLOB HPP *.h)

exec_program(${PROTOBUF_PROTOC_EXECUTABLE} ARGS
    --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}
    --js_out=import_style=commonjs,binary:${CMAKE_CURRENT_SOURCE_DIR}
    --python_out=${CMAKE_CURRENT_SOURCE_DIR}/protobuf_generated/python/PbSignal
    -I${CMAKE_CURRENT_SOURCE_DIR} Signal.proto)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ZeroMQ_INCLUDE_DIR})
include_directories(${dbcppp_INCLUDE_DIR})
include_directories(${Protobuf_INCLUDE_DIRS})

add_library(CanLogSyncServSignal SHARED Signal.pb.cc)
add_executable(CanLogSyncServ ${SRC} ${HPP})

target_link_libraries(CanLogSyncServ CanLogSyncServSignal ${dbcppp_LIBRARY} ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} pthread dbcppp)

install(TARGETS CanLogSyncServ DESTINATION bin)
install(TARGETS CanLogSyncServSignal DESTINATION lib)
install(FILES Signal_pb.js Signal.pb.h DESTINATION include/CanLogSyncServ)
install(CODE "execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/protobuf_generated/python COMMAND python3 setup.py install)")

