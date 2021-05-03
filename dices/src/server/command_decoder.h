#ifndef COMMAND_DECODER_H
#define COMMAND_DECODER_H

#include "common_types.h"

// Интерфейс пользователя декодера
// Требуется, т.к. декодер потоковый и гораздо удобнее реализовать такое на неком обратном вызове
struct command_decoder_user
{
  virtual ~command_decoder_user() = default;
  // Вызывается, если команда была успешно декодирована
  virtual void on_decoded_command(command cmd) = 0;
  // Вызывается, если произошла ошибка декодирования
  virtual void on_decode_error() = 0;
};

// Класс для декодирования команд
// Является потоковым декодером,
// т.к. TCP может посылать данные хоть по одному байту и нужно уметь это обрабатывать
class command_decoder
{
public:
  explicit command_decoder(command_decoder_user & user);

  // Добавить буфер и попытаться сдекодировать
  void add_buffer_and_try_decode(buffer_type buf);

private:
  command_decoder_user & user;
  std::string buffer;

  void decode_command(std::string_view str);
  static command::arguments_type decode_args(std::string_view str);
};

#endif // COMMAND_DECODER_H
