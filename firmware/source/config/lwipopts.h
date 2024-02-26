#pragma once

// Memory Management
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (10*1024)
#define MEMP_NUM_PBUF               10
#define MEMP_NUM_UDP_PCB            6
#define MEMP_NUM_TCP_PCB            10
#define MEMP_NUM_TCP_PCB_LISTEN     5
#define MEMP_NUM_TCP_SEG            8
#define MEMP_NUM_SYS_TIMEOUT        10
#define MEMP_NUM_IGMP_GROUP         5
#define PBUF_POOL_SIZE              8
#define PBUF_POOL_BUFSIZE           512


// TCP
#define LWIP_TCP                    1
#define LWIP_TCP_KEEPALIVE          1
#define TCP_TTL                     255
#define TCP_QUEUE_OOSEQ             0
#define TCP_MSS                     (1500 - 40)
#define TCP_SND_BUF                 (4*TCP_MSS)
#define TCP_SND_QUEUELEN            (2*TCP_SND_BUF/TCP_MSS)
#define TCP_WND                     (2*TCP_MSS)

// UDP
#define LWIP_UDP                    1
#define UDP_TTL                     255

// ICMP
#define LWIP_ICMP                   1

// IGMP
#define LWIP_IGMP                   1

// DHCP
#define LWIP_DHCP                   1

// MDNS
#define LWIP_MDNS_RESPONDER         1
#define MDNS_MAX_SERVICES           3

// AUTOIP
#define LWIP_AUTOIP                 1

// TCP/IP
#define TCPIP_MBOX_SIZE             6
#define TCPIP_THREAD_STACKSIZE      512
#define TCPIP_THREAD_PRIO           1

// Various
#define LWIP_STATS                  0
#define LWIP_SINGLE_NETIF           1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NUM_NETIF_CLIENT_DATA  3
#define LWIP_MULTICAST_TX_OPTIONS   1
#define LWIP_NETIF_LINK_CALLBACK    1
#define DEFAULT_TCP_RECVMBOX_SIZE   6
#define DEFAULT_ACCEPTMBOX_SIZE     6

// APIs
#define LWIP_NETCONN                1
#define LWIP_SOCKET                 1

// Disable checksum calculations since it is done by the peripheral.
#define CHECKSUM_GEN_IP             0
#define CHECKSUM_GEN_UDP            0
#define CHECKSUM_GEN_TCP            0
#define CHECKSUM_CHECK_IP           0
#define CHECKSUM_CHECK_UDP          0
#define CHECKSUM_CHECK_TCP          0
#define CHECKSUM_GEN_ICMP           0
