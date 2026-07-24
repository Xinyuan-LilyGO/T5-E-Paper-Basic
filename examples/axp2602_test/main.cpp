#include <Arduino.h>
#include <Wire.h>

#include <algorithm>
#include <cmath>

#include <GaugeAXP2602.hpp>
#include <M5GFX.h>
#include <lgfx/v1/platforms/esp32/Bus_EPD.h>
#include <lgfx/v1/platforms/esp32/Panel_EPD.hpp>

namespace board
{
constexpr int kPinI2cSda = 3;
constexpr int kPinI2cScl = 2;
constexpr int kPinGaugeIrq = 21;
constexpr int kPinChargeStatus = 4;
constexpr int kPinUsbDetect = 5;
constexpr int kPinBoot = 0;

constexpr int kPinEpdDb0 = 6;
constexpr int kPinEpdDb1 = 14;
constexpr int kPinEpdDb2 = 7;
constexpr int kPinEpdDb3 = 12;
constexpr int kPinEpdDb4 = 9;
constexpr int kPinEpdDb5 = 11;
constexpr int kPinEpdDb6 = 8;
constexpr int kPinEpdDb7 = 10;
constexpr int kPinEpdXstl = 13;
constexpr int kPinEpdSpv = 17;
constexpr int kPinEpdCkv = 18;
constexpr int kPinEpdPwr = 45;
constexpr int kPinBstEn = 46;
constexpr int kPinEpdXle = 15;
constexpr int kPinEpdXcl = 16;

constexpr int kPanelWidth = 960;
constexpr int kPanelHeight = 540;
constexpr uint8_t kPanelOffsetRotation = 3;
}  // namespace board

class LilyGoEPaperBasicDisplay : public lgfx::LGFX_Device
{
 public:
  LilyGoEPaperBasicDisplay()
  {
    auto bus_cfg = bus_.config();
    bus_cfg.bus_speed = 16000000;
    bus_cfg.pin_data[0] = board::kPinEpdDb0;
    bus_cfg.pin_data[1] = board::kPinEpdDb1;
    bus_cfg.pin_data[2] = board::kPinEpdDb2;
    bus_cfg.pin_data[3] = board::kPinEpdDb3;
    bus_cfg.pin_data[4] = board::kPinEpdDb4;
    bus_cfg.pin_data[5] = board::kPinEpdDb5;
    bus_cfg.pin_data[6] = board::kPinEpdDb6;
    bus_cfg.pin_data[7] = board::kPinEpdDb7;
    bus_cfg.pin_pwr = board::kPinBstEn;
    bus_cfg.pin_spv = board::kPinEpdSpv;
    bus_cfg.pin_ckv = board::kPinEpdCkv;
    bus_cfg.pin_sph = board::kPinEpdXstl;
    bus_cfg.pin_oe = board::kPinEpdPwr;
    bus_cfg.pin_le = board::kPinEpdXle;
    bus_cfg.pin_cl = board::kPinEpdXcl;
    bus_cfg.bus_width = 8;
    bus_.config(bus_cfg);

    panel_.setBus(&bus_);

    auto detail_cfg = panel_.config_detail();
    detail_cfg.line_padding = 8;
    panel_.config_detail(detail_cfg);

    auto panel_cfg = panel_.config();
    panel_cfg.memory_width = board::kPanelWidth;
    panel_cfg.panel_width = board::kPanelWidth;
    panel_cfg.memory_height = board::kPanelHeight;
    panel_cfg.panel_height = board::kPanelHeight;
    panel_cfg.offset_rotation = board::kPanelOffsetRotation;
    panel_cfg.offset_x = 0;
    panel_cfg.offset_y = 0;
    panel_cfg.bus_shared = false;
    panel_.config(panel_cfg);

    setPanel(&panel_);
  }

 private:
  lgfx::Bus_EPD bus_;
  lgfx::Panel_EPD panel_;
};

