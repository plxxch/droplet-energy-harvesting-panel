# Droplet Energy Harvesting Panel — Wireless Smoke/Temperature Sensor

Firmware for the self-powered wireless forest monitoring system described in:

> X. Xu, P. Li, Y. Ding, W. Xu, S. Liu, Z. Zhang, Z. Wang, and Z. Yang,
> “Droplet energy harvesting panel,” *Energy & Environmental Science*,
> vol. 15, no. 7, pp. 2916–2926, 2022.
> DOI: [10.1039/d2ee00357k](https://doi.org/10.1039/d2ee00357k)

## Overview

A droplet-energy-harvesting (DEH) panel charges a storage capacitor, which
powers a wireless sensor node. The node measures temperature and smoke and
transmits the values to a receiver connected to a laptop.

| Component | Part |
|---|---|
| Power management | LTC3588-1, 10 mF storage capacitor (charged to ~10.69 V) |
| Transmitter | TI CC430F5137, sub-1 GHz radio |
| Receiver | TI CC430F6137, connected to a laptop over UART |
| Temperature sensor | Internal CC430 sensor, ADC12 channel A10 |
| Smoke sensor | MAX30105 optical sensor, I2C address `0x57` |
| Display / logging | VOFA+ (open-source serial plotter) |

> **Frequency:** the RF settings are for **433 MHz**. The original CCS project
> inherited the TI example configuration names `MHZ_868` / `MHZ_915`; those are
> legacy labels, not the actual operating frequency. The build configurations
> here are named `433MHz_Alarm` and `433MHz_Raw`.

## Two packet variants

The same hardware supports two payload formats, selected by the CCS build
configuration:

| Build configuration | Payload | Purpose |
|---|---|---|
| `433MHz_Alarm` | 3 bytes: `[temp_hi, temp_lo, smoke_alarm]` | Minimum airtime; smoke is a 0/1 alarm |
| `433MHz_Raw` | 5 bytes: `[smoke0, smoke1, smoke2, temp_hi, temp_lo]` | Raw MAX30105 value for analysis |

- **3-byte variant** — the node keeps the temperature reading and a smoke
  threshold flag. This is the lower-energy option for a battery-free node.
- **5-byte variant** — the node transmits the raw 24-bit MAX30105 reading plus
  the temperature. This is the format used for the raw smoke/temperature
  traces.

Both variants use the same sensor/receiver hardware. The only differences are
the payload packing in the main source file and `PKTLEN` in `HAL/RF1A.c`.

## Repository layout

```
.
├── sensor/                     # CCS project: smoke/temperature sensor node
│   ├── main_alarm.c            # 3-byte variant
│   ├── main_raw.c              # 5-byte variant
│   ├── smoke-sensor.h
│   ├── smoke_config.h          # RF_PACKET_LEN selection
│   ├── HAL/RF1A.c              # TI RF1A driver (BSD), PKTLEN = RF_PACKET_LEN
│   └── ...
├── receiver/                   # CCS project: receiver node
│   ├── main_alarm.c            # parses the 3-byte packet
│   ├── main_raw.c              # parses the 5-byte packet
│   ├── smoke-receiver.h
│   ├── smoke_config.h
│   ├── HAL/                    # RF1A, PMM, port mapper (BSD)
│   └── ...
├── THIRD_PARTY_NOTICES.md
├── CITATION.cff
└── LICENSE
```

## Packet formats

### 3-byte (`433MHz_Alarm`)

| Byte | Content |
|---|---|
| 0 | Temperature high byte |
| 1 | Temperature low byte |
| 2 | Smoke alarm (`0` / `1`) |

### 5-byte (`433MHz_Raw`)

| Byte | Content |
|---|---|
| 0–2 | Raw MAX30105 reading (24-bit) |
| 3 | Temperature high byte |
| 4 | Temperature low byte |

## UART output

The receiver prints:

```
Data:<channel1>,<channel2>\n
```

- `433MHz_Alarm`: `channel1` = smoke alarm (0/1), `channel2` = temperature
- `433MHz_Raw`: `channel1` = raw smoke value, `channel2` = temperature

UART is **9600 8N1** on P1.5/P1.6, clocked from ACLK. VOFA+ can read the
comma-separated values directly.

## Building

1. Install **Code Composer Studio 8.3.0** (or a compatible version) with
   MSP430 support.
2. Import `sensor/` and `receiver/` with
   `File → Import → CCS Projects`.
3. Select the build configuration:
   - `433MHz_Alarm` → 3-byte temperature + smoke alarm
   - `433MHz_Raw` → 5-byte raw smoke + temperature
4. Build and flash with an MSP-FET / eZ430 probe.

The build configuration defines `RF_PACKET_LEN` (`3` or `5`), excludes the
other main file, and selects the matching linker script. `HAL/RF1A.c` uses
`RF_PACKET_LEN` for the `PKTLEN` register, so the radio and the payload stay
in sync.

No external files need to be downloaded: all third-party source code in this
repository is BSD-licensed.

> **Hardware note:** the 3-byte and 5-byte variants use the same RF settings
> except for the packet length. Verify both configurations on hardware before
> relying on them.

> **Toolchain note:** the original CCS workspace stored the two build
> configurations with different TI compiler toolchain versions. If CCS reports
> a toolchain mismatch, set both configurations to the same MSP430 compiler
> version under *Project Properties → Build → Compiler version*.

## License

The MIT license in [LICENSE](LICENSE) covers the original code authored by
Pengyu Li. Third-party files remain under their own licenses — see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Citation

```bibtex
@article{xu2022droplet,
  author  = {Xu, Xiaote and Li, Pengyu and Ding, Yongtao and Xu, Wanghuai and Liu, Shiyuan and Zhang, Zhuomin and Wang, Zuankai and Yang, Zhengbao},
  title   = {Droplet energy harvesting panel},
  journal = {Energy \& Environmental Science},
  volume  = {15},
  number  = {7},
  pages   = {2916--2926},
  year    = {2022},
  doi     = {10.1039/d2ee00357k}
}
```
