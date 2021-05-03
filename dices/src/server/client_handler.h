#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "common_types.h"
#include "command_decoder.h"
#include <memory>

// Интрефейс владельца обработчика
// Нужен для обращения обработчика к своему обладателю для посылки ответа на команду
struct client_handler_owner
{
  virtual ~client_handler_owner() = default;
  virtual void on_send_command(connection_id id, command cmd) = 0;
};

// Обработчик сообщений от клиента
// Наследуется как пользователь декодера команд,
//  т.к. содержит в себе его и ему нужно потоково декодировать команды
// На вход принимает команды, и формирует ответы
struct client_handler : public command_decoder_user
{
  client_handler(connection_id id, client_handler_owner & owner) :
    id(id),
    owner(owner),
    decoder(*this),
    got_handshake(false)
  {}

  // Метод обрабатывает входящий буфер, передавая его в декодер команд
  void data_received(buffer_type buf)
  {
    decoder.add_buffer_and_try_decode(std::move(buf));
  }

  // command_decoder_user interface
  // Метод вызывается декодером, когда он успешно декодирует команду
  // Т.к. класс имеет состояние, то оно здесь проверяется
  // Таким образом, нельзя послать команду, если не было команды hello
  // На данный момент поддерживается только команда roll,
  //  которая генерирует случайное число от 1 до 6
  // На любое незнакомое сообщение отвечает ошибкой
  // Больше на данный момент команд не поддерживается
  void on_decoded_command(command cmd) override
  {
    command to_send;
    if (cmd.type == "hello")
    {
      to_send.type = "ok";
      got_handshake = true;
    }
    else if (!got_handshake)
    {
      to_send.type = "error";
    }
    else if (cmd.type == "roll")
    {
      to_send.type = "won";
      to_send.args.emplace("result", std::to_string(1 + rand() % 6));
    }
    else
    {
      to_send.type = "error";
    }

    owner.on_send_command(id, std::move(to_send));
  }

  // Перехват ошибки декодирования сообщения
  // Посылаем ошибку клиенту
  void on_decode_error() override
  {
    command to_send;
    to_send.type = "error";
    owner.on_send_command(id, to_send);
  }

  const connection_id id;
  client_handler_owner & owner;
  command_decoder decoder;
  bool got_handshake;
};
using client_handler_ptr = std::unique_ptr<client_handler>;

#endif // CLIENT_HANDLER_H