namespace
{
constexpr uint8_t kAxp2602ChipId = 0x1C;
constexpr uint8_t kRegIrqStatus = 0x20;
constexpr uint32_t kSampleIntervalMs = 1000;
constexpr uint32_t kDisplayIntervalMs = 5000;
constexpr uint32_t kGaugeRetryIntervalMs = 5000;
constexpr uint32_t kButtonDebounceMs = 20;
constexpr uint32_t kQualityRefreshInterval = 12;
constexpr float kIdleCurrentThresholdMa = 1.0f;

constexpr uint8_t kIrqLowBattery = 0x01;
constexpr uint8_t kIrqSocUpdate = 0x02;
constexpr uint8_t kIrqOverTemperature = 0x04;
constexpr uint8_t kIrqWatchdog = 0x08;

LilyGoEPaperBasicDisplay display;
GaugeAXP2602 gauge;

volatile bool gauge_irq_pending = false;
bool display_ready = false;
bool gauge_ready = false;
bool request_display_refresh = false;
uint32_t last_sample_ms = 0;
uint32_t last_display_ms = 0;
uint32_t last_gauge_retry_ms = 0;
uint32_t display_refresh_count = 0;

struct GaugeSnapshot {
  bool valid = false;
  bool sleeping = false;
  bool usb_present = false;
  bool charge_status_low = false;
  bool voltage_measurement = false;
  bool current_measurement = false;
  bool die_measurement = false;
  int chip_id = -1;
  uint16_t battery_mv = 0;
  float current_ma = 0.0f;
  int16_t current_raw = 0;
  uint8_t soc = 0;
  uint8_t soh = 0;
  int8_t battery_temperature_c = 0;
  float die_temperature_c = 0.0f;
  uint16_t time_to_empty_min = 0;
  uint16_t time_to_full_min = 0;
  float absolute_power_w = 0.0f;
  uint8_t low_soc_threshold = 0;
  uint8_t over_temperature_threshold = 0;
  uint8_t last_irq_raw = 0;
  uint32_t irq_count = 0;
  uint32_t sample_count = 0;
  uint32_t error_count = 0;
  uint32_t last_poll_ms = 0;
  GaugeAXP2602::OperatingMode operating_mode = GaugeAXP2602::OPERATING_MODE_NORMAL;
  GaugeAXP2602::CurrentSenseResistor sense_resistor =
      GaugeAXP2602::SENSE_RESISTOR_10_MOHM;
};

GaugeSnapshot data;

void ARDUINO_ISR_ATTR onGaugeInterrupt()
{
  gauge_irq_pending = true;
}

void setUiFont(uint8_t scale = 1, bool bold = false)
{
  display.setFont(bold ? &fonts::efontCN_16_b : &fonts::efontCN_16);
  display.setTextSize(scale);
}

String stateText()
{
  if (!gauge_ready || !data.valid) return "NO GAUGE";
  if (data.sleeping) return "GAUGE SLEEP";
  if (data.current_ma > kIdleCurrentThresholdMa) return "CHARGING";
  if (data.current_ma < -kIdleCurrentThresholdMa) return "DISCHARGING";
  return "IDLE";
}

String signedCurrent(float current_ma)
{
  String result;
  if (current_ma > 0.0f) result += "+";
  result += String(current_ma, 1);
  result += " mA";
  return result;
}

String positiveCurrent(float current_ma)
{
  return String(std::max(0.0f, current_ma), 1) + " mA";
}

String formatTime(uint16_t minutes)
{
  if (minutes == 0xFFFF) return "--";
  if (minutes >= 60) {
    return String(minutes / 60) + " h " + String(minutes % 60) + " min";
  }
  return String(minutes) + " min";
}

String operatingModeText()
{
  switch (data.operating_mode) {
    case GaugeAXP2602::OPERATING_MODE_HIGH_PRECISION: return "HIGH PRECISION";
    case GaugeAXP2602::OPERATING_MODE_LOW_POWER: return "LOW POWER";
    case GaugeAXP2602::OPERATING_MODE_NORMAL:
    default: return "NORMAL";
  }
}

String senseResistorText()
{
  switch (data.sense_resistor) {
    case GaugeAXP2602::SENSE_RESISTOR_5_MOHM: return "5 mOhm";
    case GaugeAXP2602::SENSE_RESISTOR_20_MOHM: return "20 mOhm";
    case GaugeAXP2602::SENSE_RESISTOR_40_MOHM: return "40 mOhm";
    case GaugeAXP2602::SENSE_RESISTOR_10_MOHM:
    default: return "10 mOhm";
  }
}

String irqText(uint8_t status)
{
  if (status == 0) return "NONE";
  String result;
  if (status & kIrqLowBattery) result += "LOW_BAT ";
  if (status & kIrqSocUpdate) result += "SOC ";
  if (status & kIrqOverTemperature) result += "OVER_TEMP ";
  if (status & kIrqWatchdog) result += "WDT ";
  result.trim();
  return result;
}

String chargerStatusText()
{
  if (!data.usb_present) return "NO USB";
  return data.charge_status_low ? "CHARGING (LOW)" : "DONE/IDLE (HIGH)";
}

void drawCard(int x, int y, int w, int h, const char* title, const char* subtitle)
{
  display.fillRoundRect(x, y, w, h, 12, TFT_WHITE);
  display.drawRoundRect(x, y, w, h, 12, TFT_BLACK);
  display.drawFastHLine(x + 12, y + 38, w - 24, TFT_BLACK);
  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1, true);
  display.drawString(title, x + 14, y + 10);
  display.setFont(&fonts::Font2);
  display.setTextDatum(textdatum_t::top_right);
  display.drawString(subtitle, x + w - 14, y + 13);
}

