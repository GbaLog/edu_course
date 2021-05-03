#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include "network_utils.h"
#include <vector>
#include <string>
#include <map>

// Абстракция connection_id - уникальный с точки зрения системы идентификатор,
//  который используется как ключ у TCP-сервера
using connection_id = SOCKET;
// Буфер, используемый для хранения данных, принятых от клиента или посылаемых ему
using buffer_type = std::vector<uint8_t>;

// Команда имеет следующий вид:
//  type:arg1=val1;arg2=val2\n
// Ключи могут повторяться, но порядок их не должен иметь значения
// Если нужно сохранить порядок, то следует дополнительно закодировать это в одном ключе
struct command
{
  std::string type;
  using arguments_type = std::multimap<std::string, std::string>;
  arguments_type args;
};

#endif // COMMON_TYPES_H
