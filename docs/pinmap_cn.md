# T5 e-Paper Basic 引脚映射

> 本文根据 `hardware/T5 e-Paper Basic.pdf` 整理。

## 1. 主控直连 GPIO

| 功能 | 建议名 | GPIO / 信号 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| 主 I2C SDA | `PIN_I2C_SDA` | `GPIO3` | 原理图 U9 / U1 / U7 / J4 / J7 / J8 / P3 | 板级网络 `SDA`，供 AXP2602、XL9555 和外部 I2C 接口共用 |
| 主 I2C SCL | `PIN_I2C_SCL` | `GPIO2` | 原理图 U9 / U1 / U7 / J4 / J7 / J8 / P3 | 板级网络 `SCL`，全板主 I2C 时钟 |
| 扩展 IO 中断 / 功能键 | `PIN_EXT_IRQ` | `GPIO1` | 原理图 U9 / U7 / KEY7 | 板级网络 `EX_IRQ`，与 `XL9555 INT#`、KEY7 共线 |
| 充电状态输入 | `PIN_CHG_STAT` | `GPIO4` | 原理图 U9 / 充电电路 | 板级网络 `CHG_STAT` |
| USB VBUS 检测 | `PIN_USB_DET` | `GPIO5` | 原理图 U9 / USB-C 电路 | 板级网络 `USB_DET` |
| EPD 数据 D0 | `PIN_EPD_DB0` | `GPIO6` | 原理图 U9 / J6 | 板级网络 `EPD_DB0` |
| EPD 数据 D2 | `PIN_EPD_DB2` | `GPIO7` | 原理图 U9 / J6 | 板级网络 `EPD_DB2` |
| EPD 数据 D6 | `PIN_EPD_DB6` | `GPIO8` | 原理图 U9 / J6 | 板级网络 `EPD_DB6` |
| EPD 数据 D4 | `PIN_EPD_DB4` | `GPIO9` | 原理图 U9 / J6 | 板级网络 `EPD_DB4` |
| EPD 数据 D7 | `PIN_EPD_DB7` | `GPIO10` | 原理图 U9 / J6 | 板级网络 `EPD_DB7` |
| EPD 数据 D5 | `PIN_EPD_DB5` | `GPIO11` | 原理图 U9 / J6 | 板级网络 `EPD_DB5` |
| EPD 数据 D3 | `PIN_EPD_DB3` | `GPIO12` | 原理图 U9 / J6 | 板级网络 `EPD_DB3` |
| EPD XSTL | `PIN_EPD_XSTL` | `GPIO13` | 原理图 U9 / J6 | 板级网络 `EPD_XSTL` |
| EPD 数据 D1 | `PIN_EPD_DB1` | `GPIO14` | 原理图 U9 / J6 | 板级网络 `EPD_DB1` |
| USB D- | `PIN_USB_DM` | `GPIO19` | 原理图 U9 / USB1 | 板级网络 `USB_DM` |
| USB D+ | `PIN_USB_DP` | `GPIO20` | 原理图 U9 / USB1 | 板级网络 `USB_DP` |
| AXP2602 中断 | `PIN_AXP2602_INT` | `GPIO21` | 原理图 U9 / U1 / J5 | 原理图网名写作 `Guage_IRQ`，接 AXP2602 `IRQ` |
| GPIO33 扩展 | `PIN_GPIO33` | `GPIO33` | 原理图 U9 / J4 | J4 标注 `DIN` |
| GPIO34 扩展 | `PIN_GPIO34` | `GPIO34` | 原理图 U9 / J4 | J4 标注 `LRCK` |
| GPIO35 扩展 | `PIN_GPIO35` | `GPIO35` | 原理图 U9 / J4 | J4 标注 `DOUT` |
| GPIO36 扩展 | `PIN_GPIO36` | `GPIO36` | 原理图 U9 / J4 | J4 标注 `SCLK` |
| GPIO37 扩展 | `PIN_GPIO37` | `GPIO37` | 原理图 U9 / J4 | J4 标注 `MCLK` |
| SD SPI MOSI | `PIN_SD_MOSI` | `GPIO38` | 原理图 U9 / J2 / J5 | 板级网络 `CARD_MOSI` |
| SD SPI SCK | `PIN_SD_SCK` | `GPIO39` (`MTCK`) | 原理图 U9 / J2 / J5 | 板级网络 `CARD_SCK` |
| SD SPI MISO | `PIN_SD_MISO` | `GPIO40` (`MTDO`) | 原理图 U9 / J2 / J5 | 板级网络 `CARD_MISO` |
| GPIO41 扩展 | `PIN_GPIO41` | `GPIO41` (`MTDI`) | 原理图 U9 / J5 | J5 标注 `LRCK` |
| GPIO42 扩展 | `PIN_GPIO42` | `GPIO42` (`MTMS`) | 原理图 U9 / J5 | J5 标注 `DOUT` |
| UART0 TX | `PIN_UART_TXD` | `GPIO43` (`U0TXD`) | 原理图 U9 / P2 | 串口下载与日志输出 |
| UART0 RX | `PIN_UART_RXD` | `GPIO44` (`U0RXD`) | 原理图 U9 / P2 | 串口下载与日志输入 |
| EPD 逻辑电源控制 | `PIN_EPD_PWR` | `GPIO45` | 原理图 U9 / J6 | 板级网络 `EPD_PWR` |
| 升压使能 | `PIN_BST_EN` | `GPIO46` | 原理图 U9 / 升压电路 | 板级网络 `BST_EN` |
| SD SPI CS | `PIN_SD_CS` | `GPIO47` (`SPICLK_P`) | 原理图 U9 / J2 | 板级网络 `CARD_CS` |
| GPIO48 扩展 | `PIN_GPIO48` | `GPIO48` (`SPICLK_N`) | 原理图 U9 / J5 | J5 标注 `SCLK` |
| 复位使能 | `PIN_CHIP_PU` | `CHIP_PU` | 原理图 U9 / KEY6 | 不是普通 GPIO，KEY6 为复位键 |

