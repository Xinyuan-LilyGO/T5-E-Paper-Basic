<h1 align = "center">🏆 T5 e-Paper Basic 🏆</h1>

<p align="center">
  <b>English</b> | <a href="./README_CN.md">中文</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-6.5.0-ff7f00" height="20px">
  <img src="https://img.shields.io/badge/Arduino-ESP32--S3-008284" height="20px">
</p>

LILYGO T5 e-Paper Basic is an ESP32-S3 development board for a large-format parallel e-paper display. This repository contains PlatformIO examples for the display, XL9555 I/O expander, SD card, Wi-Fi, AXP2602 battery gauge, factory testing, and an employee badge application.

## :zero: Version

| Product | Repository board | Display | MCU | Flash / PSRAM |
| :--- | :--- | :--- | :--- | :--- |
| T5 e-Paper Basic | `T5-ePaper-S3` | E-paper, 4.7 inch class, 960 x 540 | ESP32-S3 | 16 MB / PSRAM |

The factory test application also contains a panel configuration for `1216 x 684`. Select the panel with `FACTORY_TEST_PANEL_1216X684`; the default factory-test build currently enables this option.

Product information: [LILYGO T5 e-Paper Basic]()

## :one: Product

| Item | Specification |
| :--- | :--- |
| MCU | ESP32-S3 |
| Flash | 16 MB |
| Display bus | 8-bit parallel EPD |
| Standard display configuration | 960 x 540 |
| Factory-test display configuration | 1216 x 684 (optional) |
| I2C devices | AXP2602 battery gauge, XL9555 I/O expander |
| Storage | TF / SD card through SPI |
| Wireless | 2.4 GHz Wi-Fi and Bluetooth LE provided by ESP32-S3 |
| USB | USB-C, USB CDC and download interface |
| Board power | USB-C and single-cell lithium battery interface |

Related resources:

