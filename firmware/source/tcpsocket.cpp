#include "tcpsocket.h"
#include <cassert>

static err_t _accept(void* arg, struct tcp_pcb* newpcb, err_t err) {
  assert(arg != nullptr);
  return static_cast<TcpSocket*>(arg)->cb_accept(newpcb, err);
}

static err_t _recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
  assert(arg != nullptr);
  return static_cast<TcpSocket*>(arg)->cb_recv(tpcb, p, err);
}

static void _error(void* arg, err_t err) {
  if (arg) {
    static_cast<TcpSocket*>(arg)->cb_error(err);
  }
}

static err_t _poll(void* arg, struct tcp_pcb* tpcb) {
  if (arg) {
    return static_cast<TcpSocket*>(arg)->cb_poll(tpcb);
  } else {
    // nothing to be done.
    tcp_abort(tpcb);
    return ERR_ABRT;
  }
}

static err_t _sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
  assert(arg != nullptr);
  return static_cast<TcpSocket*>(arg)->cb_sent(tpcb, len);
}

TcpSocket::TcpSocket(u16_t port)
  : mPcb(tcp_new()), mState(State::None), mRetries(0U), mBuf(nullptr) {
    assert(mPcb != nullptr);

    if (tcp_bind(mPcb, IP_ADDR_ANY, port) == ERR_OK) {
      mPcb = tcp_listen(mPcb);
      tcp_arg(mPcb, this);
      tcp_accept(mPcb, _accept);
    } else {
      memp_free(MEMP_TCP_PCB, mPcb);
    }
}

bool TcpSocket::canSend() const {
  return (mState == State::Accepted) || (mState == State::Received);
}

err_t TcpSocket::cb_accept(struct tcp_pcb* newpcb, err_t err) {
  LWIP_UNUSED_ARG(err);

  mState = State::Accepted;
  mPcb = newpcb;
  mRetries = 0U;
  mBuf = nullptr;

  tcp_setprio(newpcb, TCP_PRIO_MIN);
  tcp_recv(newpcb, _recv);
  tcp_err(newpcb, _error);
  tcp_poll(newpcb, _poll, 0);
  return ERR_OK;
}

err_t TcpSocket::cb_recv(struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
  err_t ret_err;

  // If we receive an empty tcp frame from client => close connection.
  if (p == nullptr) {
    mState = State::Closing;
    if (mBuf == nullptr) {
       // We're done sending, close connection.
       close(tpcb);
    } else {
      // We're not done yet, acknowledge received packet.
      tcp_sent(tpcb, _sent);

      // Send remaining data.
      send(tpcb);
    }
    ret_err = ERR_OK;
  } else if (err != ERR_OK) {
    // A non empty frame was received from client but for some reason err != ERR_OK.
    if (p != nullptr) {
      mBuf = nullptr;
      pbuf_free(p);
    }
    ret_err = err;
  } else if (mState == State::Accepted) {
    // First data chunk in p->payload.
    mState = State::Received;

    // Store reference to incoming pbuf (chain).
    mBuf = p;

    // Initialize LwIP tcp_sent callback function.
    tcp_sent(tpcb, _sent);

    // Send back the received data (echo).
    send(tpcb);

    ret_err = ERR_OK;
  } else if (mState == State::Received) {
    // More data received from client and previous data has been already sent.
    if (mBuf == nullptr) {
      mBuf = p;

      // Send back received data.
      send(tpcb);
    } else {
      // Chain pbufs to the end of what we recv'ed previously.
      struct pbuf* ptr = mBuf;
      pbuf_chain(ptr, p);
    }
    ret_err = ERR_OK;
  } else if(mState == State::Closing) {
    // Odd case, remote side closing twice, trash data.
    tcp_recved(tpcb, p->tot_len);
    mBuf = nullptr;
    pbuf_free(p);
    ret_err = ERR_OK;
  } else {
    // Unknown mState, trash data.
    tcp_recved(tpcb, p->tot_len);
    mBuf = nullptr;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

void TcpSocket::cb_error(err_t err) {
  LWIP_UNUSED_ARG(err);
}

err_t TcpSocket::cb_poll(struct tcp_pcb* tpcb) {
  if (mBuf) {
    tcp_sent(tpcb, _sent);
    /* there is a remaining pbuf (chain) , try to send data */
    send(tpcb);
  } else if (mState == State::Closing) {
    /* no remaining pbuf (chain)  */
    close(tpcb);
  }
  return ERR_OK;
}

err_t TcpSocket::cb_sent(struct tcp_pcb* tpcb, u16_t len) {
  LWIP_UNUSED_ARG(len);

  mRetries = 0;

  if (mBuf) {
    // Still got pbufs to send.
    tcp_sent(tpcb, _sent);
    send(tpcb);
  } else if (mState == State::Closing) {
    // If no more data to send and client closed connection.
    close(tpcb);
  }
  return ERR_OK;
}

void TcpSocket::send(struct tcp_pcb* tpcb) {
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;

  while ((wr_err == ERR_OK) &&
         (mBuf != NULL) &&
         (mBuf->len <= tcp_sndbuf(tpcb)))
  {

    /* get pointer on pbuf from es structure */
    ptr = mBuf;

    /* enqueue data for transmission */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

    if (wr_err == ERR_OK)
    {
      u16_t plen;
      u8_t freed;

      plen = ptr->len;

      /* continue with next pbuf in chain (if any) */
      mBuf = ptr->next;

      if(mBuf != NULL)
      {
        /* increment reference count for mBuf */
        pbuf_ref(mBuf);
      }

     /* chop first pbuf from chain */
      do
      {
        /* try hard to free pbuf */
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
     /* we can read more data now */
     tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
      /* we are low on memory, try later / harder, defer to poll */
     mBuf = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

void TcpSocket::close(struct tcp_pcb* tpcb) {
  // Remove all callbacks.
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  // close tcp connection.
  tcp_close(tpcb);
}
