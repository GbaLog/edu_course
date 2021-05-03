#include "connection_manager.h"
#include <iostream>
#include "network_utils.h"

connection_manager::connection_manager(connection_manager_user & user) :
  user(user),
  server_socket(INVALID_SOCKET),
  run(false)
{}

bool connection_manager::start(const std::string & ip, uint16_t port)
{
  // Проверяем, что сервер уже запущен.
  // NOTE: для перезапуска сервера на другом адресе/порту, нужно сначала вызвать функцию stop.
  if (run == true)
  {
    std::cerr << "server already started" << std::endl;
    return false;
  }

  SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET)
  {
    print_last_error("server socket");
    return false;
  }

  sockaddr_in in;
  ::memset(&in, 0, sizeof(in));
  in.sin_family = AF_INET;
  in.sin_addr.s_addr = ::inet_addr(ip.c_str());
  in.sin_port = ::htons(port);
  if (::bind(sock, reinterpret_cast<sockaddr *>(&in), sizeof(in)) == SOCKET_ERROR)
  {
    print_last_error("bind server socket");
    return false;
  }

  if (::listen(sock, 20) == SOCKET_ERROR)
  {
    print_last_error("listen server socket");
    return false;
  }

  server_socket = sock;
  run = true;
  return run_loop();
}

void connection_manager::stop()
{
  run = false;
}

void connection_manager::close_connection(connection_id id)
{
  // Проверяем, что у нас есть такой клиент и отключаем его
  auto it = clients.find(id);
  if (it == clients.end())
    return;

  connection_data & data = it->second;
  handle_disconnect(id, data);
}

void connection_manager::write_to_connection(connection_id id, buffer_type buf)
{
  // Проверяем, что у нас есть такой клиент и добавляем ему буфер на запись
  auto it = clients.find(id);
  if (it == clients.end())
    return;

  connection_data & data = it->second;
  data.write_buf.push(std::move(buf));
}

void connection_manager::print_last_error(const std::string & text)
{
  auto reason = last_network_error_message();
  std::cerr << "Error text: " << text << ", reason: " << reason << std::endl;
}

bool connection_manager::run_loop()
{
  //TODO: использовать IOCP на Windows и epoll на Linux если нужно будет больше производительности

  fd_set read_fds;
  fd_set write_fds;
  fd_set except_fds;

  // select будет засыпать на секунду
  timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  // Цикл работает пока нет ошибок и сервер запущен
  while (run)
  {
    // Удаляем отключённые сокеты
    process_disconnecting();

    // Подготавливаем каждый fd_set
    prepare_fds(read_fds, write_fds, except_fds);

    int res = ::select(0, &read_fds, &write_fds, &except_fds, &tv);
    switch (res)
    {
    // Произошла ошибка в select
    case SOCKET_ERROR:
      print_last_error("select");
      return false;
    // Ничего не произошло
    case 0:
      continue;
    default:
      // Сначала обрабатываем серверный сокет, хотя это не имеет особой пользы,
      // клиенты могут обрабатываться раньше
      if (FD_ISSET(server_socket, &read_fds))
      {
        handle_accept();
      }

      if (FD_ISSET(server_socket, &except_fds))
      {
        print_last_error("server sock");
        return false;
      }

      for (auto & [client, data] : clients)
      {
        if (FD_ISSET(client, &read_fds))
        {
          handle_read(client, data);
        }

        if (FD_ISSET(client, &write_fds))
        {
          handle_write(client, data);
        }

        if (FD_ISSET(client, &except_fds))
        {
          handle_disconnect(client, data);
        }
      }
    }
  }

  // Если цикл закончился, то чистим все соединения
  for (auto & [client, data] : clients)
  {
    handle_disconnect(client, data);
  }
  process_disconnecting();
  return true;
}

void connection_manager::process_disconnecting()
{
  // Проходим по всем для удаления и удаляем их, предварительно отдавая клиенту все данные,
  //  которые успели прийти и не были ещё отданы
  for (auto & [client, data] : to_delete)
  {
    clients.erase(client);
    flush_data(client, data);
    user.on_connection_closed(client);
  }
  to_delete.clear();
}

void connection_manager::prepare_fds(fd_set & read_fds, fd_set & write_fds, fd_set & except_fds)
{
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&except_fds);

  FD_SET(server_socket, &read_fds);
  FD_SET(server_socket, &except_fds);

  for (auto & [client, data] : clients)
  {
    FD_SET(client, &read_fds);
    // Добавляем если только есть что писать
    if (data.write_buf.empty() == false)
      FD_SET(client, &write_fds);
    FD_SET(client, &except_fds);
  }
}

