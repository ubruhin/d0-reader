#![no_std]
#![no_main]

use defmt_rtt as _;
use panic_halt as _;

use core::{cell::RefCell, fmt::Write};
use cortex_m::{asm, interrupt::Mutex};
use cortex_m_rt::{entry, exception};

use stm32f1xx_hal::{
    afio::AfioExt, gpio::Alternate, gpio::Pin, gpio::PinState, pac, pac::USART3,
    prelude::_stm32_hal_time_U32Ext, serial::Config as SerialConfig, serial::Serial,
};

use stm32_eth::{
    dma::{RxRingEntry, TxRingEntry},
    hal::{flash::FlashExt, gpio::GpioExt, rcc::RccExt},
    stm32::{interrupt, CorePeripherals, Peripherals, SYST},
    EthPins, Parts, PartsIn,
};

use smoltcp::{
    iface::{Config as InterfaceConfig, Interface, SocketSet, SocketStorage},
    socket::tcp::{Socket as TcpSocket, SocketBuffer as TcpSocketBuffer},
    time::{Duration, Instant},
    wire::{EthernetAddress, IpCidr, Ipv4Address, Ipv4Cidr},
};

use fugit::RateExtU32;

struct Gpio {
    pub gpioa: stm32_eth::hal::gpio::gpioa::Parts,
    pub gpiob: stm32_eth::hal::gpio::gpiob::Parts,
    pub gpioc: stm32_eth::hal::gpio::gpioc::Parts,
}

const IP_ADDRESS: Ipv4Address = Ipv4Address::new(192, 168, 1, 51);
const MAC: [u8; 6] = [0x42, 0x42, 0x42, 0x42, 0x42, 0x01];

static TIME: Mutex<RefCell<u64>> = Mutex::new(RefCell::new(0));
static TIMEOUT: Mutex<RefCell<u32>> = Mutex::new(RefCell::new(0));
static ETH_PENDING: Mutex<RefCell<bool>> = Mutex::new(RefCell::new(false));

static mut SERIAL: Option<Serial<USART3, (Pin<'C', 10, Alternate>, Pin<'C', 11>)>> = None;

const RX_BUFFER_SIZE: usize = 512;
static mut RX_BUFFER: &mut [u8; RX_BUFFER_SIZE] = &mut [0; RX_BUFFER_SIZE];
static mut RX_LENGTH: usize = 0;
static mut RX_ERRORS: u32 = 0;

