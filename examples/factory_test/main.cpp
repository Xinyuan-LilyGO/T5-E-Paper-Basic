#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#include <IoExpanderXL9555.hpp>
#include <M5GFX.h>
#include <lgfx/v1/platforms/esp32/Bus_EPD.h>
#include <lgfx/v1/platforms/esp32/Panel_EPD.hpp>

namespace board
{
constexpr int kPinI2cSda = 3;
constexpr int kPinI2cScl = 2;
constexpr int kPinExtIrq = 1;  // XL9555 active-low interrupt.
constexpr int kPinBoot = 0;    // BOOT button, active low.

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

constexpr int kPinSdMosi = 38;
constexpr int kPinSdSck = 39;
constexpr int kPinSdMiso = 40;
constexpr int kPinSdCs = 47;
constexpr uint32_t kSdSpiClock = 16000000;

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
constexpr char kWifiSsid[] = "LilyGo-AABB";
constexpr char kWifiPassword[] = "xinyuandianzi";
constexpr char kWifiSsid2[] = "xinyuandianzi";
constexpr char kWifiPassword2[] = "AA15994823428";

constexpr uint32_t kSplashHoldMs = 3000;
constexpr uint32_t kDebounceMs = 12;
constexpr uint32_t kWifiConnectTimeoutMs = 12000;
constexpr uint8_t kCardDetectPin = 5;
constexpr uint16_t kButtonMask = 0x001F;
constexpr uint16_t kCardMask = 1U << kCardDetectPin;
constexpr uint16_t kUsedInputMask = kButtonMask | kCardMask;
constexpr size_t kButtonCount = 6;  // BTN0..BTN4 plus the direct BOOT button.
constexpr size_t kMaxWifiNetworks = 12;

constexpr int kCenterX = 56;
constexpr int kCenterW = 428;
constexpr int kHeaderY = 18;
constexpr int kHeaderH = 154;
constexpr int kSdY = 194;
constexpr int kSdH = 252;
constexpr int kWifiY = 468;
constexpr int kWifiH = 416;

constexpr char kSdTestPath[] = "/.lilygo_factory_test.tmp";
constexpr char kSdTestPayload[] = "LILYGO T5 E-Paper Basic factory test\n";

struct DisplayRect {
  int x;
  int y;
  int w;
  int h;
};

struct ButtonVisual {
  int x;
  int y;
  const char* label;
  uint8_t logical_index;
};

// Visual order follows 2.png: BTN2/BTN4, BTN1/BTN3, BTN0/BOOT.
constexpr std::array<ButtonVisual, kButtonCount> kButtonVisuals = {{
    {0, 244, "BTN2", 2},
    {493, 244, "BTN4", 4},
    {0, 356, "BTN1", 1},
    {493, 356, "BTN3", 3},
    {0, 626, "BTN0", 0},
    {493, 626, "BOOT", 5},
}};
constexpr int kButtonW = 47;
constexpr int kButtonH = 58;

struct InputState {
  uint16_t port = kUsedInputMask;
  std::array<bool, kButtonCount> buttons = {};
  bool card_inserted = false;
};

enum class SdState : uint8_t {
  kUnchecked,
  kNoCard,
  kPassed,
  kFailed,
};

struct SdResult {
  SdState state = SdState::kUnchecked;
  String card_type;
  String error;
  uint64_t card_bytes = 0;
  uint64_t total_bytes = 0;
  uint64_t used_bytes = 0;
  uint16_t file_count = 0;
  uint16_t directory_count = 0;
  bool read_write_ok = false;
};

enum class WifiState : uint8_t {
  kScanning,
  kConnecting,
  kConnected,
  kScanOnly,
  kConnectFailed,
  kScanFailed,
};

struct WifiNetwork {
  String ssid;
  int32_t rssi = -127;
  int32_t channel = 0;
  wifi_auth_mode_t auth = WIFI_AUTH_OPEN;
};

struct WifiTarget {
  const char* ssid;
  const char* password;
};

constexpr std::array<WifiTarget, 2> kWifiTargets = {{
    {kWifiSsid, kWifiPassword},
    {kWifiSsid2, kWifiPassword2},
}};

LilyGoEPaperBasicDisplay display;
IoExpanderXL9555 io_expander;

bool io_ready = false;
bool screen_ready = false;
bool sd_bus_ready = false;
InputState stable_input;
InputState sampled_input;
uint32_t sampled_change_ms = 0;
SdResult sd_result;

WifiState wifi_state = WifiState::kScanning;
std::array<WifiNetwork, kMaxWifiNetworks> wifi_networks;
size_t wifi_network_count = 0;
std::array<bool, kWifiTargets.size()> wifi_target_found = {};
size_t next_wifi_target = 0;
int active_wifi_target = -1;
uint32_t wifi_connect_started_ms = 0;
String connected_ssid;
String connected_ip;
int32_t connected_rssi = -127;

uint32_t gray(uint8_t value)
{
  return display.color888(value, value, value);
}

void setUiFont(uint8_t scale = 1, bool bold = false)
{
  display.setFont(bold ? &fonts::efontCN_16_b : &fonts::efontCN_16);
  display.setTextSize(scale);
}

String fitText(String text, int max_width)
{
  if (display.textWidth(text) <= max_width) {
    return text;
  }

  const String suffix = "...";
  while (text.length() > 0 && display.textWidth(text + suffix) > max_width) {
    size_t erase_from = text.length() - 1;
    while (erase_from > 0
           && (static_cast<uint8_t>(text[erase_from]) & 0xC0U) == 0x80U) {
      --erase_from;
    }
    text.remove(erase_from);
  }
  return text + suffix;
}

DisplayRect logicalToNativeRect(int x, int y, int w, int h)
{
  uint_fast16_t xs = static_cast<uint_fast16_t>(x);
  uint_fast16_t ys = static_cast<uint_fast16_t>(y);
  uint_fast16_t xe = static_cast<uint_fast16_t>(x + w - 1);
  uint_fast16_t ye = static_cast<uint_fast16_t>(y + h - 1);

  const uint_fast8_t rotation = display.getRotation() & 7;
  const uint_fast8_t internal_rotation =
      ((rotation + board::kPanelOffsetRotation) & 3)
      | ((rotation & 4) ^ (board::kPanelOffsetRotation & 4));

  if (internal_rotation) {
    if (internal_rotation & 1) {
      std::swap(xs, ys);
      std::swap(xe, ye);
    }
    const uint_fast8_t rotation_mask = 1 << internal_rotation;
    if (rotation_mask & 0b11000110) {
      std::swap(xs, xe);
      xs = board::kPanelWidth - 1 - xs;
      xe = board::kPanelWidth - 1 - xe;
    }
    if (rotation_mask & 0b10011100) {
      std::swap(ys, ye);
      ys = board::kPanelHeight - 1 - ys;
      ye = board::kPanelHeight - 1 - ye;
    }
  }

  return {static_cast<int>(xs), static_cast<int>(ys),
          static_cast<int>(xe - xs + 1), static_cast<int>(ye - ys + 1)};
}

void refreshLogicalRect(int x, int y, int w, int h,
                        lgfx::epd_mode_t mode = lgfx::epd_fast)
{
  if (!screen_ready) {
    return;
  }
  x = std::max(0, x);
  y = std::max(0, y);
  w = std::min(w, display.width() - x);
  h = std::min(h, display.height() - y);
  const DisplayRect rect = logicalToNativeRect(x, y, w, h);
  display.setEpdMode(mode);
  display.display(rect.x, rect.y, rect.w, rect.h);
  display.waitDisplay();
}

void drawCornerMarks()
{
  constexpr int kLength = 22;
  display.drawFastHLine(0, 0, kLength, TFT_BLACK);
  display.drawFastVLine(0, 0, kLength, TFT_BLACK);
  display.drawFastHLine(display.width() - kLength, 0, kLength, TFT_BLACK);
  display.drawFastVLine(display.width() - 1, 0, kLength, TFT_BLACK);
  display.drawFastHLine(0, display.height() - 1, kLength, TFT_BLACK);
  display.drawFastVLine(0, display.height() - kLength, kLength, TFT_BLACK);
  display.drawFastHLine(display.width() - kLength, display.height() - 1, kLength, TFT_BLACK);
  display.drawFastVLine(display.width() - 1, display.height() - kLength, kLength, TFT_BLACK);
}

void drawStartupPattern()
{
  const int width = display.width();
  display.fillScreen(TFT_WHITE);
  drawCornerMarks();

  display.fillRoundRect(18, 18, width - 36, 126, 14, gray(232));
  display.drawRoundRect(18, 18, width - 36, 126, 14, TFT_BLACK);
  display.drawRoundRect(27, 27, width - 54, 108, 11, gray(150));
  display.setTextDatum(textdatum_t::top_center);
  display.setTextColor(TFT_BLACK, gray(232));
  setUiFont(2, true);
  display.drawString("屏幕测试", width / 2, 36);
  setUiFont(1);
  display.drawString("DISPLAY TEST  |  540 x 960  |  16 GRAY", width / 2, 94);

  display.fillRoundRect(18, 160, width - 36, 184, 12, gray(246));
  display.drawRoundRect(18, 160, width - 36, 184, 12, TFT_BLACK);
  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, gray(246));
  setUiFont(1, true);
  display.drawString("灰阶  GRAY SCALE  0—15", 34, 172);
  constexpr int kGrayCols = 4;
  constexpr int kGrayRows = 4;
  constexpr int kGrayGap = 7;
  const int gray_x = 34;
  const int gray_y = 205;
  const int gray_w = width - 68;
  const int cell_w = (gray_w - (kGrayCols - 1) * kGrayGap) / kGrayCols;
  const int cell_h = 28;
  for (int index = 0; index < 16; ++index) {
    const int col = index % kGrayCols;
    const int row = index / kGrayCols;
    const int x = gray_x + col * (cell_w + kGrayGap);
    const int y = gray_y + row * (cell_h + kGrayGap);
    const uint8_t shade = static_cast<uint8_t>(index * 17);
    const uint32_t color = gray(shade);
    display.fillRoundRect(x, y, cell_w, cell_h, 5, color);
    display.drawRoundRect(x, y, cell_w, cell_h, 5, TFT_BLACK);
    display.setFont(&fonts::Font2);
    display.setTextDatum(textdatum_t::middle_center);
    display.setTextColor(index < 8 ? TFT_WHITE : TFT_BLACK, color);
    display.drawString(String(index), x + cell_w / 2, y + cell_h / 2);
  }

