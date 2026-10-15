// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// StdNetworkIO (Trap #15 tasks 100-107) on the loopback interface: UDP and
// TCP client/server exchanges, ports, error codes.

#include "easym68k/sim/std_network_io.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace easym68k::sim {
namespace {

// A port that is free now (bound to 0 by the system, then released).
int FreePort(int type) {
#ifdef _WIN32
  WSADATA wsd;
  WSAStartup(MAKEWORD(2, 2), &wsd);
#endif
  auto s = socket(AF_INET, type, 0);
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  bind(s, reinterpret_cast<sockaddr*>(&a), sizeof(a));
  socklen_t len = sizeof(a);
  getsockname(s, reinterpret_cast<sockaddr*>(&a), &len);
#ifdef _WIN32
  closesocket(s);
#else
  close(s);
#endif
  return ntohs(a.sin_port);
}

int Settings(int protocol, int port) {
  return (port << 16) | protocol;
}

// Polls a receive until data arrives (non-blocking sockets).
template <typename Fn>
bool Eventually(Fn fn, int timeout_ms = 3000) {
  const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < end) {
    if (fn())
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

TEST(StdNetworkIOTest, UdpClientAndServer) {
  const int port = FreePort(SOCK_DGRAM);
  StdNetworkIO server, client;
  int result = -1;
  server.CreateServer(Settings(kNetUdp, port), &result);
  ASSERT_EQ(result, kNetOk);
  char host[256] = "127.0.0.1";
  client.CreateClient(Settings(kNetUdp, port), host, &result);
  ASSERT_EQ(result, kNetOk);

  int count = -1;
  char buf[64] = {};
  char sender[64] = {};
  server.Receive(64, buf, &count, sender, &result);  // nothing yet
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 0);

  client.Send(5, "hello", "", &count, &result);
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 5);
  ASSERT_TRUE(Eventually([&] {
    server.Receive(64, buf, &count, sender, &result);
    return result != kNetOk || count > 0;
  }));
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 5);
  EXPECT_EQ(std::string(buf, 5), "hello");
  EXPECT_STREQ(sender, "127.0.0.1");

  // The server answers on the sender's port (task 107 returns it, 106 uses
  // it as a port number).
  long d0 = 0;
  long d1 = 64;
  client.Send(2, "hi", "", &count, &result);
  ASSERT_TRUE(Eventually([&] {
    d1 = 64;
    server.ReceiveOnPort(&d0, &d1, buf, sender);
    return d0 != kNetOk || (d1 & 0xFFFF) > 0;
  }));
  EXPECT_EQ(d0, kNetOk);
  EXPECT_EQ(d1 & 0xFFFF, 2);
  const long sender_port = (d1 >> 16) & 0xFFFF;
  EXPECT_NE(sender_port, 0);
  d1 = (sender_port << 16) | 3;
  server.SendOnPort(&d0, &d1, "abc", "127.0.0.1");
  EXPECT_EQ(d0, kNetOk);
  EXPECT_EQ(d1, 3);
  ASSERT_TRUE(Eventually([&] {
    client.Receive(64, buf, &count, sender, &result);
    return result != kNetOk || count > 0;
  }));
  EXPECT_EQ(count, 3);
  EXPECT_EQ(std::string(buf, 3), "abc");

  // A datagram larger than the request: WSAEMSGSIZE, as on Windows.
  client.Send(10, "0123456789", "", &count, &result);
  ASSERT_TRUE(Eventually([&] {
    server.Receive(4, buf, &count, sender, &result);
    return result != kNetOk || count > 0;
  }));
  EXPECT_EQ(result, (10040 << 16) | kNetError);
}

