#ifndef COMMAND_ENCODER_H
#define COMMAND_ENCODER_H

#include "common_types.h"

// Кодирует команду в буфер
class command_encoder
{
public:
  static bool encode(const command & cmd, buffer_type & buf)
  {
    if (cmd.type.empty())
      return false;

    buf.insert(buf.end(), cmd.type.begin(), cmd.type.end());
    if (cmd.args.empty())
    {
      buf.push_back('\n');
      return true;
    }
    buf.push_back(':');
    for (const auto & [k, v] : cmd.args)
    {
      buf.insert(buf.end(), k.begin(), k.end());
      buf.push_back('=');
      buf.insert(buf.end(), v.begin(), v.end());
      buf.push_back(';');
    }
    buf.push_back('\n');
    return true;
  }
};

#endif // COMMAND_ENCODER_H
