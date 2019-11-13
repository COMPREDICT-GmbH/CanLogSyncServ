
exec_program(${PROTOBUF_PROTOC_EXECUTABLE} ARGS
	--cpp_out ${CMAKE_CURRENT_SOURCE_DIR}
	--csharp_out ${CMAKE_CURRENT_SOURCE_DIR}
	--java_out ${CMAKE_CURRENT_SOURCE_DIR}
	--js_out ${CMAKE_CURRENT_SOURCE_DIR}
	--objc_out ${CMAKE_CURRENT_SOURCE_DIR}
	--php_out ${CMAKE_CURRENT_SOURCE_DIR}
	--python_out ${CMAKE_CURRENT_SOURCE_DIR}
	--ruby_out ${CMAKE_CURRENT_SOURCE_DIR}
       	-I${CMAKE_CURRENT_SOURCE_DIR} Signal.proto)

file(GLOB SRC *.cpp)
file(GLOB HPP *.h)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ZeroMQ_INCLUDE_DIR})
include_directories(${VectorDBC_INCLUDE_DIR})
include_directories(${Protobuf_INCLUDE_DIRS})

add_executable(CanLogSyncServ Signal.pb.cc ${SRC} ${HPP})

target_link_libraries(CanLogSyncServ ${Vector_DBC_LIBRARY} ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} pthread)

install(TARGETS CanLogSyncServ DESTINATION bin)
install(FILES pb_signal.js pb_signals.js Pb_Signal.php Pb_Signals.php Signal.cs Signal.java Signal_pb2.py Signal.pb.cc Signal.pb.h Signal.pbobjc.h Signal.pbobjc.m Signal_pb.rb DESTINATION lib/CanLogSyncServ)