void drawMetric(int x, int y, const String& label, const String& value,
                const String& note = String())
{
  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1);
  display.drawString(label, x, y);
  display.setFont(&fonts::Font2);
  display.drawString(value, x, y + 22);
  if (note.length()) {
    display.setFont(&fonts::Font0);
    display.drawString(note, x, y + 42);
  }
}

void drawBatteryIcon(int x, int y, int w, int h, uint8_t soc)
{
  display.fillRect(x, y, w, h, TFT_WHITE);
  display.drawRoundRect(x, y, w, h, 7, TFT_BLACK);
  display.fillRect(x + w, y + h / 3, 7, h / 3, TFT_BLACK);
  const int inner_w = w - 10;
  const int fill_w = inner_w * std::min<uint8_t>(soc, 100) / 100;
  if (fill_w > 0) {
    display.fillRect(x + 5, y + 5, fill_w, h - 10, TFT_BLACK);
  }
}

void drawHeader()
{
  display.fillRect(0, 0, display.width(), 84, TFT_BLACK);
  display.setTextDatum(textdatum_t::top_center);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  setUiFont(2, true);
  display.drawString("AXP2602 充放电测试", display.width() / 2, 7);
  display.setFont(&fonts::Font2);
  display.drawString("BATTERY FUEL GAUGE  |  I2C 0x62  |  SDA3 / SCL2 / IRQ21",
                     display.width() / 2, 58);
}

void drawSummary()
{
  drawCard(18, 98, 504, 154, "实时状态", "LIVE STATUS");

  if (!gauge_ready || !data.valid) {
    display.setTextDatum(textdatum_t::middle_center);
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    setUiFont(2, true);
    display.drawString("未发现 AXP2602", display.width() / 2, 165);
    setUiFont(1);
    display.drawString("检查 I2C 地址 0x62、GPIO2 / GPIO3 和电源",
                       display.width() / 2, 214);
    return;
  }

  drawBatteryIcon(38, 139, 88, 58, data.soc);

  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setFont(&fonts::Font4);
  display.drawString(stateText(), 151, 126);
  display.setFont(&fonts::Font2);
  display.drawString(String(data.battery_mv / 1000.0f, 3) + " V", 151, 174);
  display.drawString(signedCurrent(data.current_ma), 151, 205);

  display.setTextDatum(textdatum_t::top_right);
  display.setFont(&fonts::Font7);
  display.drawString(String(std::min<uint8_t>(data.soc, 100)), 475, 128);
  display.setFont(&fonts::Font4);
  display.drawString("%", 500, 189);
}