#[entry]
unsafe fn main() -> ! {
    defmt::info!("Init core peripherals");
    let p = Peripherals::take().unwrap();
    let mut core = CorePeripherals::take().unwrap();
    let rcc = p.RCC.constrain();
    let mut flash = p.FLASH.constrain();
    let clocks = rcc
        .cfgr
        .sysclk(36.MHz())
        .hclk(36.MHz())
        .pclk1(18.MHz())
        .pclk2(18.MHz())
        .freeze(&mut flash.acr);
    let mut afio = p.AFIO.constrain();

    defmt::info!("Setting up SysTick");
    setup_systick(&mut core.SYST);

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
    let mut led = gpiob
        .pb14
        .into_push_pull_output_with_state(&mut gpiob.crh, PinState::High);
    let ir_tx = gpioc.pc10.into_alternate_push_pull(&mut gpioc.crh);
    let ir_rx = gpioc.pc11;
    let ref_clk = gpioa.pa1.into_floating_input(&mut gpioa.crl);
    let _mdio = gpioa.pa2.into_alternate_push_pull(&mut gpioa.crl);
    let crs = gpioa.pa7.into_floating_input(&mut gpioa.crl);
    let _mdc = gpioc.pc1.into_alternate_push_pull(&mut gpioc.crl);
    let rx_d0 = gpioc.pc4.into_floating_input(&mut gpioc.crl);
    let rx_d1 = gpioc.pc5.into_floating_input(&mut gpioc.crl);
    let tx_en = gpiob.pb11.into_alternate_push_pull(&mut gpiob.crh);
    let tx_d0 = gpiob.pb12.into_alternate_push_pull(&mut gpiob.crh);
    let tx_d1 = gpiob.pb13.into_alternate_push_pull(&mut gpiob.crh);
    let eth_pins = EthPins {
        ref_clk,
        crs,
        tx_en,
        tx_d0,
        tx_d1,
        rx_d0,
        rx_d1,
    };

    defmt::info!("Configuring UART");
    SERIAL.replace(Serial::new(
        p.USART3,
        (ir_tx, ir_rx),
        &mut afio.mapr,
        SerialConfig::default(),
        &clocks,
    ));
    cortex_m::peripheral::NVIC::unmask(pac::Interrupt::USART3);

    defmt::info!("Configuring ethernet");
    let ethernet = PartsIn {
        dma: p.ETHERNET_DMA,
        mac: p.ETHERNET_MAC,
        mmc: p.ETHERNET_MMC,
        ptp: p.ETHERNET_PTP,
    };
    let mut rx_ring: [RxRingEntry; 2] = Default::default();
    let mut tx_ring: [TxRingEntry; 2] = Default::default();
    let Parts {
        mut dma,
        mac: _,
        ptp: _,
    } = stm32_eth::new(
        ethernet,
        &mut rx_ring[..],
        &mut tx_ring[..],
        clocks,
        eth_pins,
    )
    .unwrap();

    defmt::info!("Enabling interrupts");
    dma.enable_interrupt();

    defmt::info!("Configuring smoltcp");
    let ethernet_addr = EthernetAddress(MAC);
    let config = InterfaceConfig::new(ethernet_addr.into());
    let mut iface = Interface::new(config, &mut &mut dma, Instant::ZERO);
    iface.update_ip_addrs(|addr| {
        addr.push(IpCidr::Ipv4(Ipv4Cidr::new(IP_ADDRESS, 24))).ok();
    });
    let mut sockets = [SocketStorage::EMPTY];
    let mut sockets = SocketSet::new(&mut sockets[..]);
    let mut server_rx_buffer = [0; 512];
    let mut server_tx_buffer = [0; 512];
    let mut server_socket = TcpSocket::new(
        TcpSocketBuffer::new(&mut server_rx_buffer[..]),
        TcpSocketBuffer::new(&mut server_tx_buffer[..]),
    );
    server_socket.set_keep_alive(Some(Duration::from_millis(5000)));
    server_socket.set_timeout(Some(Duration::from_millis(12000)));
    let server_handle = sockets.add(server_socket);

    defmt::info!("Setup done, ready for connections!");

    let mut last_tick: u64 = 0;

    enum MeterState {
        Idle,
        WaitForIdResponse,
        WaitForData,
    }
    let mut meter_state: MeterState = MeterState::Idle;

    let rx_data: &mut [u8; RX_BUFFER_SIZE] = &mut [0; RX_BUFFER_SIZE];
    let mut rx_length: usize = 0;

    loop {
        let time: u64 = cortex_m::interrupt::free(|cs| *TIME.borrow(cs).borrow());
        cortex_m::interrupt::free(|cs| {
            let mut eth_pending = ETH_PENDING.borrow(cs).borrow_mut();
            *eth_pending = false;
        });

        cortex_m::interrupt::free(|_cs| {
            *rx_data = RX_BUFFER.clone();
            rx_length = RX_LENGTH;
            RX_LENGTH = 0;
        });

        iface.poll(
            Instant::from_millis(time as i64),
            &mut &mut dma,
            &mut sockets,
        );

        let socket = sockets.get_mut::<TcpSocket>(server_handle);

        if !socket.is_listening() && !socket.is_open() {
            socket.abort();
            if let Err(e) = socket.listen(80) {
                defmt::error!("TCP listen error: {:?}", e)
            } else {
                defmt::info!("Listening at {}:80...", IP_ADDRESS);
            }
        } else if (last_tick != time) && (time % 1000 == 0) {
            last_tick = time;
            led.toggle();
        }

        if rx_length > 0 {
            led.toggle();
            if socket.can_send() {
                let _result = socket.send_slice(&rx_data[0..rx_length]);
            }
        }

        match meter_state {
            MeterState::Idle => {
                let _ = SERIAL
                    .as_mut()
                    .unwrap()
                    .reconfigure(
                        SerialConfig::default()
                            .baudrate(300_u32.bps())
                            .wordlength_8bits()
                            .parity_even(),
                        &clocks,
                    )
                    .unwrap();
                SERIAL.as_mut().unwrap().tx.write_str("/?!\r\n").unwrap();
                while !SERIAL.as_mut().unwrap().tx.is_tx_complete() {}
                set_timeout_ms(1200);
                meter_state = MeterState::WaitForIdResponse;
            }
            MeterState::WaitForIdResponse => {
                if is_timeout_elapsed() {
                    SERIAL
                        .as_mut()
                        .unwrap()
                        .tx
                        .write_str("\x06060\r\n")
                        .unwrap();
                    while !SERIAL.as_mut().unwrap().tx.is_tx_complete() {}
                    let _ = SERIAL
                        .as_mut()
                        .unwrap()
                        .reconfigure(
                            SerialConfig::default()
                                .baudrate(19200_u32.bps())
                                .wordlength_8bits()
                                .parity_even(),
                            &clocks,
                        )
                        .unwrap();
                    RX_LENGTH = 0;
                    RX_ERRORS = 0;
                    SERIAL.as_mut().unwrap().rx.listen();
                    set_timeout_ms(6800);
                    meter_state = MeterState::WaitForData;
                }
            }
            MeterState::WaitForData => {
                if is_timeout_elapsed() {
                    SERIAL.as_mut().unwrap().rx.unlisten();
                    meter_state = MeterState::Idle;
                    if socket.can_send() {
                        let _ = socket.send_slice(b"ERRORS: ");
                        send_int(socket, RX_ERRORS);
                        let _ = socket.send_slice(b"\r\n");
                    }
                }
            }
        }

        asm::wfi();
    }
}

