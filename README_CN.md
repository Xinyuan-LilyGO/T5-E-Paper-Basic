<h1 align = "center">🏆 T5 e-Paper Basic 🏆</h1>

<p align="center">
  <a href="./README.md">English</a> | <b>中文</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-6.5.0-ff7f00" height="20px">
  <img src="https://img.shields.io/badge/Arduino-ESP32--S3-008284" height="20px">
</p>

LILYGO T5 e-Paper Basic 是一款基于 ESP32-S3 的大尺寸并口电子纸开发板。本仓库提供电子纸屏幕、XL9555 IO 扩展、SD 卡、Wi-Fi、AXP2602 电量计、出厂测试和电子工牌等 PlatformIO 示例。

## :zero: 版本

| 产品 | 仓库板卡 | 屏幕 | MCU | Flash / PSRAM |
| :--- | :--- | :--- | :--- | :--- |
| T5 e-Paper Basic | `T5-ePaper-S3` | 4.7 英寸级电子纸，960 x 540 | ESP32-S3 | 16 MB / PSRAM |

出厂测试程序同时包含 `1216 x 684` 面板配置。通过 `FACTORY_TEST_PANEL_1216X684` 选择屏幕配置；当前出厂测试默认启用该选项。

产品信息：[LILYGO T5 e-Paper Basic]()

## :one: 产品参数

| 项目 | 规格 |
| :--- | :--- |
| MCU | ESP32-S3 |
| Flash | 16 MB |
| 屏幕总线 | 8-bit 并口 EPD |
| 标准屏幕配置 | 960 x 540 |
| 出厂测试屏幕配置 | 1216 x 684（可选） |
| I2C 设备 | AXP2602 电量计、XL9555 IO 扩展芯片 |
| 存储 | 通过 SPI 连接 TF / SD 卡 |
| 无线功能 | ESP32-S3 提供 2.4 GHz Wi-Fi 和 Bluetooth LE |
| USB | USB-C、USB CDC 和下载接口 |
| 供电 | USB-C 和单节锂电池接口 |

