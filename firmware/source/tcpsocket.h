#pragma once

#include "lwip/tcp.h"

class TcpSocketCallbacks;

class TcpSocket {
public:
  TcpSocket() = delete;
  TcpSocket(const TcpSocket& other) = delete;
  explicit TcpSocket(u16_t port);

  bool canSend() const;

  err_t cb_accept(struct tcp_pcb *newpcb, err_t err);
  err_t cb_recv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
  void cb_error(err_t err);
  err_t cb_poll(struct tcp_pcb* tpcb);
  err_t cb_sent(struct tcp_pcb* tpcb, u16_t len);

  void send(struct tcp_pcb* tpcb);
  void close(struct tcp_pcb* tpcb);

private:
  enum class State {
    None = 0,
    Accepted,
    Received,
    Closing,
  };


private:
  struct tcp_pcb* mPcb;
  State mState;
  u8_t mRetries;
  struct pbuf* mBuf;
};