> 说明：
> `GPIO0` 在当前原理图中只看到同名网络，未见板载按键或排针引出；若实物行为与此不同，应以 PCB 丝印和自动下载电路为准。

## 2. 墨水屏接口与电源

### 2.1 直接由主控驱动的 EPD 信号

| 功能 | 建议名 | GPIO / 信号 | J6 引脚 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- |
| EPD XLE | `PIN_EPD_XLE` | `XTAL_32K_P` | 11 | 原理图 U9 / J6 | 32K 晶振脚被复用为 EPD 锁存信号 |
| EPD XCL | `PIN_EPD_XCL` | `XTAL_32K_N` | 10 | 原理图 U9 / J6 | 经 `R53 100R` 串联后输出 |
| EPD SPV | `PIN_EPD_SPV` | `GPIO17` | 37 | 原理图 U9 / J6 | 板级网络 `EPD_SPV` |
| EPD CKV | `PIN_EPD_CKV` | `GPIO18` | 36 | 原理图 U9 / J6 | 板级网络 `EPD_CKV` |
| EPD D0 | `PIN_EPD_DB0` | `GPIO6` | 14 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D1 | `PIN_EPD_DB1` | `GPIO14` | 15 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D2 | `PIN_EPD_DB2` | `GPIO7` | 16 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D3 | `PIN_EPD_DB3` | `GPIO12` | 17 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D4 | `PIN_EPD_DB4` | `GPIO9` | 18 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D5 | `PIN_EPD_DB5` | `GPIO11` | 19 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D6 | `PIN_EPD_DB6` | `GPIO8` | 20 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD D7 | `PIN_EPD_DB7` | `GPIO10` | 21 | 原理图 U9 / J6 | 8bit 并口数据 |
| EPD XSTL | `PIN_EPD_XSTL` | `GPIO13` | 13 | 原理图 U9 / J6 | 板级网络 `EPD_XSTL` |
| EPD 数字电源 | `PIN_EPD_PWR` | `GPIO45` | 7 / 12 / 23 / 38 | 原理图 U9 / J6 | 同一网络在面板连接器上多点分配 |
| EPD 升压使能 | `PIN_BST_EN` | `GPIO46` | - | 原理图 U9 / 电源电路 | 控制 `MT3608` 升压链路 |

### 2.2 EPD 供电相关网络

| 功能 | 建议名 | 网络 / 引脚 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| 面板正压 | `PIN_EPD_VPOS` | `EPD_VPOS` / J6-1 | 原理图 升压电路 / J6 | 由板上电源电路产生 |
| 面板负压 | `PIN_EPD_VNEG` | `EPD_VNEG` / J6-2 | 原理图 反相电路 / J6 | 由板上电源电路产生 |
| 面板 VGL | `PIN_EPD_VGL` | `EPD_VGL` / J6-31 | 原理图 / J6 | 板上高压网络 |
| 面板 VGH | `PIN_EPD_VGH` | `EPD_VGH` / J6-25 | 原理图 / J6 | 板上高压网络 |
| 面板 VCOM | `PIN_EPD_VCOM` | `VCOM` / J6-24 | 原理图 / J6 | 由 `EPD_VNEG` 通过 `R33/R36/C31` 生成，未见独立 GPIO 控制 |

> 说明：
> J6 上还包含多路 GND 引脚；当前原理图里 `EPD_PWR` 之外未看到额外的 EPD PMIC/IO 扩展控制脚。

## 3. XL9555 扩展 IO 与板载按键

`XL9555/QF24` 通过主 I2C 总线连接；`INT# -> EX_IRQ -> GPIO1`。  
地址引脚 `A0/A1/A2` 全接地，按器件地址编码规则可推断 I2C 地址为 `0x20`。

