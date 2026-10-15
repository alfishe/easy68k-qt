// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Port of the original Net.cpp (Winsock 1.1) to BSD sockets / Winsock 2.
// Differences from the original, all bug fixes, are listed in PROGRESS.md
// (13.1d decisions); "Net.cpp" comments name the original function.

#include "easym68k/sim/std_network_io.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace easym68k::sim {

namespace {

#ifdef _WIN32
using Socket = SOCKET;
const Socket kInvalid = INVALID_SOCKET;
int LastError() {
  return WSAGetLastError();
}  // already a WSA code
void CloseSocket(Socket s) {
  closesocket(s);
}
bool WouldBlock(int e) {
  return e == WSAEWOULDBLOCK;
}
bool InProgress(int e) {
  return e == WSAEWOULDBLOCK || e == WSAEALREADY || e == WSAEINVAL;
}
constexpr int kIsConn = WSAEISCONN;
constexpr int kSendFlags = 0;
#else
using Socket = int;
constexpr Socket kInvalid = -1;
// POSIX errno -> Windows Sockets error code (the high word of results).
int ToWsa(int e) {
  switch (e) {
    case EINTR:
      return 10004;
    case EACCES:
      return 10013;
    case EFAULT:
      return 10014;
    case EINVAL:
      return 10022;
    case EMFILE:
      return 10024;
    case EINPROGRESS:
      return 10036;
    case EALREADY:
      return 10037;
    case ENOTSOCK:
    case EBADF:
      return 10038;
    case EDESTADDRREQ:
      return 10039;
    case EMSGSIZE:
      return 10040;
    case EADDRINUSE:
      return 10048;
    case EADDRNOTAVAIL:
      return 10049;
    case ENETDOWN:
      return 10050;
    case ENETUNREACH:
      return 10051;
    case ECONNABORTED:
      return 10053;
    case ECONNRESET:
    case EPIPE:
      return 10054;
    case ENOBUFS:
      return 10055;
    case EISCONN:
      return 10056;
    case ENOTCONN:
      return 10057;
    case ETIMEDOUT:
      return 10060;
    case ECONNREFUSED:
      return 10061;
    case EHOSTUNREACH:
      return 10065;
    default:
      if (e == EAGAIN || e == EWOULDBLOCK)
        return 10035;
      return 10000 + e;
  }
}
int LastError() {
  return ToWsa(errno);
}
void CloseSocket(Socket s) {
  close(s);
}
bool WouldBlock(int e) {
  return e == 10035;
}
bool InProgress(int e) {
  return e == 10035 || e == 10036 || e == 10037;
}
constexpr int kIsConn = 10056;
#ifdef MSG_NOSIGNAL
constexpr int kSendFlags = MSG_NOSIGNAL;  // a closed peer is an error, not SIGPIPE
#else
constexpr int kSendFlags = 0;
#endif
#endif

constexpr int kWsaNotSock = 10038;
constexpr int kWsaMsgSize = 10040;
constexpr int kConnectTimeoutMs = 5000;  // Net.cpp netCreateClient()

int Code(int wsa_error, int status) {
  return (wsa_error << 16) + status;
}

bool SetNonBlocking(Socket s) {
#ifdef _WIN32
  u_long on = 1;
  return ioctlsocket(s, FIONBIO, &on) == 0;
#else
  const int flags = fcntl(s, F_GETFL, 0);
  return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

// Waits until a non-blocking connect completes; 0 = still pending.
int WaitWritable(Socket s, int timeout_ms) {
#ifdef _WIN32
  fd_set w, e;
  FD_ZERO(&w);
  FD_ZERO(&e);
  FD_SET(s, &w);
  FD_SET(s, &e);
  timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  return select(0, nullptr, &w, &e, &tv);
#else
  pollfd p{s, POLLOUT, 0};
  return poll(&p, 1, timeout_ms);
#endif
}

int SocketError(Socket s) {
  int err = 0;
  socklen_t len = sizeof(err);
  getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
#ifdef _WIN32
  return err;
#else
  return err == 0 ? 0 : ToWsa(err);
#endif
}

// "a.b.c.d" (inet_addr: INADDR_NONE when not a dotted address).
uint32_t ParseIp(const char* text) {
  in_addr a{};
  if (text == nullptr || inet_pton(AF_INET, text, &a) != 1)
    return htonl(INADDR_NONE);
  return a.s_addr;
}

void FormatIp(uint32_t addr, char* out) {  // at most 15 characters + NUL
  in_addr a{};
  a.s_addr = addr;
  char text[INET_ADDRSTRLEN] = {};
  inet_ntop(AF_INET, &a, text, sizeof(text));
  std::snprintf(out, 16, "%s", text);
}

Socket Sk(intptr_t v) {
  return static_cast<Socket>(v);
}
intptr_t Stored(Socket s) {
  return static_cast<intptr_t>(s);
}

}  // namespace

StdNetworkIO::StdNetworkIO() : sock_(Stored(kInvalid)) {
#ifdef _WIN32
  WSADATA wsd;
  winsock_ = WSAStartup(MAKEWORD(2, 2), &wsd) == 0;
#endif
  static_assert(sizeof(Socket) <= sizeof(intptr_t), "socket handle size");
}

StdNetworkIO::~StdNetworkIO() {
  CloseSockets();
#ifdef _WIN32
  if (winsock_)
    WSACleanup();
#endif
}

void StdNetworkIO::Reset() {
  CloseSockets();
}

// Net.cpp netInit(). A new session also forgets the previous one's bound
// state (the original kept it, so a new unbound socket could be read).
int StdNetworkIO::Init(int port, int protocol) {
  CloseSockets();
  mode_ = Mode::kNone;
  bound_ = false;
#ifdef _WIN32
  if (!winsock_)
    return Code(WSASYSNOTREADY, kNetInitFailed);
#endif
  Socket s = kInvalid;
  switch (protocol) {
    case kNetUdp:
      s = socket(AF_INET, SOCK_DGRAM, 0);
      type_ = Type::kUdp;
      break;
    case kNetTcp:
      s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      type_ = Type::kUnconnectedTcp;
      break;
    default:
      return kNetInitFailed;
  }
  if (s == kInvalid) {
    type_ = Type::kUnconnected;
    return Code(LastError(), kNetInvalidSocket);
  }
  if (!SetNonBlocking(s)) {
    const int e = LastError();
    CloseSocket(s);
    type_ = Type::kUnconnected;
    return Code(e, kNetInvalidSocket);
  }
#ifndef _WIN32
  // Rebinding a port right after closing it works as on Windows.
  const int on = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#ifdef SO_NOSIGPIPE
  setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
#endif
  sock_ = Stored(s);
  remote_port_ = htons(static_cast<uint16_t>(port));
  return kNetOk;
}

// Net.cpp netCloseSockets(): an error when no socket is open (closesocket
// of no socket).
int StdNetworkIO::CloseSockets() {
  type_ = Type::kUnconnected;
  mode_ = Mode::kNone;
  bound_ = false;
  if (Sk(sock_) == kInvalid)
    return Code(kWsaNotSock, kNetError);
  CloseSocket(Sk(sock_));
  sock_ = Stored(kInvalid);
  return kNetOk;
}

// Net.cpp netCreateServer().
void StdNetworkIO::CreateServer(int settings, int* result) {
  const int protocol = settings & 0xFF;
  const int port = static_cast<int>((static_cast<uint32_t>(settings) >> 16) & 0xFFFF);
  int status = Init(port, protocol);
  if (status != kNetOk || Sk(sock_) == kInvalid) {
    *result = status;
    return;
  }
  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_port = htons(static_cast<uint16_t>(port));
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(Sk(sock_), reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
    *result = Code(LastError(), kNetBindFailed);
    return;
  }
  bound_ = true;
  mode_ = Mode::kServer;
  *result = kNetOk;
}

// One connect attempt of a non-blocking TCP client; true when connected.
bool StdNetworkIO::TryConnect(int* status) {
  sockaddr_in remote{};
  remote.sin_family = AF_INET;
  remote.sin_port = remote_port_;
  remote.sin_addr.s_addr = remote_addr_;
  *status = kNetOk;
  if (connect(Sk(sock_), reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) == 0) {
    type_ = Type::kConnectedTcp;
    return true;
  }
  const int e = LastError();
  if (e == kIsConn) {
    type_ = Type::kConnectedTcp;
    return true;
  }
  if (!InProgress(e))
    *status = Code(e, kNetError);
  return false;
}

// Net.cpp netCreateClient(): a TCP client waits up to 5 s for the
// connection (one wait for completion instead of 500 ms sleeps).
void StdNetworkIO::CreateClient(int settings, char* server, int* result) {
  const int protocol = settings & 0xFF;
  const int port = static_cast<int>((static_cast<uint32_t>(settings) >> 16) & 0xFFFF);
  int status = Init(port, protocol);
  if (status != kNetOk) {
    *result = status;
    return;
  }
  remote_addr_ = ParseIp(server);
  if (remote_addr_ == htonl(INADDR_NONE)) {  // a host name
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* found = nullptr;
    if (getaddrinfo(server, nullptr, &hints, &found) != 0 || found == nullptr) {
      *result = kNetDomainNotFound;
      return;
    }
    remote_addr_ = reinterpret_cast<sockaddr_in*>(found->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(found);
    FormatIp(remote_addr_, server);  // the IP connected to
  }
  mode_ = Mode::kClient;
  if (type_ == Type::kUnconnectedTcp && !TryConnect(&status)) {
    if (status != kNetOk) {
      *result = status;
      return;
    }
    if (WaitWritable(Sk(sock_), kConnectTimeoutMs) > 0) {
      const int e = SocketError(Sk(sock_));
      if (e != 0) {
        *result = Code(e, kNetError);
        return;
      }
      type_ = Type::kConnectedTcp;
    }
  }
  *result = kNetOk;  // also when the connection is still pending
}

// Net.cpp netSendData(). TCP data goes with send() (sendto with an address
// fails on connected sockets on some systems); with SendOnPort the port is
// a port number (the original used it unconverted, i.e. byte-swapped).
int StdNetworkIO::SendData(const char* data, unsigned int* size, const char* remote_ip,
                           bool set_port, uint16_t port) {
  const unsigned int send_size = *size;
  *size = 0;
  if (Sk(sock_) == kInvalid)
    return Code(kWsaNotSock, kNetError);
  if (mode_ == Mode::kServer) {
    remote_addr_ = ParseIp(remote_ip);
    if (set_port)
      remote_port_ = htons(port);
  }
  if (mode_ == Mode::kClient && type_ == Type::kUnconnectedTcp) {
    int status = kNetOk;
    if (!TryConnect(&status))
      return status;  // kNetOk: no connection yet
  }
  long ret = 0;
  if (type_ == Type::kUdp) {
    sockaddr_in remote{};
    remote.sin_family = AF_INET;
    remote.sin_port = remote_port_;
    remote.sin_addr.s_addr = remote_addr_;
    ret = sendto(Sk(sock_), data, static_cast<int>(send_size), kSendFlags,
                 reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
  } else {
    ret = send(Sk(sock_), data, static_cast<int>(send_size), kSendFlags);
  }
  if (ret < 0)
    return Code(LastError(), kNetError);
  bound_ = true;  // sendto binds an unbound socket
  *size = static_cast<unsigned int>(ret);
  return kNetOk;
}

// Net.cpp netReadData(): a TCP server accepts its client on the first
// read. A zero-byte request returns at once (the original's recv of 0
// bytes looked like a disconnect); a UDP datagram larger than the request
// is the WSAEMSGSIZE error, as on Windows.
int StdNetworkIO::ReadData(char* data, unsigned int* size, char* sender_ip, uint16_t* port) {
  const unsigned int read_size = *size;
  *size = 0;
  if (!bound_ || Sk(sock_) == kInvalid)
    return kNetOk;
  if (mode_ == Mode::kServer && type_ == Type::kUnconnectedTcp) {
    if (listen(Sk(sock_), 1) != 0)
      return Code(LastError(), kNetError);
    Socket client = accept(Sk(sock_), nullptr, nullptr);
    if (client == kInvalid) {
      const int e = LastError();
      return WouldBlock(e) ? kNetOk : Code(e, kNetError);
    }
    SetNonBlocking(client);  // POSIX accept() does not inherit O_NONBLOCK
    CloseSocket(Sk(sock_));
    sock_ = Stored(client);
    type_ = Type::kConnectedTcp;
  }
  if (mode_ == Mode::kClient && type_ == Type::kUnconnectedTcp)
    return kNetOk;
  if (read_size == 0)
    return kNetOk;
  sockaddr_in from{};
  long ret = 0;
  if (type_ == Type::kUdp) {
#ifdef _WIN32
    int len = sizeof(from);
    ret = recvfrom(Sk(sock_), data, static_cast<int>(read_size), 0,
                   reinterpret_cast<sockaddr*>(&from), &len);
#else
    iovec iov{data, read_size};
    msghdr msg{};
    msg.msg_name = &from;
    msg.msg_namelen = sizeof(from);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    ret = recvmsg(Sk(sock_), &msg, 0);
    if (ret >= 0 && (msg.msg_flags & MSG_TRUNC))
      return Code(kWsaMsgSize, kNetError);
#endif
  } else {
    ret = recv(Sk(sock_), data, static_cast<int>(read_size), 0);
    socklen_t len = sizeof(from);
    getpeername(Sk(sock_), reinterpret_cast<sockaddr*>(&from), &len);
  }
  if (ret < 0) {
    const int e = LastError();
    if (!WouldBlock(e))
      return Code(e, kNetError);
    ret = 0;
  } else if (ret == 0 && type_ == Type::kConnectedTcp) {
    return Code(kNetRemoteDisconnect, kNetError);
  }
  if (ret > 0) {
    FormatIp(from.sin_addr.s_addr, sender_ip);
    if (port != nullptr)
      *port = ntohs(from.sin_port);
  }
  *size = static_cast<unsigned int>(ret);
  return kNetOk;
}

void StdNetworkIO::Send(int settings, const char* data, const char* remote_ip, int* count,
                        int* result) {
  unsigned int size = static_cast<unsigned int>(settings) & 0xFFFF;
  *result = SendData(data, &size, remote_ip, false, 0);
  if (*result == kNetOk)
    *count = static_cast<int>(size);
}

void StdNetworkIO::Receive(int settings, char* buf, int* count, char* sender_ip, int* result) {
  unsigned int size = static_cast<unsigned int>(settings) & 0xFFFF;
  *result = ReadData(buf, &size, sender_ip, nullptr);
  if (*result == kNetOk)
    *count = static_cast<int>(size);
}

void StdNetworkIO::SendOnPort(long* d0, long* d1, const char* data, const char* remote_ip) {
  unsigned int size = static_cast<unsigned int>(*d1) & 0xFFFF;
  const auto port = static_cast<uint16_t>((static_cast<unsigned long>(*d1) >> 16) & 0xFFFF);
  *d0 = SendData(data, &size, remote_ip, true, port);
  if (*d0 == kNetOk)
    *d1 = static_cast<long>(size);
}

// The sender's port is 0 when nothing was received (the original returned
// an uninitialised value).
void StdNetworkIO::ReceiveOnPort(long* d0, long* d1, char* buf, char* sender_ip) {
  unsigned int size = static_cast<unsigned int>(*d1) & 0xFFFF;
  uint16_t port = 0;
  *d0 = ReadData(buf, &size, sender_ip, &port);
  if (*d0 == kNetOk)
    *d1 = static_cast<long>((static_cast<uint32_t>(port) << 16) | (size & 0xFFFF));
}

void StdNetworkIO::CloseConnection(int /*close_ip*/, int* result) {
  *result = CloseSockets();
}

// Net.cpp netLocalIP(): the host name's first IPv4 address; when that is a
// loopback address (common on Linux) the address of the default route is
// used, which is what Windows reports.
void StdNetworkIO::GetLocalIP(char* local_ip, int* result) {
  char host[256] = {};
  if (gethostname(host, sizeof(host) - 1) != 0) {
    *result = Code(LastError(), kNetError);
    return;
  }
  addrinfo hints{};
  hints.ai_family = AF_INET;
  addrinfo* found = nullptr;
  if (getaddrinfo(host, nullptr, &hints, &found) != 0 || found == nullptr) {
    *result = Code(LastError(), kNetError);
    return;
  }
  uint32_t addr = reinterpret_cast<sockaddr_in*>(found->ai_addr)->sin_addr.s_addr;
  freeaddrinfo(found);
  if ((ntohl(addr) >> 24) == 127) {
    Socket probe = socket(AF_INET, SOCK_DGRAM, 0);
    if (probe != kInvalid) {
      sockaddr_in target{};
      target.sin_family = AF_INET;
      target.sin_port = htons(53);
      target.sin_addr.s_addr = htonl(0x08080808);  // no packet is sent
      sockaddr_in self{};
      socklen_t len = sizeof(self);
      if (connect(probe, reinterpret_cast<sockaddr*>(&target), sizeof(target)) == 0 &&
          getsockname(probe, reinterpret_cast<sockaddr*>(&self), &len) == 0 &&
          self.sin_addr.s_addr != 0)
        addr = self.sin_addr.s_addr;
      CloseSocket(probe);
    }
  }
  FormatIp(addr, local_ip);
  *result = kNetOk;
}

}  // namespace easym68k::sim
