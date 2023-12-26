#![no_std]
#![no_main]

use defmt_rtt as _;
use panic_halt as _;

use smoltcp::wire::{IpAddress, Ipv4Address};

const IP_ADDRESS: Ipv4Address = Ipv4Address::new(192, 168, 1, 99);
const SOCKET_ADDRESS: (IpAddress, u16) = (IpAddress::Ipv4(IP_ADDRESS), 1337);
const MAC: [u8; 6] = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05];

#[rtic::app(device = stm32_eth::stm32, dispatchers = [SPI1])]
mod app {

    use systick_monotonic::Systick;

    use stm32_eth::{
        dma::{EthernetDMA, RxRingEntry, TxRingEntry},
        hal::{gpio::GpioExt, rcc::RccExt},
        mac::Speed,
        EthPins,
        Parts,
        PartsIn,
    };

    use smoltcp::{
        iface::{self, Interface, SocketHandle, SocketSet, SocketStorage},
        socket::tcp::{Socket as TcpSocket, SocketBuffer as TcpSocketBuffer, State as TcpState},
        wire::{EthernetAddress, IpCidr, Ipv4Cidr},
    };

    use ieee802_3_miim::{phy::PhySpeed, phy::lan87xxa::LAN8720A, Phy};

    use fugit::RateExtU32;

    struct Gpio {
        pub gpioa: stm32_eth::hal::gpio::gpioa::Parts,
        pub gpiob: stm32_eth::hal::gpio::gpiob::Parts,
        pub gpioc: stm32_eth::hal::gpio::gpioc::Parts,
    }

    #[local]
    struct Local {
        interface: Interface,
        tcp_handle: SocketHandle,
        dma: EthernetDMA<'static, 'static>,
        sockets: SocketSet<'static>,
    }

    #[shared]
    struct Shared {}

    #[monotonic(binds = SysTick, default = true)]
    type Monotonic = Systick<1000>;

    fn now_fn() -> smoltcp::time::Instant {
        let time = monotonics::now().duration_since_epoch().ticks();
        smoltcp::time::Instant::from_millis(time as i64)
    }