  constexpr int kDualY = 362;
  constexpr int kDualH = 212;
  constexpr int kGap = 14;
  const int dual_w = (width - 36 - kGap) / 2;
  display.fillRoundRect(18, kDualY, dual_w, kDualH, 12, gray(246));
  display.drawRoundRect(18, kDualY, dual_w, kDualH, 12, TFT_BLACK);
  display.fillRoundRect(18 + dual_w + kGap, kDualY, dual_w, kDualH, 12, gray(246));
  display.drawRoundRect(18 + dual_w + kGap, kDualY, dual_w, kDualH, 12, TFT_BLACK);

  display.setTextDatum(textdatum_t::top_center);
  display.setTextColor(TFT_BLACK, gray(246));
  setUiFont(1, true);
  display.drawString("棋盘格", 18 + dual_w / 2, kDualY + 12);
  display.drawString("几何图形", 18 + dual_w + kGap + dual_w / 2, kDualY + 12);

  const int checker_x = 18 + (dual_w - 128) / 2;
  const int checker_y = kDualY + 54;
  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      display.fillRect(checker_x + col * 16, checker_y + row * 16, 16, 16,
                       ((row + col) & 1) ? TFT_BLACK : TFT_WHITE);
    }
  }
  display.drawRect(checker_x, checker_y, 128, 128, TFT_BLACK);

  const int shape_x = 18 + dual_w + kGap;
  const int shape_cx = shape_x + dual_w / 2;
  display.drawCircle(shape_cx, kDualY + 98, 43, TFT_BLACK);
  display.fillCircle(shape_cx, kDualY + 98, 19, gray(100));
  display.drawTriangle(shape_x + 30, kDualY + 178, shape_cx, kDualY + 120,
                       shape_x + dual_w - 30, kDualY + 178, TFT_BLACK);
  display.drawRect(shape_x + 49, kDualY + 62, dual_w - 98, 126, TFT_BLACK);

  display.fillRoundRect(18, 592, width - 36, 252, 12, gray(246));
  display.drawRoundRect(18, 592, width - 36, 252, 12, TFT_BLACK);
  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, gray(246));
  setUiFont(1, true);
  display.drawString("线条 / 像素 / 对比度", 34, 604);
  for (int step = 0; step < 16; ++step) {
    const int y = 644 + step * 11;
    display.drawLine(34, y, width - 34, 820 - step * 10, gray(step * 16));
  }
  for (int x = 40; x < width - 40; x += 8) {
    display.drawFastVLine(x, 662, 142, (x & 8) ? TFT_BLACK : gray(190));
  }
  display.fillRect(38, 790, width - 76, 24, TFT_BLACK);
  display.drawFastHLine(38, 821, width - 76, TFT_BLACK);

  display.fillRoundRect(18, 862, width - 36, 80, 12, TFT_BLACK);
  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  setUiFont(1, true);
  display.drawString("目视检查屏幕完整性 · 3 秒后进入出厂测试", width / 2, 890);
  display.setFont(&fonts::Font2);
  display.drawString("LILYGO  T5 E-PAPER BASIC", width / 2, 920);
}