void drawElectricalCard()
{
  drawCard(18, 268, 504, 238, "电气参数", "VOLTAGE / CURRENT / POWER");

  if (!gauge_ready || !data.valid) return;

  const float charge_current = std::max(0.0f, data.current_ma);
  const float discharge_current = std::max(0.0f, -data.current_ma);

  drawMetric(36, 314, "电池电压 VBAT",
             String(data.battery_mv) + " mV",
             String(data.battery_mv / 1000.0f, 3) + " V");
  drawMetric(280, 314, "双向电池电流",
             signedCurrent(data.current_ma),
             "SensorLib: +充电 / -放电");
  drawMetric(36, 372, "充电电流",
             positiveCurrent(charge_current));
  drawMetric(280, 372, "放电电流",
             positiveCurrent(discharge_current));
  drawMetric(36, 430, "瞬时电池功率",
             String(data.absolute_power_w, 3) + " W");
  drawMetric(280, 430, "充电输入电压",
             "--  不支持",
             "AXP2602 无 VBUS ADC");

  display.setTextDatum(textdatum_t::top_left);
  setUiFont(1);
  display.drawString(String("USB 输入：")
                         + (data.usb_present ? "有" : "无")
                         + "  GPIO5="
                         + (data.usb_present ? "HIGH" : "LOW"),
                     36, 484);
  display.drawString(String("充电状态：") + chargerStatusText()
                         + "  GPIO4="
                         + (data.charge_status_low ? "LOW" : "HIGH"),
                     280, 484);
}

void drawBatteryCard()
{
  drawCard(18, 520, 504, 182, "电池估算与温度", "SOC / SOH / TIME / TEMP");

  if (!gauge_ready || !data.valid) return;

  drawMetric(36, 566, "电量 SOC", String(data.soc) + " %");
  drawMetric(198, 566, "健康度 SOH", String(data.soh) + " %");
  drawMetric(360, 566, "电流原始值", String(data.current_raw));

  drawMetric(36, 620, "预计放空 TTE", formatTime(data.time_to_empty_min));
  drawMetric(198, 620, "预计充满 TTF", formatTime(data.time_to_full_min));
  drawMetric(360, 620, "温度结果",
             String(data.battery_temperature_c) + " C");

  display.setTextDatum(textdatum_t::top_left);
  setUiFont(1);
  display.drawString(String("芯片温度：") + String(data.die_temperature_c, 1)
                         + " C    低电量阈值：" + data.low_soc_threshold
                         + "%    过温阈值：" + data.over_temperature_threshold + " C",
                     36, 681);
}

