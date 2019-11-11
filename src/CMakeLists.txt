
file(GLOB SRC *.cpp)
file(GLOB HPP *.h)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ZeroMQ_INCLUDE_DIR})
include_directories(${VectorDBC_INCLUDE_DIR})

add_executable(CanLogSyncServ ${SRC} ${HPP})

target_link_libraries(CanLogSyncServ ${Vector_DBC_LIBRARY} ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES} pthread)

install(TARGETS CanLogSyncServ DESTINATION bin)

