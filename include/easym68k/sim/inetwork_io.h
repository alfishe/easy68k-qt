// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// INetworkIO — UDP/TCP network I/O interface (Trap #15 tasks 100-107).
// Replaces Winsock API in original CODE9.CPP.

#pragma once

namespace easym68k::sim {

class INetworkIO {
 public:
  virtual ~INetworkIO() = default;
  virtual void CreateClient(int settings, const char* server, int* result) = 0;
  virtual void CreateServer(int settings, int* result) = 0;
  virtual void Send(int settings, const char* data, const char* remote_ip, int* count,
                    int* result) = 0;
  virtual void Receive(int settings, char* buf, int* count, char* sender_ip, int* result) = 0;
  virtual void CloseConnection(int close_ip, int* result) = 0;
  virtual void GetLocalIP(char* local_ip, int* result) = 0;
  virtual void SendOnPort(long* d0, long* d1, const char* data, const char* remote_ip) = 0;
  virtual void ReceiveOnPort(long* d0, long* d1, char* buf, char* sender_ip) = 0;
};

}  // namespace easym68k::sim
