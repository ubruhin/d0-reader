#pragma once

#include "lwip/tcp.h"

class TcpSocketCallbacks;

class TcpSocket {
public:
  TcpSocket() = delete;
  TcpSocket(const TcpSocket& other) = delete;
  explicit TcpSocket(u16_t port);

  bool canWrite() const;
  void write(const uint8_t* data, u16_t len);

  err_t cb_accept(struct tcp_pcb *newpcb, err_t err);
  err_t cb_recv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
  void cb_error(err_t err);
  err_t cb_poll(struct tcp_pcb* tpcb);
  err_t cb_sent(struct tcp_pcb* tpcb, u16_t len);

  void send(struct tcp_pcb* tpcb);
  void close();

private:
  enum class State {
    None = 0,
    Accepted,
    Received,
    Closing,
  };

private:
  struct tcp_pcb* mListenPcb;
  struct tcp_pcb* mSocketPcb;
  State mState;
  u8_t mRetries;
  struct pbuf* mRxBuf;
};
