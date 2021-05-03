#include "application.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#ifdef WIN32
#include <windows.h>
#endif
#include "command_encoder.h"

application::application() :
  conn_manager(*this)
{
  std::srand(std::time(nullptr));
}

int application::run(const std::string & ip, uint32_t port)
{
#ifdef WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
  {
    std::cerr << "Cannot start WSA: " << last_network_error_message() << std::endl;
    return EXIT_FAILURE;
  }
#endif

  int ret = EXIT_SUCCESS;
  if (conn_manager.start(ip, port) == false)
  {
    std::cerr << "Cannot start manager" << std::endl;
    ret = EXIT_FAILURE;
  }

#ifdef WIN32
  WSACleanup();
#endif
  return ret;
}

void application::on_connection(connection_id id)
{
  // Добавляем в карту новое соединение
  std::cout << "on connection: " << id << std::endl;
  conns.emplace(id, std::make_unique<client_handler>(id, *this));
}

void application::on_connection_closed(connection_id id)
{
  // Просто удаляем, тут нет каких-то ресурсов, которые нужно дополнительно освобождать
  std::cout << "on connection closed: " << id << std::endl;
  conns.erase(id);
}

void application::on_connection_read(connection_id id, buffer_type buf)
{
  // Проверяем, есть ли такое соединение, и посылаем буфер обработчику
  std::cout << "on connection read: " << id << ", size: " << buf.size() << std::endl;
  auto it = conns.find(id);
  if (it == conns.end())
  {
    std::cerr << "cannot find id: " << id << std::endl;
    return;
  }
  client_handler_ptr & handler = it->second;
  handler->data_received(std::move(buf));
}

void application::on_send_command(connection_id id, command cmd)
{
  // Кодируем команду и посылаем в сервер
  // Не проверяем, есть ли такой id, т.к. сервер сам это проверяет
  buffer_type buf;
  if (command_encoder::encode(cmd, buf) == false)
    return;
  std::cout << "write for id: " << id << ": " << buf.size() << " bytes for: " << cmd.type << std::endl;
  conn_manager.write_to_connection(id, std::move(buf));
}