void drawSectionFrame(int y, int h, const char* title, const char* english)
{
  display.fillRoundRect(kCenterX, y, kCenterW, h, 14, TFT_WHITE);
  display.drawRoundRect(kCenterX, y, kCenterW, h, 14, TFT_BLACK);
  display.drawFastHLine(kCenterX + 14, y + 50, kCenterW - 28, TFT_BLACK);
  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1, true);
  display.drawString(title, kCenterX + 18, y + 14);
  display.setFont(&fonts::Font2);
  display.setTextDatum(textdatum_t::top_left);
  display.drawString(english, kCenterX + 154, y + 18);
}

void drawStatusBadge(int x, int y, int w, const String& text, bool dark,
                     bool ascii_text = false)
{
  const uint32_t background = dark ? TFT_BLACK : TFT_WHITE;
  const uint32_t foreground = dark ? TFT_WHITE : TFT_BLACK;
  display.fillRoundRect(x, y, w, 30, 15, background);
  display.drawRoundRect(x, y, w, 30, 15, TFT_BLACK);
  display.drawRoundRect(x + 1, y + 1, w - 2, 28, 14, TFT_BLACK);
  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(foreground, background);
  if (ascii_text) {
    display.setFont(&fonts::Font2);
  } else {
    setUiFont(1, true);
  }
  display.drawString(text, x + w / 2, y + 15);
}