TEST(StdNetworkIOTest, TcpClientAndServer) {
  const int port = FreePort(SOCK_STREAM);
  StdNetworkIO server, client;
  int result = -1;
  server.CreateServer(Settings(kNetTcp, port), &result);
  ASSERT_EQ(result, kNetOk);
  int count = -1;
  char buf[64] = {};
  char sender[64] = {};
  server.Receive(64, buf, &count, sender, &result);  // listen; no client yet
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 0);

  char host[256] = "localhost";  // a host name is resolved and returned
  client.CreateClient(Settings(kNetTcp, port), host, &result);
  ASSERT_EQ(result, kNetOk);
  EXPECT_STREQ(host, "127.0.0.1");

  ASSERT_TRUE(Eventually([&] {  // the server accepts on a read
    client.Send(4, "ping", "", &count, &result);
    if (result != kNetOk || count == 0)
      return false;
    server.Receive(64, buf, &count, sender, &result);
    return result != kNetOk || count > 0;
  }));
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(std::string(buf, 4), "ping");
  EXPECT_STREQ(sender, "127.0.0.1");

  server.Send(4, "pong", "127.0.0.1", &count, &result);
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 4);
  ASSERT_TRUE(Eventually([&] {
    client.Receive(64, buf, &count, sender, &result);
    return result != kNetOk || count > 0;
  }));
  EXPECT_EQ(std::string(buf, 4), "pong");

  // A zero-byte request is not a disconnect.
  count = -1;
  client.Receive(0, buf, &count, sender, &result);
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 0);

  // The client closes: the server's next read reports the disconnect.
  client.CloseConnection(0, &result);
  EXPECT_EQ(result, kNetOk);
  ASSERT_TRUE(Eventually([&] {
    server.Receive(64, buf, &count, sender, &result);
    return result != kNetOk;
  }));
  EXPECT_EQ(result, (kNetRemoteDisconnect << 16) | kNetError);
}

TEST(StdNetworkIOTest, ErrorsAndState) {
  StdNetworkIO net;
  int result = -1;
  net.CreateServer(Settings(7, 40000), &result);  // invalid protocol
  EXPECT_EQ(result, kNetInitFailed);
  net.CloseConnection(0, &result);  // nothing open
  EXPECT_EQ(result, (10038 << 16) | kNetError);
  int count = 99;
  net.Send(3, "abc", "127.0.0.1", &count, &result);
  EXPECT_EQ(result & 0xFFFF, kNetError);
  EXPECT_EQ(count, 99);  // unchanged on error
  char buf[8];
  char sender[64] = {};
  net.Receive(8, buf, &count, sender, &result);  // unbound: nothing
  EXPECT_EQ(result, kNetOk);
  EXPECT_EQ(count, 0);
  char host[256] = "no-such-host.invalid";
  net.CreateClient(Settings(kNetUdp, 40000), host, &result);
  EXPECT_EQ(result, kNetDomainNotFound);

  const int port = FreePort(SOCK_DGRAM);
  StdNetworkIO first, second;
  first.CreateServer(Settings(kNetUdp, port), &result);
  ASSERT_EQ(result, kNetOk);
  first.Reset();  // program start closes the socket
  second.CreateServer(Settings(kNetUdp, port), &result);
  EXPECT_EQ(result, kNetOk);
}

TEST(StdNetworkIOTest, TcpConnectionRefused) {
  const int port = FreePort(SOCK_STREAM);  // nobody listens
  StdNetworkIO client;
  char host[256] = "127.0.0.1";
  int result = -1;
  client.CreateClient(Settings(kNetTcp, port), host, &result);
  EXPECT_EQ(result, (10061 << 16) | kNetError);  // WSAECONNREFUSED
}

TEST(StdNetworkIOTest, LocalIp) {
  StdNetworkIO net;
  char ip[64] = {};
  int result = -1;
  net.GetLocalIP(ip, &result);
  EXPECT_EQ(result, kNetOk);
  EXPECT_GT(std::strlen(ip), 6u);
  EXPECT_LT(std::strlen(ip), 16u);
}

}  // namespace
}  // namespace easym68k::sim
