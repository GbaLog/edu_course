#include "command_decoder.h"

namespace
{

// Обрезает строку справа, удаляя лишние символы перевода каретки
template<class T>
void trim_right(T & val)
{
  while (val.back() == '\n' || val.back() == '\r')
  {
    if constexpr (std::is_same_v<T, std::string_view>)
      val.remove_suffix(1);
    else
      val.pop_back();
  }
}

}

command_decoder::command_decoder(command_decoder_user & user) :
  user(user)
{}

void command_decoder::add_buffer_and_try_decode(buffer_type buf)
{
  buffer.append(buf.begin(), buf.end());

  // Don't accept messages longer than 1.5Kb
  if (buffer.size() > 1536)
  {
    buffer.clear();
    user.on_decode_error();
  }

  auto it = buffer.find('\n');
  while (it != buffer.npos)
  {
    std::string_view tmp{buffer.data(), it};
    if (tmp.empty() == false)
    {
      decode_command(tmp);
    }
    buffer.erase(0, it + 1);
    it = buffer.find('\n');
  }
}

void command_decoder::decode_command(std::string_view str)
{
  command cmd;
  auto cmd_type_sep = str.find(':');
  // means string is a command by itself
  if (cmd_type_sep == str.npos)
  {
    cmd.type = std::string{str.data(), str.size()};
    trim_right(cmd.type);
    user.on_decoded_command(std::move(cmd));
    return;
  }
  else
    cmd.type = std::string{str.data(), cmd_type_sep};
  str.remove_prefix(cmd_type_sep + 1);

  cmd.args = decode_args(str);
  user.on_decoded_command(std::move(cmd));
}

command::arguments_type command_decoder::decode_args(std::string_view str)
{
  // arguments looks like this: a=b;c=d\n
  command::arguments_type args;

  auto eq_mark = str.find('=');
  while (eq_mark != str.npos)
  {
    auto end_arg = str.find_first_of(";\n");
    std::string_view key = str.substr(0, eq_mark);
    std::string_view value = str.substr(eq_mark + 1, end_arg - eq_mark - 1);
    trim_right(value);
    args.emplace(std::string(key.data(), key.size()), std::string(value.data(), value.size()));
    if (end_arg == str.npos)
      break;
    str.remove_prefix(end_arg + 1);
    eq_mark = str.find('=');
  }
  return args;
}
