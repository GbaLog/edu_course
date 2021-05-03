#ifndef APPLICATION_H
#define APPLICATION_H

#include "connection_manager.h"
#include "client_handler.h"
#include <unordered_map>

// Класс, с которого начинается жизнь сервера
// Содержит в себе менеджер подключений(другими словами TCP-сервер),
//  а также карту с идентификатора подключения на его обработчик
// Он сам является посредником между менеджером подключений и обработчиками
// Это необходимо, чтобы избежать высокой связанности обработчика и сервера,
//  они не должны друг об друге знать
class application : public connection_manager_user,
                    public client_handler_owner
{
public:
  application();

  int run(const std::string & ip, uint32_t port);

  // connection_manager_user interface
  void on_connection(connection_id id) override;
  void on_connection_closed(connection_id id) override;
  void on_connection_read(connection_id id, buffer_type buf) override;

  // client_handler_owner interface
  void on_send_command(connection_id id, command cmd) override;

public:
  connection_manager conn_manager;
  std::unordered_map<connection_id, client_handler_ptr> conns;
};

#endif // APPLICATION_H