fn send_int(socket: &mut TcpSocket, number: u32) {
    let mut decimals = 1;
    let mut divisor = 1;
    while number / divisor > 9 {
        decimals += 1;
        divisor *= 10;
    }
    let mut remaining = number;
    let mut s = [0; 16];
    for i in 0..decimals {
        let digit = remaining / divisor;
        remaining -= digit * divisor;
        divisor /= 10;
        s[i] = b'0' + (digit as u8);
    }
    let _ = socket.send_slice(&s);
}

fn setup_systick(syst: &mut SYST) {
    syst.set_reload(4500);
    syst.enable_counter();
    syst.enable_interrupt();
}

fn set_timeout_ms(value: u32) {
    cortex_m::interrupt::free(|cs| {
        *TIMEOUT.borrow(cs).borrow_mut() = value;
    });
}

fn is_timeout_elapsed() -> bool {
    let mut tmp = 1;
    cortex_m::interrupt::free(|cs| {
        tmp = *TIMEOUT.borrow(cs).borrow();
    });
    return tmp == 0;
}

#[exception]
fn SysTick() {
    cortex_m::interrupt::free(|cs| {
        let mut time = TIME.borrow(cs).borrow_mut();
        *time += 1;
        let mut timeout = TIMEOUT.borrow(cs).borrow_mut();
        if *timeout > 0 {
            *timeout -= 1;
        }
    })
}

#[interrupt]
unsafe fn USART3() {
    if let Some(serial) = SERIAL.as_mut() {
        while serial.rx.is_rx_not_empty() {
            if let Ok(w) = serial.rx.read() {
                if RX_LENGTH < RX_BUFFER_SIZE {
                    RX_BUFFER[RX_LENGTH] = w & 0x7F;
                    RX_LENGTH += 1;
                } else {
                    RX_ERRORS += 1;
                }
            } else {
                RX_ERRORS += 1;
            }
        }
    }
}

#[interrupt]
fn ETH() {
    cortex_m::interrupt::free(|cs| {
        let mut eth_pending = ETH_PENDING.borrow(cs).borrow_mut();
        *eth_pending = true;
    });

    stm32_eth::eth_interrupt_handler();
}
