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
  : mListenPcb(tcp_new()), mSocketPcb(nullptr), mState(State::None), mRetries(0U), mRxBuf(nullptr) {
    assert(mListenPcb != nullptr);

    if (tcp_bind(mListenPcb, IP_ADDR_ANY, port) == ERR_OK) {
      mListenPcb = tcp_listen(mListenPcb);
      tcp_arg(mListenPcb, this);
      tcp_accept(mListenPcb, _accept);
    } else {
      memp_free(MEMP_TCP_PCB, mListenPcb);
    }
}

bool TcpSocket::canWrite() const {
  return (mState == State::Accepted) || (mState == State::Received);
}

void TcpSocket::write(const uint8_t* data, u16_t len) {
  if (mSocketPcb && (tcp_write(mSocketPcb, data, len, TCP_WRITE_FLAG_COPY) != ERR_OK)) {
    close();
  }
}

err_t TcpSocket::cb_accept(struct tcp_pcb* newpcb, err_t err) {
  LWIP_UNUSED_ARG(err);

  close();

  mSocketPcb = newpcb;
  mState = State::Accepted;
  mRetries = 0U;
  mRxBuf = nullptr;

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
    if (mRxBuf == nullptr) {
       // We're done sending, close connection.
       close();
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
      mRxBuf = nullptr;
      pbuf_free(p);
    }
    ret_err = err;
  } else if (mState == State::Accepted) {
    // First data chunk in p->payload.
    mState = State::Received;

    // Store reference to incoming pbuf (chain).
    //mRxBuf = p;

    // Initialize LwIP tcp_sent callback function.
    //tcp_sent(tpcb, _sent);

    // Send back the received data (echo).
    //send(tpcb);

    tcp_recved(tpcb, p->len);
    ret_err = ERR_OK;
  } else if (mState == State::Received) {
    // More data received from client and previous data has been already sent.
//    if (mRxBuf == nullptr) {
//      mRxBuf = p;
//
//      // Send back received data.
//      send(tpcb);
//    } else {
//      // Chain pbufs to the end of what we recv'ed previously.
//      struct pbuf* ptr = mRxBuf;
//      pbuf_chain(ptr, p);
//    }
    tcp_recved(tpcb, p->len);
    ret_err = ERR_OK;
  } else if(mState == State::Closing) {
    // Odd case, remote side closing twice, trash data.
    tcp_recved(tpcb, p->tot_len);
    mRxBuf = nullptr;
    pbuf_free(p);
    ret_err = ERR_OK;
  } else {
    // Unknown mState, trash data.
    tcp_recved(tpcb, p->tot_len);
    mRxBuf = nullptr;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

void TcpSocket::cb_error(err_t err) {
  LWIP_UNUSED_ARG(err);
  mState = State::Closing;
  mSocketPcb = nullptr;
  mRxBuf = nullptr;
}

err_t TcpSocket::cb_poll(struct tcp_pcb* tpcb) {
  //if (mBuf) {
  //  tcp_sent(tpcb, _sent);
  //  /* there is a remaining pbuf (chain) , try to send data */
  //  send(tpcb);
  //} else if (mState == State::Closing) {
  //  /* no remaining pbuf (chain)  */
  //  close();
  //}
  return ERR_OK;
}

err_t TcpSocket::cb_sent(struct tcp_pcb* tpcb, u16_t len) {
  LWIP_UNUSED_ARG(len);

  mRetries = 0;

  /*if (mBuf) {
    // Still got pbufs to send.
    tcp_sent(tpcb, _sent);
    send(tpcb);
  } else*/ if (mState == State::Closing) {
    // If no more data to send and client closed connection.
    close();
  }
  return ERR_OK;
}

void TcpSocket::send(struct tcp_pcb* tpcb) {
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;

  while ((wr_err == ERR_OK) &&
         (mRxBuf != NULL) &&
         (mRxBuf->len <= tcp_sndbuf(tpcb)))
  {

    /* get pointer on pbuf from es structure */
    ptr = mRxBuf;

    /* enqueue data for transmission */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

    if (wr_err == ERR_OK)
    {
      u16_t plen;
      u8_t freed;

      plen = ptr->len;

      /* continue with next pbuf in chain (if any) */
      mRxBuf = ptr->next;

      if(mRxBuf != NULL)
      {
        /* increment reference count for mRxBuf */
        pbuf_ref(mRxBuf);
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
     mRxBuf = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

void TcpSocket::close() {
  if (mSocketPcb) {
    tcp_recv(mSocketPcb, nullptr);
    tcp_err(mSocketPcb, nullptr);
    tcp_poll(mSocketPcb, nullptr, 0);
    tcp_close(mSocketPcb);
    mSocketPcb = nullptr;
  }

  mState = State::Closing;
}