void drawDiagnosticsCard()
{
  drawCard(18, 716, 504, 188, "诊断与控制", "DEVICE / IRQ / SLEEP");

  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1);

  if (!gauge_ready || !data.valid) {
    display.drawString("AXP2602 初始化失败，将每 5 秒自动重试", 36, 766);
    display.drawString(String("I2C：SDA GPIO") + board::kPinI2cSda
                           + " / SCL GPIO" + board::kPinI2cScl,
                       36, 802);
    display.drawString("整机开关机：AXP2602 不支持", 36, 838);
    return;
  }

  display.drawString(String("芯片 ID：0x") + String(data.chip_id, HEX)
                         + "    工作模式：" + operatingModeText()
                         + "    采样电阻：" + senseResistorText(),
                     36, 762);
  display.drawString(String("测量使能：VBAT=") + (data.voltage_measurement ? "ON" : "OFF")
                         + "  IBAT=" + (data.current_measurement ? "ON" : "OFF")
                         + "  TDIE=" + (data.die_measurement ? "ON" : "OFF"),
                     36, 793);
  display.drawString(String("IRQ：GPIO21=")
                         + (digitalRead(board::kPinGaugeIrq) ? "HIGH" : "LOW")
                         + "  RAW=0x" + String(data.last_irq_raw, HEX)
                         + "  " + irqText(data.last_irq_raw)
                         + "  COUNT=" + data.irq_count,
                     36, 824);
  display.drawString(String("电量计：") + (data.sleeping ? "SLEEP" : "AWAKE")
                         + "    采样=" + data.sample_count
                         + "    错误=" + data.error_count
                         + "    轮询=" + data.last_poll_ms + " ms",
                     36, 855);
  display.drawString("整机开关机：不支持；BOOT 仅切换电量计休眠/唤醒",
                     36, 886);
}

void drawFooter()
{
  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1);
  display.drawString("BOOT：AXP2602 休眠/唤醒（不会关闭整机）",
                     display.width() / 2, 925);
  display.setFont(&fonts::Font2);
  display.drawString("SERIAL: S=SLEEP  W=WAKE  R=RESET  H=HELP  |  VALUES UPDATE EVERY 5s",
                     display.width() / 2, 950);
}

void drawScreen()
{
  display.fillScreen(TFT_WHITE);
  drawHeader();
  drawSummary();
  drawElectricalCard();
  drawBatteryCard();
  drawDiagnosticsCard();
  drawFooter();
}

void refreshDisplay(bool quality)
{
  if (!display_ready) return;
  drawScreen();
  display.setEpdMode(quality ? lgfx::epd_quality : lgfx::epd_fast);
  display.display();
  display.waitDisplay();
  ++display_refresh_count;
  last_display_ms = millis();
  request_display_refresh = false;
}

void printHelp()
{
  Serial.println("\nAXP2602 commands:");
  Serial.println("  S - put the fuel gauge into sleep mode");
  Serial.println("  W - wake the fuel gauge");
  Serial.println("  R - reset and reconfigure the fuel gauge");
  Serial.println("  H - print this help");
  Serial.println("Note: gauge sleep does NOT turn off the ESP32 or system power.\n");
}

void configureGauge()
{
  gauge.wakeup();
  gauge.setOperatingMode(GaugeAXP2602::OPERATING_MODE_NORMAL);
  gauge.setCurrentSenseResistor(GaugeAXP2602::SENSE_RESISTOR_10_MOHM);
  gauge.setBatteryDetection(true);
  gauge.setCurrentMeasurement(true);
  gauge.setThermalDieMeasurement(true);

  gauge.disableAllIRQ();
  gauge.clearIRQStatus();
  gauge.enableIRQ(GaugeAXP2602::IRQ_ENABLE_LOW_BATTERY);
  gauge.enableIRQ(GaugeAXP2602::IRQ_ENABLE_SOC_UPDATE);
  gauge.enableIRQ(GaugeAXP2602::IRQ_ENABLE_OVER_TEMPERATURE);
  gauge.enableIRQ(GaugeAXP2602::IRQ_ENABLE_WDT_TIMEOUT);
}

bool initializeGauge()
{
  if (!gauge.begin(Wire, board::kPinI2cSda, board::kPinI2cScl)) {
    gauge_ready = false;
    data.valid = false;
    ++data.error_count;
    Serial.println("[AXP2602] init failed: check I2C address 0x62 and wiring");
    return false;
  }

  Wire.setClock(400000);
  configureGauge();
  gauge_ready = true;
  Serial.printf("[AXP2602] ready: ID=0x%02X, SDA=%d, SCL=%d, IRQ=%d\n",
                gauge.getChipID(), board::kPinI2cSda,
                board::kPinI2cScl, board::kPinGaugeIrq);
  return true;
}

