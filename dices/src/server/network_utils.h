#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

// Этот файл содержит функции и структуры для унификации интерфейса на платформах Windows и Linux
// В частности:
//  - функция last_network_error_message возвращает текст последней ошибки,
//  - перечисление содержит несколько кодов ошибок, различающихся на разных платформах,
//  - функция net_error возвращает код последней ошибки,
//  - так же сделано немного алиасов типов и для Linux создана функция closesocket
//    из Windows, чтобы поддержать унифицированный интерфейс

#include <string_view>

#ifdef WIN32

#include <windows.h>

inline
std::string_view last_network_error_message()
{
  int ec = WSAGetLastError();
  static thread_local char msg_buf[256];
  ::memset(msg_buf, 0, sizeof(msg_buf));

  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr,
                 ec,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 msg_buf,
                 sizeof(msg_buf),
                 nullptr);

  return {msg_buf};
}

enum
{
  NetWouldBlock = WSAEWOULDBLOCK,
  NetAgain = WSAEWOULDBLOCK
};

inline
int net_error() { return WSAGetLastError(); }

using socklen_t = int;

#else
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>

constexpr static int SOCKET_ERROR = -1;
constexpr static int INVALID_SOCKET = -1;

enum
{
  NetWouldBlock = EWOULDBLOCK,
  NetAgain = EAGAIN
};

using SOCKET = int;

inline
int net_error() { return errno; }

inline
int closesocket(SOCKET fd) { return ::close(fd); }

inline
std::string_view last_network_error_message()
{
  return strerror(errno);
}
#endif

#endif // NETWORK_UTILS_H