void drawMainHeader()
{
  display.fillRoundRect(kCenterX, kHeaderY, kCenterW, kHeaderH, 16, gray(242));
  display.drawRoundRect(kCenterX, kHeaderY, kCenterW, kHeaderH, 16, TFT_BLACK);
  display.fillRoundRect(kCenterX, kHeaderY, kCenterW, 70, 16, TFT_BLACK);
  display.fillRect(kCenterX, kHeaderY + 54, kCenterW, 16, TFT_BLACK);

  display.setTextDatum(textdatum_t::top_center);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  setUiFont(2, true);
  display.drawString("出厂测试", kCenterX + kCenterW / 2, kHeaderY + 9);
  display.setFont(&fonts::Font2);
  display.setTextDatum(textdatum_t::top_left);
  display.drawString("T5 E-PAPER BASIC", kCenterX + 18, kHeaderY + 49);
  display.setTextDatum(textdatum_t::top_right);
  display.drawString("FACTORY DIAGNOSTICS", kCenterX + kCenterW - 18, kHeaderY + 49);

  display.setTextColor(TFT_BLACK, gray(242));
  setUiFont(1);
  display.setTextDatum(textdatum_t::top_left);
  display.drawString("屏幕", kCenterX + 18, kHeaderY + 86);
  display.drawString("按键 / IO", kCenterX + 146, kHeaderY + 86);
  display.drawString("分辨率", kCenterX + 292, kHeaderY + 86);
  display.setFont(&fonts::Font2);
  display.drawString("PASS", kCenterX + 18, kHeaderY + 118);
  display.drawString(io_ready ? "PASS" : "CHECK", kCenterX + 146, kHeaderY + 118);
  display.drawString("540 x 960", kCenterX + 292, kHeaderY + 118);
}

const char* sdCardTypeName(uint8_t type)
{
  switch (type) {
    case CARD_MMC: return "MMC";
    case CARD_SD: return "SDSC";
    case CARD_SDHC: return "SDHC / SDXC";
    default: return "UNKNOWN";
  }
}

String formatCapacity(uint64_t bytes)
{
  if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
    return String(static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
  }
  return String(static_cast<unsigned long long>(bytes / (1024ULL * 1024ULL))) + " MB";
}

void drawSdIcon(int x, int y, bool active)
{
  const uint32_t fill = active ? TFT_BLACK : TFT_WHITE;
  const uint32_t text = active ? TFT_WHITE : TFT_BLACK;
  display.fillRect(x, y + 12, 74, 92, fill);
  display.fillTriangle(x + 50, y + 12, x + 74, y + 36, x + 74, y + 12, TFT_WHITE);
  display.drawRect(x, y + 12, 74, 92, TFT_BLACK);
  display.drawRect(x + 1, y + 13, 72, 90, active ? TFT_BLACK : TFT_WHITE);
  for (int pin = 0; pin < 5; ++pin) {
    display.fillRect(x + 9 + pin * 11, y + 22, 6, 20, text);
  }
  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(text, fill);
  display.setFont(&fonts::Font4);
  display.drawString("SD", x + 37, y + 72);
}

void drawSdCard()
{
  drawSectionFrame(kSdY, kSdH, "SD 卡测试", "STORAGE / R-W");

  String badge;
  bool badge_dark = false;
  switch (sd_result.state) {
    case SdState::kPassed:
      badge = "PASS";
      badge_dark = true;
      break;
    case SdState::kFailed:
      badge = "FAIL";
      break;
    case SdState::kNoCard:
      badge = "NO CARD";
      break;
    case SdState::kUnchecked:
    default:
      badge = "TEST";
      break;
  }
  drawStatusBadge(kCenterX + kCenterW - 108, kSdY + 10, 88, badge, badge_dark, true);
  drawSdIcon(kCenterX + 22, kSdY + 78, sd_result.state == SdState::kPassed);

  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1);
  const int info_x = kCenterX + 120;
  const int info_y = kSdY + 69;

  if (sd_result.state == SdState::kNoCard) {
    setUiFont(1, true);
    display.drawString("未发现 SD 卡", info_x, info_y + 20);
    setUiFont(1);
    display.drawString("请插入卡后重新测试", info_x, info_y + 58);
    display.setFont(&fonts::Font2);
    display.drawString("CARD DETECT: EMPTY", info_x, info_y + 92);
    return;
  }

  if (sd_result.state == SdState::kUnchecked) {
    setUiFont(1, true);
    display.drawString("正在检测存储卡...", info_x, info_y + 38);
    display.setFont(&fonts::Font2);
    display.drawString("MOUNT / WRITE / READ", info_x, info_y + 76);
    return;
  }

  if (sd_result.state == SdState::kFailed) {
    setUiFont(1, true);
    display.drawString("SD 卡测试失败", info_x, info_y + 16);
    setUiFont(1);
    display.drawString(fitText(sd_result.error, 270), info_x, info_y + 53);
    display.setFont(&fonts::Font2);
    display.drawString("CHECK CARD / FORMAT", info_x, info_y + 91);
    return;
  }

  display.drawString(String("类型：") + sd_result.card_type, info_x, info_y);
  display.drawString(String("容量：") + formatCapacity(sd_result.card_bytes), info_x, info_y + 31);
  display.drawString(String("空间：") + formatCapacity(sd_result.used_bytes) + " / "
                         + formatCapacity(sd_result.total_bytes),
                     info_x, info_y + 62);
  display.drawString(String("根目录：") + sd_result.directory_count + " 目录  "
                         + sd_result.file_count + " 文件",
                     info_x, info_y + 93);
  display.drawString("挂载：PASS   读写：PASS", info_x, info_y + 124);
}

