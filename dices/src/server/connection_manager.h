#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "network_utils.h"
#include "common_types.h"
#include <unordered_map>
#include <string>
#include <queue>

// В файле представлен класс для управления асинхронным TCP-сервером
// На данный момент поддерживает только IPv4-соединения

// Пользователь TCP-сервера, получает уведомления о создании и уничтожении соединения,
//  а также об том, что получено сообщение
struct connection_manager_user
{
  virtual ~connection_manager_user() = default;
  // Вызывается, когда установлено новое соединение
  virtual void on_connection(connection_id id) = 0;
  // Вызывается, когда соединение закрывается по просьбе удалённой стороны
  virtual void on_connection_closed(connection_id id) = 0;
  // Вызывается, когда удалённая сторона прислыает сообщение
  virtual void on_connection_read(connection_id id, buffer_type buf) = 0;
};

// Класс TCP-сервера, имеет довольно аскетичный интерфейс.
class connection_manager
{
public:
  connection_manager(connection_manager_user & user);

  // Функции передаются IP-адрес и порт, на которые сервер должен принимать соединения
  // Функция блокирует поток выполнения в случае успешного старта и в конце возвращает true
  // Если запуск неуспешен, то возвращает false и не блокирует поток
  // Может вернуть false во время работы, если произойдёт какая-то серьёзная ошибка
  [[nodiscard]]
  bool start(const std::string & ip, uint16_t port);
  // Останавливает сервер.
  void stop();

  // Закрыть соединение со своей стороны
  void close_connection(connection_id id);
  // Послать удалённой стороне некое сообщение
  void write_to_connection(connection_id id, buffer_type buf);

private:
  connection_manager_user & user;
  SOCKET server_socket;
  bool run;

  // Старуктура, хранящая в себе различные данные, связанные с соединением
  struct connection_data
  {
    std::queue<buffer_type> write_buf;
    std::queue<buffer_type> read_buf;
    sockaddr_in address;
  };

  // Карта сокета на данные соединения
  using map_clients = std::unordered_map<SOCKET, connection_data>;
  // Хранит текущих клиентов
  map_clients clients;
  // Т.к. соединения обрабатываются в цикле, мы не может просто удалять их и идти дальше по циклу,
  //  поэтому если нам нужно закрыть соединение, мы заносим его в эту карту и,
  //  на следующем проходе цикла сначала будут удалены все клиенты, а потом уже начнётся обработка новых
  map_clients to_delete;

  void print_last_error(const std::string & text);
  bool run_loop();
  void process_disconnecting();
  void prepare_fds(fd_set & read_fds, fd_set & write_fds, fd_set & except_fds);
  void handle_accept();
  void handle_read(SOCKET client, connection_data & data);
  void handle_write(SOCKET client, connection_data & data);
  void handle_disconnect(SOCKET client, connection_data & data);
  void handle_disconnect_remote(SOCKET client, connection_data & data);
  void flush_data(connection_id id, connection_data & data);
  std::string get_peer_address(const sockaddr_in & addr);
};

#endif // CONNECTION_MANAGER_H