相关项目：[M5GFX](https://github.com/m5stack/M5GFX)、[SensorLib](https://github.com/lewisxhe/SensorLib)。

## :two: 更新程序

### 2.1 进入下载模式

PlatformIO 通常可以自动复位并进入下载模式。手动操作时：

1. 按住 `BOOT`。
2. 按下并释放 `RESET`。
3. 释放 `BOOT`。
4. 重新执行烧录命令。

### 2.2 烧录合并固件

预编译固件位于 [`firmware/`](./firmware)，可使用 `esptool` 从地址 `0x0` 开始烧录：

```powershell
python -m esptool --chip esp32s3 --port COMx --baud 921600 write_flash 0x0 firmware\T5-E-Paper-Basic-factory_20260817.bin
```

请将 `COMx` 和固件文件名替换为实际值。

## :three: 快速开始

推荐使用 PlatformIO。

### 3.1 PlatformIO

1. 安装 [Visual Studio Code](https://code.visualstudio.com/)、[Python](https://www.python.org/) 和 PlatformIO 扩展。
2. 打开本仓库，并使用支持数据传输的 USB-C 线连接开发板。
3. 修改 [`platformio.ini`](./platformio.ini) 中的 `src_dir`，选择示例。
4. 执行：

```powershell
pio run -e T5_E_PAPER_BASIC
pio run -e T5_E_PAPER_BASIC -t upload
```

第一次编译时会自动下载 `M5GFX`、`SensorLib` 等依赖库。

### 3.2 Arduino IDE

本项目主要针对 PlatformIO 维护。Arduino IDE 可参考以下设置：

| 设置 | 值 |
| :--- | :--- |
| 开发板 | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240 MHz |
| Flash Mode / Size | QIO / 16 MB |
| Partition Scheme | 3 MB APP / FATFS |
| PSRAM | Enabled |
| Upload Speed | 921600 |
| USB Mode | CDC and JTAG |

### 3.3 目录结构

~~~text
boards/    PlatformIO 板卡定义
docs/      引脚映射和出厂测试文档
examples/  硬件及应用示例
firmware/  预编译固件
hardware/  原理图和芯片规格书
lib/       本地库和头文件
other/     测试图片和开发参考资料
script/    PlatformIO 构建脚本
~~~

### 3.4 示例程序

| 示例 | 路径 | 说明 |
| :--- | :--- | :--- |
| 出厂测试 | [`examples/factory_test`](./examples/factory_test) | 综合测试屏幕、XL9555、SD 卡、Wi-Fi、电量和按键 |
| EPD 屏幕测试 | [`examples/epd_display_test`](./examples/epd_display_test) | 测试几何图形、灰阶、文字和图案 |
| Wi-Fi 扫描 | [`examples/wifi_scan`](./examples/wifi_scan) | 扫描附近的 Wi-Fi 网络 |
| SD 卡测试 | [`examples/sd_test`](./examples/sd_test) | 测试 TF / SD 卡访问 |
| AXP2602 测试 | [`examples/axp2602_test`](./examples/axp2602_test) | 测试电池电压、电流、SOC、SOH 和电量计状态 |
| 电子工牌 | [`examples/employee_badge`](./examples/employee_badge) | 绘制静态电子工牌和二维码 |

## :four: 引脚

完整引脚表请参阅 [`docs/pinmap_cn.md`](./docs/pinmap_cn.md)。主要连接如下：

| 功能 | 引脚 |
| :--- | :--- |
| I2C SDA / SCL | GPIO3 / GPIO2 |
| XL9555 中断 | GPIO1 |
| AXP2602 中断 | GPIO21 |
| EPD 数据 D0..D7 | GPIO6、14、7、12、9、11、8、10 |
| EPD XSTL | GPIO13 |
| EPD SPV / CKV | GPIO17 / GPIO18 |
| EPD 电源 / 升压使能 | GPIO45 / GPIO46 |
| SD MOSI / SCK / MISO / CS | GPIO38 / GPIO39 / GPIO40 / GPIO47 |
| BOOT 按键 | GPIO0 |

## :five: 出厂测试

`platformio.ini` 默认选择：

```ini
src_dir = examples/factory_test
```

程序包含电子纸、XL9555、`BTN0` 至 `BTN4`、TF / SD 卡、Wi-Fi、AXP2602 和 GPIO0 `BOOT` 按键测试。详细流程请参阅 [`docs/factory_test_procedure_cn.md`](./docs/factory_test_procedure_cn.md)。

查看串口日志：

```powershell
pio device monitor --baud 115200
```

## :six: 常见问题

| 问题 | 检查项 |
| :--- | :--- |
| 屏幕无显示 | 检查电子纸排线、USB 供电、板卡定义和面板分辨率配置。 |
| 屏幕出现横线或严重残影 | 重新上电，检查屏幕排线，并等待完整刷新结束。 |
| SD 卡无法识别 | 使用 FAT32 格式，并检查 GPIO38/39/40/47 及 XL9555 `P05` 卡检测信号。 |
| Wi-Fi 扫描不到热点 | 使用 2.4 GHz 热点，ESP32-S3 不支持仅 5 GHz 的网络。 |
| 按键没有反应 | 检查 GPIO2/GPIO3 的 I2C 连接，以及 XL9555 `P00` 至 `P04`。 |
| 无法上传程序 | 手动进入下载模式，并更换支持数据传输的 USB 线。 |

## :seven: 原理图和硬件资料

硬件资料位于 [`hardware/`](./hardware)：

- [T5 e-Paper Basic 原理图](./hardware/T5%20e-Paper%20Basic.pdf)
- [E0470A03-AF-S 屏幕规格书](./hardware/E0470A03-AF-S%20A%E7%89%88%E8%A7%84%E6%A0%BC%E4%B9%A6.pdf)
- [E0470A01-AF-CF 屏幕规格书](./hardware/E0470A01-AF-CF%28A%29%281%29.pdf)
- [AXP2602 设计指南](./hardware/AXP2602%E8%AE%BE%E8%AE%A1%E6%8C%87%E5%8D%97_V1.0.pdf)

板卡定义：[`boards/T5-ePaper-S3.json`](./boards/T5-ePaper-S3.json)