int signalLevel(int32_t rssi)
{
  if (rssi >= -55) return 4;
  if (rssi >= -67) return 3;
  if (rssi >= -78) return 2;
  return 1;
}

void drawSignalBars(int x, int baseline_y, int32_t rssi, uint32_t foreground, uint32_t background)
{
  const int level = signalLevel(rssi);
  for (int bar = 0; bar < 4; ++bar) {
    const int height = 5 + bar * 5;
    const int bx = x + bar * 8;
    const int by = baseline_y - height;
    display.fillRect(bx, by, 5, height, background);
    if (bar < level) {
      display.fillRect(bx, by, 5, height, foreground);
    } else {
      display.drawRect(bx, by, 5, height, foreground);
    }
  }
}

String wifiBadgeText()
{
  switch (wifi_state) {
    case WifiState::kConnected: return "已连接";
    case WifiState::kConnecting: return "连接中";
    case WifiState::kScanOnly: return "已扫描";
    case WifiState::kConnectFailed: return "未连接";
    case WifiState::kScanFailed: return "扫描失败";
    case WifiState::kScanning:
    default: return "扫描中";
  }
}

void drawNetworkRows(int first_y, size_t max_rows)
{
  const size_t rows = std::min(wifi_network_count, max_rows);
  display.setTextDatum(textdatum_t::middle_left);
  setUiFont(1);

  if (rows == 0) {
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.drawString(wifi_state == WifiState::kScanning ? "正在扫描附近 WiFi..." : "未扫描到 WiFi",
                       kCenterX + 20, first_y + 18);
    return;
  }

  for (size_t index = 0; index < rows; ++index) {
    const int y = first_y + static_cast<int>(index) * 34;
    display.fillRect(kCenterX + 14, y, kCenterW - 28, 33, TFT_WHITE);
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setFont(&fonts::Font2);
    display.drawString(String(index + 1), kCenterX + 20, y + 16);
    setUiFont(1);
    String ssid = wifi_networks[index].ssid.length() ? wifi_networks[index].ssid : "<隐藏网络>";
    display.drawString(fitText(ssid, 220), kCenterX + 48, y + 16);
    display.setFont(&fonts::Font2);
    display.drawString(String(wifi_networks[index].rssi) + " dBm", kCenterX + 292, y + 16);
    drawSignalBars(kCenterX + 372, y + 27, wifi_networks[index].rssi,
                   TFT_BLACK, TFT_WHITE);
  }
}

void drawWifiCard()
{
  drawSectionFrame(kWifiY, kWifiH, "WiFi 测试", "SCAN / CONNECT");
  drawStatusBadge(kCenterX + kCenterW - 108, kWifiY + 10, 88, wifiBadgeText(),
                  wifi_state == WifiState::kConnected);

  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1);

  if (wifi_state == WifiState::kConnected) {
    const int panel_y = kWifiY + 62;
    display.fillRoundRect(kCenterX + 14, panel_y, kCenterW - 28, 108, 10, TFT_BLACK);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    setUiFont(1, true);
    display.drawString(fitText(connected_ssid, 290), kCenterX + 32, panel_y + 14);
    setUiFont(1);
    display.drawString(String("IP 地址：") + connected_ip, kCenterX + 32, panel_y + 47);
    display.drawString(String("信号：") + connected_rssi + " dBm", kCenterX + 32, panel_y + 76);
    drawSignalBars(kCenterX + 353, panel_y + 83, connected_rssi, TFT_WHITE, TFT_BLACK);

    display.setTextColor(TFT_BLACK, TFT_WHITE);
    setUiFont(1, true);
    display.drawString(String("附近网络（按信号排序，共 ") + wifi_network_count + " 个）",
                       kCenterX + 18, kWifiY + 181);
    drawNetworkRows(kWifiY + 211, 5);
    return;
  }

  int list_y = kWifiY + 96;
  if (wifi_state == WifiState::kScanning) {
    display.drawString("正在扫描附近 WiFi，请稍候...", kCenterX + 20, kWifiY + 65);
  } else if (wifi_state == WifiState::kConnecting && active_wifi_target >= 0) {
    display.drawString(String("正在连接：") + kWifiTargets[active_wifi_target].ssid,
                       kCenterX + 20, kWifiY + 65);
  } else if (wifi_state == WifiState::kScanOnly) {
    display.drawString("未发现预设 WiFi，仅显示扫描结果", kCenterX + 20, kWifiY + 65);
  } else if (wifi_state == WifiState::kConnectFailed) {
    display.drawString("预设 WiFi 连接失败，扫描结果如下", kCenterX + 20, kWifiY + 65);
  } else {
    display.drawString("WiFi 扫描失败，请检查射频功能", kCenterX + 20, kWifiY + 65);
  }
  drawNetworkRows(list_y, 8);
}