void connection_manager::handle_accept()
{
  sockaddr_in client_addr;
  ::memset(&client_addr, 0, sizeof(client_addr));
  socklen_t addr_len = sizeof(client_addr);

  // Принимаем нового клиента и делаем сокет неблокирующим,
  //  чтобы вызовы send/recv не были блокирующими
  SOCKET client = ::accept(server_socket,
                           reinterpret_cast<sockaddr *>(&client_addr), &addr_len);
  if (client == INVALID_SOCKET)
  {
    print_last_error("accept");
    return;
  }

  u_long val = 1;
#ifdef WIN32
  if (::ioctlsocket(client, FIONBIO, &val) == SOCKET_ERROR)
#else
  if (::ioctl(client, FIONBIO, &val) == SOCKET_ERROR)
#endif
  {
    ::closesocket(client);
    print_last_error("ioctrlsocket");
    return;
  }

  std::cout << "accepted from: " << get_peer_address(client_addr) << std::endl;

  // Это ок, т.к. клиенты ещё не начали обрабатываться
  auto [it, ok] = clients.insert(std::make_pair(client, connection_data{}));
  if (ok == false)
  {
    std::cerr << "Cannot insert new peer" << std::endl;
    ::closesocket(client);
    return;
  }

  // Оповещаем пользователя
  connection_data & data = it->second;
  data.address = client_addr;
  user.on_connection(client);
}

void connection_manager::handle_read(SOCKET client, connection_data & data)
{
  buffer_type buf;
  ssize_t received_count = 0;

  // Алгоритм:
  // Принимаем по 256 байт, если операция может быть блокирована, то прерываем цикл
  // Если принято 0 байт, значит клиент отключился, сообщаем об этом
  // Иначе добавляем в буфер и пробуем принять снова
  do
  {
    buf.resize(256);

    received_count = ::recv(client, reinterpret_cast<char *>(buf.data()), buf.size(), 0);
    if (received_count < 0)
    {
      int err = net_error();
      if (err == NetWouldBlock || err == NetAgain)
        break;
      handle_disconnect(client, data);
      return;
    }
    else if (received_count == 0)
    {
      handle_disconnect_remote(client, data);
      return;
    }

    // Избавляемся от нулей в конце
    buf.resize(received_count);
    data.read_buf.push(std::move(buf));
    buf.clear();
  } while (received_count > 0);

  // Посылаем данные клиенту
  flush_data(client, data);
}

void connection_manager::handle_write(SOCKET client, connection_data & data)
{
  // Нечего делать
  if (data.write_buf.empty())
    return;

  // Пытаемся послать, если может быть блокирована, то просто ничего не делаем
  // Если записали не весь буфер, то вставляем его обратно,
  // иначе извлекаем из очереди буфер и забываем о нём
  auto buf = std::move(data.write_buf.front());
  int res = ::send(client, reinterpret_cast<const char *>(buf.data()), buf.size(), 0);
  // Not sent at all
  if (res < 0)
  {
    int err = net_error();
    if (err == NetWouldBlock || err == NetAgain)
    {
      data.write_buf.front() = std::move(buf);
    }
    else
    {
      handle_disconnect(client, data);
    }
  }
  // Not full buffer sent, write back remain data
  else if (res != buf.size())
  {
    buf.erase(buf.begin(), buf.begin() + res);
    data.write_buf.front() = std::move(buf);
  }
  // All fine
  else
  {
    data.write_buf.pop();
  }
}

void connection_manager::handle_disconnect(SOCKET client, connection_data & data)
{
  // Закрываем сокет, дабы не принимать по нему больше сообщений//
  ::closesocket(client);

  std::cout << "disconnect peer with address: " << get_peer_address(data.address)
            << std::endl;
  to_delete.emplace(client, std::move(data));
}

void connection_manager::handle_disconnect_remote(SOCKET client, connection_data & data)
{
  // Закрываем сокет, дабы не принимать по нему больше сообщений//
  ::closesocket(client);

  std::cout << "peer with address: " << get_peer_address(data.address)
            << " closed connection" << std::endl;
  to_delete.emplace(client, std::move(data));
}

void connection_manager::flush_data(connection_id id, connection_data & data)
{
  // Чистим очередь входящих сообщений, отдавая их пользователю
  while (data.read_buf.empty() == false)
  {
    user.on_connection_read(id, std::move(data.read_buf.front()));
    data.read_buf.pop();
  }
}

// Функция преобразовывает стуктуру с IPv4 адресом в строку
std::string connection_manager::get_peer_address(const sockaddr_in & addr)
{
  std::string ret = ::inet_ntoa(addr.sin_addr);
  ret += ":";
  ret += std::to_string(::ntohs(addr.sin_port));
  return ret;
}
