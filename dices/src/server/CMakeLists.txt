cmake_minimum_required(VERSION 3.17)

project(roll_srv)

set(SRC_LIST
  connection_manager.cpp
  connection_manager.h
  client_handler.h
  network_utils.h
  common_types.h

  command_decoder.cpp
  command_decoder.h
  command_encoder.h

  application.cpp
  application.h)

add_executable(${PROJECT_NAME} main.cpp ${SRC_LIST})

if (${WIN32})
  target_link_libraries(${PROJECT_NAME} PRIVATE wsock32 ws2_32)
endif()