void drawSideButton(size_t visual_index)
{
  const ButtonVisual& visual = kButtonVisuals[visual_index];
  const bool pressed = stable_input.buttons[visual.logical_index];
  display.fillRoundRect(visual.x, visual.y, kButtonW, kButtonH, 8, TFT_WHITE);
  display.drawRoundRect(visual.x, visual.y, kButtonW, kButtonH, 8, TFT_BLACK);
  if (pressed) {
    display.drawRoundRect(visual.x + 2, visual.y + 2,
                          kButtonW - 4, kButtonH - 4, 6, TFT_BLACK);
  }
  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setFont(&fonts::Font2);
  display.drawString(visual.label, visual.x + kButtonW / 2, visual.y + kButtonH / 2 - 7);
  display.setFont(&fonts::Font0);
  display.drawString(pressed ? "DOWN" : "UP", visual.x + kButtonW / 2,
                     visual.y + kButtonH / 2 + 12);
}

void drawButtonConnectors()
{
  for (const ButtonVisual& visual : kButtonVisuals) {
    const int mid_y = visual.y + kButtonH / 2;
    if (visual.x == 0) {
      display.drawFastHLine(kButtonW, mid_y, kCenterX - kButtonW, TFT_BLACK);
    } else {
      display.drawFastHLine(kCenterX + kCenterW, mid_y,
                            visual.x - (kCenterX + kCenterW), TFT_BLACK);
    }
  }
}

void drawMainScreen()
{
  display.fillScreen(TFT_WHITE);
  drawCornerMarks();
  drawButtonConnectors();
  drawMainHeader();
  drawSdCard();
  drawWifiCard();
  for (size_t index = 0; index < kButtonVisuals.size(); ++index) {
    drawSideButton(index);
  }

  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setUiFont(1);
  display.drawString("按下侧键检查输入 · 黑色表示按下", display.width() / 2, 910);
  display.setFont(&fonts::Font2);
  display.drawString("LILYGO FACTORY TEST  |  EPD / SD / WIFI / KEYS", display.width() / 2, 940);
}

void refreshMainFull()
{
  drawMainScreen();
  display.setEpdMode(lgfx::epd_quality);
  display.display();
  display.waitDisplay();
}

void refreshSdCard()
{
  drawSdCard();
  refreshLogicalRect(kCenterX, kSdY, kCenterW, kSdH, lgfx::epd_text);
}

void refreshWifiCard()
{
  drawWifiCard();
  refreshLogicalRect(kCenterX, kWifiY, kCenterW, kWifiH, lgfx::epd_text);
}

bool inputEquals(const InputState& lhs, const InputState& rhs)
{
  return lhs.port == rhs.port && lhs.buttons == rhs.buttons
         && lhs.card_inserted == rhs.card_inserted;
}

InputState readInput()
{
  InputState state;
  uint16_t port = kUsedInputMask;
  if (io_ready) {
    port = io_expander.digitalReadPort() & kUsedInputMask;
  }
  state.port = port;
  for (size_t index = 0; index < 5; ++index) {
    state.buttons[index] = (port & (1U << index)) == 0;
  }
  state.buttons[5] = digitalRead(board::kPinBoot) == LOW;
  state.card_inserted = io_ready && ((port & kCardMask) == 0);
  return state;
}