| XL9555 | 建议名 | 板级网络 | 方向 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- |
| P00 | `PIN_IOE_BTN0` | `BTN0` | 输入 | 原理图 U7 | 对应 KEY1 |
| P01 | `PIN_IOE_BTN1` | `BTN1` | 输入 | 原理图 U7 | 对应 KEY2 |
| P02 | `PIN_IOE_BTN2` | `BTN2` | 输入 | 原理图 U7 | 对应 KEY3 |
| P03 | `PIN_IOE_BTN3` | `BTN3` | 输入 | 原理图 U7 | 对应 KEY4 |
| P04 | `PIN_IOE_BTN4` | `BTN4` | 输入 | 原理图 U7 | 对应 KEY5 |
| P05 | `PIN_IOE_CARD_DET` | `CARD_DET` | 输入 | 原理图 U7 / J2 | TF 卡检测脚 |

> 说明：
> `P06`、`P07`、`P10` 到 `P17` 在当前原理图中未见连接。

## 4. TF 卡与扩展接口

### 4.1 TF 卡 `J2`

| 功能 | 建议名 | GPIO / 映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| TF SPI MISO | `PIN_SD_MISO` | `GPIO40` (`CARD_MISO`) | 原理图 U9 / J2 | 接 J2 `DAT0/MISO` |
| TF SPI SCK | `PIN_SD_SCK` | `GPIO39` (`CARD_SCK`) | 原理图 U9 / J2 | 接 J2 `CLK/SCK` |
| TF SPI MOSI | `PIN_SD_MOSI` | `GPIO38` (`CARD_MOSI`) | 原理图 U9 / J2 | 接 J2 `CMD/MOSI` |
| TF SPI CS | `PIN_SD_CS` | `GPIO47` (`CARD_CS`) | 原理图 U9 / J2 | 接 J2 `DAT1/CS` |
| TF 检测 | `PIN_IOE_CARD_DET` | `XL9555 P05` | 原理图 U7 / J2 | J2 `DETECT` |
| TF 电源 | `PIN_SD_VDD` | `SOC_VDD` | 原理图 J2 | SPI 模式下 `DAT1/DAT2` 未使用 |

### 4.2 扩展排针 / 小接口

| 接口 | 板上网络 | 来源 | 备注 |
| --- | --- | --- | --- |
| `J4` 9Pin | `SDA`, `SCL`, `GPIO37`, `GPIO36`, `GPIO35`, `GPIO34`, `GPIO33`, `SOC_VDD`, `GND` | 原理图 J4 | 接口丝印为 `SDA/SCL/MCLK/SCLK/DOUT/LRCK/DIN/5V/GND`，其中 `5V` 实际接 `SOC_VDD` |
| `J5` 9Pin | `CARD_MISO`, `CARD_SCK`, `CARD_MOSI`, `GPIO48`, `GPIO42`, `GPIO41`, `GPIO21`, `SOC_VDD`, `GND` | 原理图 J5 | 接口丝印同样使用 `SDA/SCL/MCLK/SCLK/DOUT/LRCK/DIN/5V/GND` 命名；`R41` 标注 `NC`，默认不把 `EPD_XLE` 接入该支路 |
| `J7` 4Pin | `SOC_VDD`, `GND`, `SCL`, `SDA` | 原理图 J7 | 丝印 `VIN/GND/SCL/SDA`，其中 `VIN` 实际为 `SOC_VDD` |
| `J8` 6Pin | `NC`, `NC`, `SCL`, `SDA`, `GND`, `SOC_VDD` | 原理图 J8 | `INT`、`COT` 两脚未接；其余为 I2C 预留接口 |
| `P2` 2x3 | `U0RXD`, `U0TXD`, `SOC_VDD`, `GND` | 原理图 P2 | UART0 小排针 |
| `P3` 2x3 | `SCL`, `SDA`, `SOC_VDD`, `GND` | 原理图 P3 | I2C 小排针 |

## 5. I2C 总线器件

| 设备 | 地址 | 来源 | 备注 |
| --- | --- | --- | --- |
| `AXP2602` | 原理图未标注 | 原理图 U1 | 使用主 I2C，总线中断接 `GPIO21 / Guage_IRQ` |
| `XL9555/QF24` | 推断 `0x20` | 原理图 U7 | `A0/A1/A2` 全接地，`INT# -> GPIO1 / EX_IRQ` |
| 外部 I2C 设备 | - | 原理图 J4 / J7 / J8 / P3 | 与板上器件共用 `GPIO2 / GPIO3` |

## 6. 使用提示

- `GPIO1 / EX_IRQ` 同时连接 `XL9555 INT#` 和 KEY7，软件里最好把“按键事件”和“IO 扩展中断事件”一起处理。
- `J4`、`J5`、`J7`、`J8`、`P2`、`P3` 上的电源脚都接 `SOC_VDD`，它是板上的 3.3V 主电源，不是 USB 5V。
- `EPD_XLE`、`EPD_XCL` 复用了 `XTAL_32K_P/N`；如果驱动层假定它们是普通编号 GPIO，需要单独适配。
- `R41` 在原理图中标注 `NC`，默认不会把 `EPD_XLE` 接到 J5 的 `DIN/GPIO21` 支路。
- 若仓库中其他说明文档与本页不一致，应以 `hardware/T5 e-Paper Basic.pdf` 为准。
