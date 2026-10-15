// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// StdNetworkIO: the Sim68K network back end (Trap #15 tasks 100-107) on
// BSD sockets / Winsock, ported from the original Net.cpp. One UDP or TCP
// connection at a time, as client or server, with non-blocking sockets:
// sends and receives never wait (0 bytes when nothing is available).
//
// Results are the original's two-part codes: the low 16 bits are a status
// (kNetOk, kNetError, ...), the high 16 bits a Windows Sockets error code
// (POSIX errors are mapped to the equivalent WSA codes).
// Headless (no Qt); used by Sim68K-Qt and the command-line tools.

#ifndef EASY68K_SIM_STD_NETWORK_IO_H_
#define EASY68K_SIM_STD_NETWORK_IO_H_

#include <cstdint>

#include "easym68k/sim/inetwork_io.h"

namespace easym68k::sim {

// Net.h status codes.
constexpr int kNetOk = 0;
constexpr int kNetError = 1;
constexpr int kNetInitFailed = 2;
constexpr int kNetInvalidSocket = 3;
constexpr int kNetBindFailed = 5;
constexpr int kNetDomainNotFound = 8;
constexpr int kNetRemoteDisconnect = 0x2775;  // WSAECONNRESET, high word
constexpr int kNetUdp = 0;
constexpr int kNetTcp = 1;

class StdNetworkIO : public INetworkIO {
 public:
  StdNetworkIO();
  ~StdNetworkIO() override;

  StdNetworkIO(const StdNetworkIO&) = delete;
  StdNetworkIO& operator=(const StdNetworkIO&) = delete;

  // settings: bits 0-7 protocol (0 UDP, 1 TCP), bits 16-31 port. A host name
  // in `server` is replaced by its dotted IP address. A TCP client waits up
  // to 5 s for the connection; if it is not complete then, the first sends
  // finish it.
  void CreateClient(int settings, char* server, int* result) override;
  void CreateServer(int settings, int* result) override;
  // settings bits 0-15: byte count. *count is set only on success.
  void Send(int settings, const char* data, const char* remote_ip, int* count,
            int* result) override;
  void Receive(int settings, char* buf, int* count, char* sender_ip, int* result) override;
  void CloseConnection(int close_ip, int* result) override;
  void GetLocalIP(char* local_ip, int* result) override;
  // D1: bits 0-15 byte count, bits 16-31 port (server: destination port;
  // receive: the sender's port is returned). D0 = result.
  void SendOnPort(long* d0, long* d1, const char* data, const char* remote_ip) override;
  void ReceiveOnPort(long* d0, long* d1, char* buf, char* sender_ip) override;

  // Program start (STARTSIM.CPP netCloseSockets()).
  void Reset();

 private:
  enum class Mode { kNone, kServer, kClient };
  enum class Type { kUnconnected, kUdp, kUnconnectedTcp, kConnectedTcp };

  int Init(int port, int protocol);
  int CloseSockets();
  bool TryConnect(int* status);  // TCP client; false + *status on error
  int SendData(const char* data, unsigned int* size, const char* remote_ip, bool set_port,
               uint16_t port);
  int ReadData(char* data, unsigned int* size, char* sender_ip, uint16_t* port);

  intptr_t sock_;  // SOCKET / int file descriptor; kInvalid when closed
  Mode mode_ = Mode::kNone;
  Type type_ = Type::kUnconnected;
  bool bound_ = false;
  uint32_t remote_addr_ = 0;  // network byte order
  uint16_t remote_port_ = 0;  // network byte order
  [[maybe_unused]] bool winsock_ = false;  // Winsock started (Windows)
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_STD_NETWORK_IO_H_