void countRootEntries()
{
  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    return;
  }
  File entry = root.openNextFile();
  while (entry) {
    if (entry.isDirectory()) {
      ++sd_result.directory_count;
    } else {
      ++sd_result.file_count;
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

void runSdTest(bool card_detected)
{
  sd_result = SdResult{};
  if (io_ready && !card_detected) {
    SD.end();
    sd_result.state = SdState::kNoCard;
    Serial.println("[SD] no card detected");
    return;
  }

  if (!sd_bus_ready) {
    pinMode(board::kPinSdCs, OUTPUT);
    digitalWrite(board::kPinSdCs, HIGH);
    SPI.begin(board::kPinSdSck, board::kPinSdMiso, board::kPinSdMosi, board::kPinSdCs);
    sd_bus_ready = true;
  }

  bool mounted = false;
  for (int attempt = 0; attempt < 3 && !mounted; ++attempt) {
    SD.end();
    delay(20);
    mounted = SD.begin(board::kPinSdCs, SPI, board::kSdSpiClock);
    if (!mounted) delay(80);
  }
  if (!mounted || SD.cardType() == CARD_NONE) {
    sd_result.state = io_ready && card_detected ? SdState::kFailed : SdState::kNoCard;
    sd_result.error = "无法挂载 SD 卡";
    Serial.println("[SD] mount failed");
    return;
  }

  sd_result.card_type = sdCardTypeName(SD.cardType());
  sd_result.card_bytes = SD.cardSize();
  sd_result.total_bytes = SD.totalBytes();
  sd_result.used_bytes = SD.usedBytes();
  countRootEntries();

  SD.remove(kSdTestPath);
  File test_file = SD.open(kSdTestPath, FILE_WRITE);
  if (!test_file) {
    sd_result.state = SdState::kFailed;
    sd_result.error = "创建测试文件失败";
    Serial.println("[SD] test file create failed");
    return;
  }
  const size_t payload_size = strlen(kSdTestPayload);
  const size_t written = test_file.write(reinterpret_cast<const uint8_t*>(kSdTestPayload), payload_size);
  test_file.close();
  if (written != payload_size) {
    sd_result.state = SdState::kFailed;
    sd_result.error = "写入测试文件失败";
    SD.remove(kSdTestPath);
    Serial.println("[SD] write failed");
    return;
  }

  char readback[sizeof(kSdTestPayload)] = {};
  test_file = SD.open(kSdTestPath, FILE_READ);
  const size_t read_size = test_file ? test_file.readBytes(readback, payload_size) : 0;
  if (test_file) test_file.close();
  const bool content_ok = read_size == payload_size
                          && memcmp(readback, kSdTestPayload, payload_size) == 0;
  const bool removed = SD.remove(kSdTestPath);
  if (!content_ok || !removed) {
    sd_result.state = SdState::kFailed;
    sd_result.error = !content_ok ? "读取校验失败" : "删除测试文件失败";
    Serial.println("[SD] read-back or cleanup failed");
    return;
  }

  sd_result.read_write_ok = true;
  sd_result.state = SdState::kPassed;
  Serial.printf("[SD] PASS, type=%s, size=%llu MB\n", sd_result.card_type.c_str(),
                sd_result.card_bytes / (1024ULL * 1024ULL));
}

void beginWifiScan()
{
  wifi_state = WifiState::kScanning;
  wifi_network_count = 0;
  wifi_target_found.fill(false);
  next_wifi_target = 0;
  active_wifi_target = -1;
  connected_ssid = "";
  connected_ip = "";
  connected_rssi = -127;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  const int result = WiFi.scanNetworks(true, true);
  if (result == WIFI_SCAN_FAILED) {
    wifi_state = WifiState::kScanFailed;
    Serial.println("[WiFi] could not start scan");
  } else {
    Serial.println("[WiFi] asynchronous scan started");
  }
}

bool startNextWifiTarget()
{
  while (next_wifi_target < kWifiTargets.size()) {
    const size_t target = next_wifi_target++;
    if (!wifi_target_found[target]) {
      continue;
    }
    active_wifi_target = static_cast<int>(target);
    wifi_state = WifiState::kConnecting;
    wifi_connect_started_ms = millis();
    WiFi.disconnect();
    WiFi.begin(kWifiTargets[target].ssid, kWifiTargets[target].password);
    Serial.printf("[WiFi] connecting to %s\n", kWifiTargets[target].ssid);
    refreshWifiCard();
    return true;
  }
  active_wifi_target = -1;
  return false;
}

void finishWifiScan(int count)
{
  wifi_network_count = std::min(static_cast<size_t>(std::max(count, 0)), kMaxWifiNetworks);
  for (size_t index = 0; index < wifi_network_count; ++index) {
    wifi_networks[index].ssid = WiFi.SSID(static_cast<int>(index));
    wifi_networks[index].rssi = WiFi.RSSI(static_cast<int>(index));
    wifi_networks[index].channel = WiFi.channel(static_cast<int>(index));
    wifi_networks[index].auth = WiFi.encryptionType(static_cast<int>(index));
  }
  std::sort(wifi_networks.begin(), wifi_networks.begin() + wifi_network_count,
            [](const WifiNetwork& lhs, const WifiNetwork& rhs) { return lhs.rssi > rhs.rssi; });

  for (size_t index = 0; index < wifi_network_count; ++index) {
    const WifiNetwork& network = wifi_networks[index];
    if (network.ssid.length() == 0) continue;
    for (size_t target = 0; target < kWifiTargets.size(); ++target) {
      if (network.ssid == kWifiTargets[target].ssid) {
        wifi_target_found[target] = true;
      }
    }
  }
  WiFi.scanDelete();

  Serial.printf("[WiFi] %u network(s) retained, sorted by RSSI\n",
                static_cast<unsigned>(wifi_network_count));
  for (size_t index = 0; index < wifi_network_count; ++index) {
    Serial.printf("  %2u. %-32s %4d dBm\n", static_cast<unsigned>(index + 1),
                  wifi_networks[index].ssid.c_str(), wifi_networks[index].rssi);
  }

  next_wifi_target = 0;
  if (!startNextWifiTarget()) {
    wifi_state = WifiState::kScanOnly;
    refreshWifiCard();
  }
}

void pollWifi()
{
  if (wifi_state == WifiState::kScanning) {
    const int result = WiFi.scanComplete();
    if (result >= 0) {
      finishWifiScan(result);
    } else if (result == WIFI_SCAN_FAILED) {
      wifi_state = WifiState::kScanFailed;
      refreshWifiCard();
    }
    return;
  }

  if (wifi_state != WifiState::kConnecting) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    connected_ssid = WiFi.SSID();
    connected_ip = WiFi.localIP().toString();
    connected_rssi = WiFi.RSSI();
    wifi_state = WifiState::kConnected;
    Serial.printf("[WiFi] connected: %s, IP=%s, RSSI=%d dBm\n", connected_ssid.c_str(),
                  connected_ip.c_str(), connected_rssi);
    refreshWifiCard();
    return;
  }

  if (millis() - wifi_connect_started_ms >= kWifiConnectTimeoutMs) {
    Serial.printf("[WiFi] connection timeout: %s\n",
                  active_wifi_target >= 0 ? kWifiTargets[active_wifi_target].ssid : "unknown");
    WiFi.disconnect();
    if (!startNextWifiTarget()) {
      wifi_state = WifiState::kConnectFailed;
      refreshWifiCard();
    }
  }
}

void handleStableInput(const InputState& previous)
{
  for (size_t visual_index = 0; visual_index < kButtonVisuals.size(); ++visual_index) {
    const uint8_t logical_index = kButtonVisuals[visual_index].logical_index;
    if (previous.buttons[logical_index] == stable_input.buttons[logical_index]) {
      continue;
    }
    Serial.printf("[KEY] %s -> %s\n", kButtonVisuals[visual_index].label,
                  stable_input.buttons[logical_index] ? "DOWN" : "UP");
    drawSideButton(visual_index);
    refreshLogicalRect(kButtonVisuals[visual_index].x, kButtonVisuals[visual_index].y,
                       kButtonW, kButtonH, lgfx::epd_fastest);
  }

  if (previous.card_inserted != stable_input.card_inserted) {
    Serial.printf("[SD] card detect -> %s\n", stable_input.card_inserted ? "INSERTED" : "REMOVED");
    sd_result = SdResult{};
    refreshSdCard();
    runSdTest(stable_input.card_inserted);
    refreshSdCard();
  }
}

void pollInputs()
{
  const InputState raw = readInput();
  if (!inputEquals(raw, sampled_input)) {
    sampled_input = raw;
    sampled_change_ms = millis();
    return;
  }
  if (inputEquals(raw, stable_input) || millis() - sampled_change_ms < kDebounceMs) {
    return;
  }
  const InputState previous = stable_input;
  stable_input = raw;
  handleStableInput(previous);
}
}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);