- [M5GFX](https://github.com/m5stack/M5GFX): graphics and parallel EPD driver used by the examples
- [SensorLib](https://github.com/lewisxhe/SensorLib): AXP2602 and XL9555 support used by the examples

## :two: Update Program

### 2.1 Enter download mode

PlatformIO normally resets the board and enters download mode automatically. If manual operation is required:

1. Hold the `BOOT` button.
2. Press and release `RESET`.
3. Release `BOOT`.
4. Start the upload command again.

### 2.2 Flash a merged firmware image

Pre-compiled firmware is available in [`firmware/`](./firmware). A merged ESP32-S3 image can be written at address `0x0` with `esptool`:

```powershell
python -m esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 firmware\T5-E-Paper-Basic-factory_20260817.bin
```

Replace `COMx` and the firmware filename with the values for the target device.

## :three: Quick Start

PlatformIO is recommended for building the examples.

### 3.1 PlatformIO

1. Install [Visual Studio Code](https://code.visualstudio.com/), [Python](https://www.python.org/), and the PlatformIO extension.
2. Open this repository in Visual Studio Code.
3. Connect the board with a data-capable USB-C cable.
4. Select an example by changing `src_dir` in [`platformio.ini`](./platformio.ini).
5. Build or upload the project from PlatformIO, or run:

```powershell
pio run -e T5_E_PAPER_BASIC
pio run -e T5_E_PAPER_BASIC -t upload
```

The first build downloads the required PlatformIO dependencies, including `M5GFX` and `SensorLib`.

### 3.2 Arduino IDE

The project is maintained for PlatformIO. If using Arduino IDE, configure an ESP32-S3 board with settings equivalent to the repository board definition:

| Arduino IDE setting | Value |
| :--- | :--- |
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240 MHz |
| Flash Mode | QIO |
| Flash Size | 16 MB |
| Partition Scheme | 3 MB APP / FATFS |
| PSRAM | Enabled |
| Upload Speed | 921600 |
| USB Mode | CDC and JTAG |

### 3.3 Folder structure

~~~text
boards/    PlatformIO board definition
docs/      Pin mapping and factory-test documentation
examples/  Hardware and application examples
firmware/  Pre-compiled firmware images
hardware/  Schematics and component datasheets
lib/       Local libraries and headers
other/     Test images, notes, and development references
script/    PlatformIO build scripts
~~~

### 3.4 Examples

| Example | Path | Description |
| :--- | :--- | :--- |
| Factory test | [`examples/factory_test`](./examples/factory_test) | Combined display, XL9555, SD, Wi-Fi, battery, and button test |
| EPD display test | [`examples/epd_display_test`](./examples/epd_display_test) | Display geometry, grayscale, typography, and pattern pages |
| Wi-Fi scan | [`examples/wifi_scan`](./examples/wifi_scan) | Scan nearby Wi-Fi networks |
| SD test | [`examples/sd_test`](./examples/sd_test) | Test TF / SD card access |
| AXP2602 test | [`examples/axp2602_test`](./examples/axp2602_test) | Battery voltage, current, SOC, SOH, and gauge status |
| Employee badge | [`examples/employee_badge`](./examples/employee_badge) | Render a static employee badge with an embedded logo and QR code |

## :four: Pins

The complete pin map is maintained in [`docs/pinmap_cn.md`](./docs/pinmap_cn.md). The main connections are:

| Function | Pin |
| :--- | :--- |
| I2C SDA | GPIO3 |
| I2C SCL | GPIO2 |
| XL9555 interrupt | GPIO1 |
| AXP2602 interrupt | GPIO21 |
| EPD data D0..D7 | GPIO6, 14, 7, 12, 9, 11, 8, 10 |
| EPD XSTL | GPIO13 |
| EPD SPV / CKV | GPIO17 / GPIO18 |
| EPD power / boost enable | GPIO45 / GPIO46 |
| SD MOSI / SCK / MISO / CS | GPIO38 / GPIO39 / GPIO40 / GPIO47 |
| BOOT button | GPIO0 |

## :five: Factory Test

The factory-test firmware is selected by default in `platformio.ini`:

```ini
src_dir = examples/factory_test
```

It performs the following checks:

- E-paper initialization and display refresh
- XL9555 I/O expander communication and `BTN0` to `BTN4`
- TF / SD card detection, mount, read, write, and cleanup
- Wi-Fi scan and optional connection to configured test access points
- AXP2602 battery gauge initialization and telemetry
- Direct `BOOT` button input on GPIO0

See the detailed procedure in [`docs/factory_test_procedure_cn.md`](./docs/factory_test_procedure_cn.md).

For serial diagnostics:

```powershell
pio device monitor --baud 115200
```

## :six: FAQ

| Problem | Check |
| :--- | :--- |
| Display is blank | Confirm the EPD cable, USB power, board definition, and panel resolution configuration. |
| Display has lines or severe ghosting | Power-cycle the board, check the panel cable, and allow the full refresh to finish. |
| SD card is not detected | Use a FAT32 card and check MOSI `GPIO38`, SCK `GPIO39`, MISO `GPIO40`, CS `GPIO47`, and XL9555 `P05` card detect. |
| Wi-Fi scan finds no network | Use a 2.4 GHz access point; ESP32-S3 does not scan 5 GHz-only networks. |
| Buttons do not respond | Check I2C on GPIO2/GPIO3 and the XL9555 connections on `P00` to `P04`. |
| Upload fails | Enter download mode manually and retry with a data-capable USB cable. |

## :seven: Schematic and Hardware

Hardware documents are available in [`hardware/`](./hardware):

- [T5 e-Paper Basic schematic](./hardware/T5%20e-Paper%20Basic.pdf)
- [E0470A03-AF-S panel datasheet](./hardware/E0470A03-AF-S%20A%E7%89%88%E8%A7%84%E6%A0%BC%E4%B9%A6.pdf)
- [E0470A01-AF-CF panel datasheet](./hardware/E0470A01-AF-CF%28A%29%281%29.pdf)
- [AXP2602 design guide](./hardware/AXP2602%E8%AE%BE%E8%AE%A1%E6%8C%87%E5%8D%97_V1.0.pdf)

Board definition: [`boards/T5-ePaper-S3.json`](./boards/T5-ePaper-S3.json)