    #[init(local = [
        rx_ring: [RxRingEntry; 2] = [RxRingEntry::new(), RxRingEntry::new()],
        tx_ring: [TxRingEntry; 2] = [TxRingEntry::new(), TxRingEntry::new()],
        rx_storage: [u8; 512] = [0u8; 512],
        tx_storage: [u8; 512] = [0u8; 512],
        socket_storage: [SocketStorage<'static>; 1] = [SocketStorage::EMPTY; 1],
    ])]
    fn init(cx: init::Context) -> (Shared, Local, init::Monotonics) {
        use stm32_eth::hal::flash::FlashExt;

        defmt::info!("Init core peripherals");
        let core = cx.core;
        let p = cx.device;

        let rcc = p.RCC.constrain();
        let mut flash = p.FLASH.constrain();
        let clocks = rcc.cfgr.sysclk(72.MHz()).hclk(72.MHz()).freeze(&mut flash.acr);
        let mono = Systick::new(core.SYST, clocks.hclk().raw());

        let ethernet = PartsIn {
            dma: p.ETHERNET_DMA,
            mac: p.ETHERNET_MAC,
            mmc: p.ETHERNET_MMC,
            ptp: p.ETHERNET_PTP,
        };
        let rx_ring = cx.local.rx_ring;
        let tx_ring = cx.local.tx_ring;
        let (rx_storage, tx_storage, socket_storage) = (
            cx.local.rx_storage,
            cx.local.tx_storage,
            cx.local.socket_storage,
        );

        defmt::info!("Setting up pins");

        let gpio = Gpio {
            gpioa: p.GPIOA.split(),
            gpiob: p.GPIOB.split(),
            gpioc: p.GPIOC.split(),
        };
        let Gpio {
            mut gpioa,
            mut gpiob,
            mut gpioc,
        } = gpio;
        let ref_clk = gpioa.pa1.into_floating_input(&mut gpioa.crl);
        let mdio = gpioa.pa2.into_alternate_push_pull(&mut gpioa.crl);
        let crs = gpioa.pa7.into_floating_input(&mut gpioa.crl);
        let mdc = gpioc.pc1.into_alternate_push_pull(&mut gpioc.crl);
        let rx_d0 = gpioc.pc4.into_floating_input(&mut gpioc.crl);
        let rx_d1 = gpioc.pc5.into_floating_input(&mut gpioc.crl);
        let tx_en = gpiob.pb11.into_alternate_push_pull(&mut gpiob.crh);
        let tx_d0 = gpiob.pb12.into_alternate_push_pull(&mut gpiob.crh);
        let tx_d1 = gpiob.pb13.into_alternate_push_pull(&mut gpiob.crh);
        let pins = EthPins {
            ref_clk,
            crs,
            tx_en,
            tx_d0,
            tx_d1,
            rx_d0,
            rx_d1,
        };

        defmt::info!("Configuring ethernet");
        let Parts {
            mut dma,
            mac,
            ..
        } = stm32_eth::new_with_mii(ethernet, rx_ring, tx_ring, clocks, pins, mdio, mdc).unwrap();

        defmt::info!("Enabling interrupts");
        dma.enable_interrupt();

        defmt::info!("Setting up smoltcp");

        let mut routes = smoltcp::iface::Routes::new();
        routes
            .add_default_ipv4_route(smoltcp::wire::Ipv4Address::UNSPECIFIED)
            .ok();

        let rx_buffer = TcpSocketBuffer::new(&mut rx_storage[..]);
        let tx_buffer = TcpSocketBuffer::new(&mut tx_storage[..]);

        let socket = TcpSocket::new(rx_buffer, tx_buffer);

        let config = iface::Config::new(EthernetAddress::from_bytes(&crate::MAC).into());

        let mut interface = Interface::new(config, &mut &mut dma, smoltcp::time::Instant::ZERO);
        interface.update_ip_addrs(|addr| {
            addr.push(IpCidr::Ipv4(Ipv4Cidr::new(crate::IP_ADDRESS, 24)))
                .ok();
        });

        let mut sockets = SocketSet::new(&mut socket_storage[..]);

        let tcp_handle = sockets.add(socket);

        let socket = sockets.get_mut::<TcpSocket>(tcp_handle);
        socket.listen(crate::SOCKET_ADDRESS).ok();

        interface.poll(now_fn(), &mut &mut dma, &mut sockets);

        let mut phy = LAN8720A::new(mac, 0);
        phy.phy_init();
        defmt::info!("Waiting for link up.");
        while !phy.phy_link_up() {}
        defmt::info!("Link up.");
        if let Some(speed) = phy.link_speed().map(|s| match s {
            PhySpeed::HalfDuplexBase10T => Speed::HalfDuplexBase10T,
            PhySpeed::FullDuplexBase10T => Speed::FullDuplexBase10T,
            PhySpeed::HalfDuplexBase100Tx => Speed::HalfDuplexBase100Tx,
            PhySpeed::FullDuplexBase100Tx => Speed::FullDuplexBase100Tx,
        }) {
            phy.get_miim().set_speed(speed);
            defmt::info!("Detected link speed: {}", speed);
        } else {
            defmt::warn!("Failed to detect link speed.");
        }

        defmt::info!("Setup done. Listening...");

        (
            Shared {},
            Local {
                interface,
                tcp_handle,
                dma,
                sockets,
            },
            init::Monotonics(mono),
        )
    }

    #[task(binds = ETH, local = [interface, tcp_handle, dma, sockets, data: [u8; 512] = [0u8; 512]], priority = 2)]
    fn eth_interrupt(cx: eth_interrupt::Context) {
        let (iface, tcp_handle, buffer, sockets, mut dma) = (
            cx.local.interface,
            cx.local.tcp_handle,
            cx.local.data,
            cx.local.sockets,
            cx.local.dma,
        );

        let interrupt_reason = stm32_eth::eth_interrupt_handler();
        defmt::debug!("Got an ethernet interrupt! Reason: {}", interrupt_reason);

        iface.poll(now_fn(), &mut dma, sockets);

        let socket = sockets.get_mut::<TcpSocket>(*tcp_handle);
        if let Ok(recv_bytes) = socket.recv_slice(buffer) {
            if recv_bytes > 0 {
                socket.send_slice(&buffer[..recv_bytes]).ok();
                defmt::info!("Echoed {} bytes.", recv_bytes);
            }
        }

        if !socket.is_listening() && !socket.is_open() || socket.state() == TcpState::CloseWait {
            socket.abort();
            socket.listen(crate::SOCKET_ADDRESS).ok();
            defmt::warn!("Disconnected... Reopening listening socket.");
        }

        iface.poll(now_fn(), &mut dma, sockets);
    }
}