void captureIrqStatus()
{
  const int irq_status = gauge.readReg(kRegIrqStatus);
  if (irq_status < 0) return;

  const uint8_t raw = static_cast<uint8_t>(irq_status) & 0x0F;
  if (raw == 0) return;

  data.last_irq_raw = raw;
  ++data.irq_count;
  request_display_refresh = true;
  Serial.printf("[AXP2602] IRQ raw=0x%02X: %s\n", raw, irqText(raw).c_str());
  gauge.clearIRQStatus();
}

bool sampleGauge()
{
  if (!gauge_ready) return false;

  const uint32_t start_ms = millis();
  if (!gauge.refresh()) {
    ++data.error_count;
    data.valid = false;
    Serial.println("[AXP2602] refresh failed");
    return false;
  }

  data.chip_id = gauge.getChipID();
  data.sleeping = gauge.isSleepModeEnabled();
  data.battery_mv = gauge.getVoltage();
  data.current_ma = gauge.getCurrent();
  data.current_raw = gauge.getCurrentRaw();
  data.soc = gauge.getStateOfCharge();
  data.soh = gauge.getBatteryStatus();
  data.battery_temperature_c = gauge.getTemperature();
  data.die_temperature_c = gauge.getThermalDieTemperature();
  data.time_to_empty_min = gauge.getTimeToEmpty();
  data.time_to_full_min = gauge.getTimeToFull();
  data.absolute_power_w = gauge.getAbsolutePower();
  data.low_soc_threshold = gauge.getLowBatterySOCThreshold();
  data.over_temperature_threshold = gauge.getOverTemperatureThreshold();
  data.operating_mode = gauge.getOperatingMode();
  data.sense_resistor = gauge.getCurrentSenseResistor();
  data.voltage_measurement = gauge.isBatteryVoltageMeasurementEnabled();
  data.current_measurement = gauge.isCurrentMeasurementEnabled();
  data.die_measurement = gauge.isThermalDieMeasurementEnabled();
  data.usb_present = digitalRead(board::kPinUsbDetect) == HIGH;
  data.charge_status_low = digitalRead(board::kPinChargeStatus) == LOW;
  data.last_poll_ms = millis() - start_ms;
  ++data.sample_count;
  data.valid = data.chip_id == kAxp2602ChipId;

  noInterrupts();
  const bool irq_was_pending = gauge_irq_pending;
  gauge_irq_pending = false;
  interrupts();
  if (irq_was_pending || digitalRead(board::kPinGaugeIrq) == LOW) {
    captureIrqStatus();
  }

  Serial.printf(
      "[AXP2602] state=%s VBAT=%u mV IBAT=%+.1f mA P=%.3f W "
      "SOC=%u%% SOH=%u%% TEMP=%d C TDIE=%.1f C TTE=%u min TTF=%u min "
      "USB=%s CHG_STAT=%s\n",
      stateText().c_str(), data.battery_mv, data.current_ma, data.absolute_power_w,
      data.soc, data.soh, data.battery_temperature_c, data.die_temperature_c,
      data.time_to_empty_min, data.time_to_full_min,
      data.usb_present ? "YES" : "NO",
      data.charge_status_low ? "LOW" : "HIGH");
  return data.valid;
}

void setGaugeSleep(bool sleep)
{
  if (!gauge_ready) return;

  if (sleep) {
    gauge.sleep();
    Serial.println("[AXP2602] fuel gauge entered sleep; system power remains on");
  } else {
    gauge.wakeup();
    delay(20);
    gauge.setOperatingMode(GaugeAXP2602::OPERATING_MODE_NORMAL);
    Serial.println("[AXP2602] fuel gauge woke up");
  }
  data.sleeping = gauge.isSleepModeEnabled();
  request_display_refresh = true;
}