#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
  Serial.enableReboot(false);
#endif

  Serial.println("\n========== LILYGO FACTORY TEST ==========");
  pinMode(board::kPinExtIrq, INPUT_PULLUP);
  pinMode(board::kPinBoot, INPUT_PULLUP);

  if (!display.init()) {
    Serial.println("[EPD] init failed");
    while (true) delay(1000);
  }
  display.setRotation(0);
  display.setColorDepth(4);
  display.setAutoDisplay(false);
  screen_ready = true;

  Serial.printf("[EPD] ready: %d x %d\n", display.width(), display.height());
  drawStartupPattern();
  display.setEpdMode(lgfx::epd_quality);
  display.display();
  display.waitDisplay();
  Serial.println("[EPD] startup pattern visible for 3 seconds");
  delay(kSplashHoldMs);

  io_ready = io_expander.begin(Wire, XL9555_SLAVE_ADDRESS0,
                               board::kPinI2cSda, board::kPinI2cScl);
  if (io_ready) {
    Wire.setClock(400000);
    io_expander.configPins(kUsedInputMask, INPUT);
    Serial.println("[IO] XL9555 ready");
  } else {
    Serial.println("[IO] XL9555 not found");
  }

  stable_input = readInput();
  sampled_input = stable_input;
  sampled_change_ms = millis();
  sd_result.state = SdState::kUnchecked;
  wifi_state = WifiState::kScanning;
  refreshMainFull();

  runSdTest(stable_input.card_inserted);
  refreshSdCard();
  beginWifiScan();
  if (wifi_state == WifiState::kScanFailed) {
    refreshWifiCard();
  }
}

void loop()
{
  pollInputs();
  pollWifi();
  delay(5);
}