void resetGauge()
{
  if (!gauge_ready) return;
  gauge.reset();
  configureGauge();
  Serial.println("[AXP2602] reset and reconfigured");
  sampleGauge();
  request_display_refresh = true;
}

void handleSerial()
{
  while (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    switch (command) {
      case 's':
      case 'S':
        setGaugeSleep(true);
        break;
      case 'w':
      case 'W':
        setGaugeSleep(false);
        break;
      case 'r':
      case 'R':
        resetGauge();
        break;
      case 'h':
      case 'H':
        printHelp();
        break;
      default:
        break;
    }
  }
}

void handleBootButton()
{
  static bool raw_pressed = false;
  static bool stable_pressed = false;
  static uint32_t changed_ms = 0;

  const bool pressed = digitalRead(board::kPinBoot) == LOW;
  if (pressed != raw_pressed) {
    raw_pressed = pressed;
    changed_ms = millis();
    return;
  }
  if (pressed == stable_pressed || millis() - changed_ms < kButtonDebounceMs) {
    return;
  }

  stable_pressed = pressed;
  if (stable_pressed && gauge_ready) {
    setGaugeSleep(!gauge.isSleepModeEnabled());
  }
}
}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);

#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
  Serial.enableReboot(false);
#endif

  Serial.println("\n========== AXP2602 CHARGE / DISCHARGE TEST ==========");
  Serial.println("AXP2602 is a fuel gauge, not the LGS4056 charger.");
  Serial.println("It has no VBUS ADC and cannot turn off system power.");

  pinMode(board::kPinGaugeIrq, INPUT_PULLUP);
  pinMode(board::kPinChargeStatus, INPUT_PULLUP);
  pinMode(board::kPinUsbDetect, INPUT);
  pinMode(board::kPinBoot, INPUT_PULLUP);

  if (!display.init()) {
    Serial.println("[EPD] init failed");
    while (true) delay(1000);
  }
  display.setRotation(0);
  display.setColorDepth(4);
  display.setAutoDisplay(false);
  display_ready = true;

  initializeGauge();
  if (gauge_ready) {
    attachInterrupt(digitalPinToInterrupt(board::kPinGaugeIrq),
                    onGaugeInterrupt, FALLING);
    sampleGauge();
  }

  refreshDisplay(true);
  printHelp();
  last_sample_ms = millis();
  last_gauge_retry_ms = millis();
}

void loop()
{
  handleSerial();
  handleBootButton();

  const uint32_t now = millis();
  if (!gauge_ready && now - last_gauge_retry_ms >= kGaugeRetryIntervalMs) {
    last_gauge_retry_ms = now;
    if (initializeGauge()) {
      attachInterrupt(digitalPinToInterrupt(board::kPinGaugeIrq),
                      onGaugeInterrupt, FALLING);
      sampleGauge();
    }
    request_display_refresh = true;
  }

  if (gauge_ready && now - last_sample_ms >= kSampleIntervalMs) {
    last_sample_ms = now;
    const bool old_usb = data.usb_present;
    const bool old_charge_status = data.charge_status_low;
    const bool old_sleep = data.sleeping;
    const int old_direction =
        (data.current_ma > kIdleCurrentThresholdMa)
            ? 1
            : ((data.current_ma < -kIdleCurrentThresholdMa) ? -1 : 0);

    sampleGauge();

    const int new_direction =
        (data.current_ma > kIdleCurrentThresholdMa)
            ? 1
            : ((data.current_ma < -kIdleCurrentThresholdMa) ? -1 : 0);
    if (old_usb != data.usb_present
        || old_charge_status != data.charge_status_low
        || old_sleep != data.sleeping
        || old_direction != new_direction) {
      request_display_refresh = true;
    }
  }

  if (request_display_refresh || now - last_display_ms >= kDisplayIntervalMs) {
    const bool quality =
        display_refresh_count != 0
        && (display_refresh_count % kQualityRefreshInterval) == 0;
    refreshDisplay(quality);
  }

  delay(10);
}
