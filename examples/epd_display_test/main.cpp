#include <Arduino.h>
#include <Wire.h>
#include <array>
#include <utility>

#include <M5GFX.h>
#include <IoExpanderXL9555.hpp>
#include <lgfx/v1/platforms/esp32/Bus_EPD.h>
#include <lgfx/v1/platforms/esp32/Panel_EPD.hpp>

namespace board
{
constexpr int kPinI2cSda = 3;
constexpr int kPinI2cScl = 2;
constexpr int kPinExtIrq = 1;

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
    auto bus_cfg = _bus.config();
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
    _bus.config(bus_cfg);

    _panel.setBus(&_bus);

    auto detail_cfg = _panel.config_detail();
    detail_cfg.line_padding = 8;
    _panel.config_detail(detail_cfg);

    auto panel_cfg = _panel.config();
    panel_cfg.memory_width = board::kPanelWidth;
    panel_cfg.panel_width = board::kPanelWidth;
    panel_cfg.memory_height = board::kPanelHeight;
    panel_cfg.panel_height = board::kPanelHeight;
    panel_cfg.offset_rotation = board::kPanelOffsetRotation;
    panel_cfg.offset_x = 0;
    panel_cfg.offset_y = 0;
    panel_cfg.bus_shared = false;
    _panel.config(panel_cfg);

    setPanel(&_panel);
  }

 private:
  lgfx::Bus_EPD _bus;
  lgfx::Panel_EPD _panel;
};

namespace
{
constexpr size_t kPageCount = 5;
constexpr size_t kButtonCount = 5;
constexpr uint8_t kCardDetectPin = 5;
constexpr int kStatusHeight = 104;
constexpr uint32_t kDebounceMs = 20;
constexpr uint32_t kFastRefreshBurstLimit = 6;
constexpr uint16_t kButtonMask = 0x001F;
constexpr uint16_t kCardMask = (1U << kCardDetectPin);
constexpr uint16_t kUsedInputMask = kButtonMask | kCardMask;
constexpr uint16_t kUnusedMask = static_cast<uint16_t>(~kUsedInputMask);

constexpr std::array<uint8_t, kButtonCount> kButtonPins = {0, 1, 2, 3, 4};
constexpr std::array<const char*, kButtonCount> kButtonNames = {
    "KEY1", "KEY2", "KEY3", "KEY4", "KEY5"};
constexpr std::array<const char*, kPageCount> kPageTitles = {
    "Overview", "Geometry", "Gray Scale", "Typography", "Pattern"};

LilyGoEPaperBasicDisplay gfx;
IoExpanderXL9555 io_expander;

struct InputState {
  uint16_t port = 0xFFFF;
  std::array<bool, kButtonCount> buttons = {};
  bool card_inserted = false;
  bool irq_low = false;
};

InputState current_input;
bool io_ready = false;
bool screen_ready = false;
uint8_t current_page = 0;
uint16_t last_sampled_port = 0xFFFF;
uint32_t last_change_ms = 0;
uint32_t full_refresh_count = 0;
uint32_t partial_refresh_count = 0;
uint32_t fast_full_refresh_streak = 0;

struct DisplayRect {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

int minInt(int a, int b)
{
  return (a < b) ? a : b;
}

int contentTop()
{
  return 64;
}

int statusTop()
{
  return gfx.height() - kStatusHeight;
}

int contentHeight()
{
  return statusTop() - contentTop() - 16;
}

DisplayRect logicalToNativeRect(int x, int y, int w, int h)
{
  uint_fast16_t xs = static_cast<uint_fast16_t>(x);
  uint_fast16_t ys = static_cast<uint_fast16_t>(y);
  uint_fast16_t xe = static_cast<uint_fast16_t>(x + w - 1);
  uint_fast16_t ye = static_cast<uint_fast16_t>(y + h - 1);

  const uint_fast8_t rotation = gfx.getRotation() & 7;
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

  DisplayRect native_rect;
  native_rect.x = xs;
  native_rect.y = ys;
  native_rect.w = xe - xs + 1;
  native_rect.h = ye - ys + 1;
  return native_rect;
}

lgfx::epd_mode_t preferredPageRefreshMode(uint8_t page)
{
  switch (page) {
    case 0:
    case 3:
      return lgfx::epd_text;
    case 2:
      return lgfx::epd_quality;
    case 1:
    case 4:
    default:
      return lgfx::epd_fast;
  }
}

InputState decodeInput(uint16_t port_state)
{
  InputState state;
  state.port = port_state;
  for (size_t index = 0; index < kButtonCount; ++index) {
    state.buttons[index] = (port_state & (1U << kButtonPins[index])) == 0;
  }
  state.card_inserted = (port_state & kCardMask) == 0;
  state.irq_low = digitalRead(board::kPinExtIrq) == LOW;
  return state;
}

void drawHeader(const char* title)
{
  gfx.fillRect(0, 0, gfx.width(), 56, TFT_BLACK);
  gfx.setTextColor(TFT_WHITE, TFT_BLACK);
  gfx.setTextSize(2);
  gfx.setCursor(18, 16);
  gfx.print("T5 e-Paper Basic");
  gfx.setCursor(gfx.width() - 18 - gfx.textWidth(title), 16);
  gfx.print(title);
  gfx.drawFastHLine(0, 56, gfx.width(), TFT_BLACK);
}

void drawOverviewPage()
{
  const int x = 24;
  int y = contentTop() + 8;

  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(2);
  gfx.setCursor(x, y);
  gfx.println("EPD display smoke test");

  gfx.setTextSize(1);
  y += 42;
  gfx.fillRoundRect(x, y, gfx.width() - 48, 146, 12, gfx.color888(230, 230, 230));
  gfx.drawRoundRect(x, y, gfx.width() - 48, 146, 12, TFT_BLACK);
  gfx.setCursor(x + 16, y + 16);
  gfx.println("Display driver : m5stack/M5GFX@^0.2.24");
  gfx.println("IO expander    : lewisxhe/SensorLib@^0.4.1");
  gfx.println("Bus            : 8-bit parallel EPD via M5GFX Bus_EPD");
  gfx.println("I2C            : GPIO3(SDA) / GPIO2(SCL)");
  gfx.println("Buttons        : XL9555 P00-P04 -> KEY1..KEY5");
  gfx.println("Card detect    : XL9555 P05");

  y += 170;
  gfx.fillRoundRect(x, y, gfx.width() - 48, 156, 12, TFT_WHITE);
  gfx.drawRoundRect(x, y, gfx.width() - 48, 156, 12, TFT_BLACK);
  gfx.setCursor(x + 16, y + 16);
  gfx.println("Press KEY1..KEY5 to switch test pages:");
  gfx.println("KEY1 Overview   KEY2 Geometry   KEY3 Gray");
  gfx.println("KEY4 Type test  KEY5 Pattern test");
  gfx.println();
  gfx.printf("Resolution      : %d x %d\n", gfx.width(), gfx.height());
  gfx.printf("Full refreshes  : %lu\n", static_cast<unsigned long>(full_refresh_count));
  gfx.printf("Partial refresh : %lu\n", static_cast<unsigned long>(partial_refresh_count));
  gfx.printf("XL9555 status   : %s\n", io_ready ? "ready" : "not found");
}

void drawGeometryPage()
{
  const int margin = 24;
  const int top = contentTop() + 12;
  const int width = gfx.width();
  const int height = contentHeight();
  const int left_width = (width - margin * 3) / 2;
  const int right_x = margin * 2 + left_width;

  gfx.drawRoundRect(margin, top, left_width, 180, 18, TFT_BLACK);
  gfx.fillRoundRect(margin + 12, top + 12, left_width - 24, 156, 18, gfx.color888(220, 220, 220));
  gfx.drawRect(margin + 28, top + 28, left_width - 56, 124, TFT_BLACK);
  gfx.drawRect(margin + 48, top + 48, left_width - 96, 84, TFT_BLACK);
  gfx.fillTriangle(margin + 54, top + 150, margin + 116, top + 54, margin + 178, top + 150,
                   gfx.color888(120, 120, 120));

  const int circle_r = minInt(left_width, 180) / 5;
  gfx.drawCircle(right_x + left_width / 2, top + 56, circle_r, TFT_BLACK);
  gfx.drawCircle(right_x + left_width / 2, top + 56, circle_r + 24, TFT_BLACK);
  gfx.fillCircle(right_x + left_width / 2, top + 56, circle_r - 16, gfx.color888(160, 160, 160));
  gfx.drawEllipse(right_x + left_width / 2, top + 150, left_width / 3, 34, TFT_BLACK);
  gfx.fillEllipse(right_x + left_width / 2, top + 150, left_width / 4, 24, gfx.color888(210, 210, 210));

  const int graph_y = top + 220;
  const int graph_h = height - 236;
  gfx.drawRect(margin, graph_y, width - margin * 2, graph_h, TFT_BLACK);
  for (int step = 0; step <= 10; ++step) {
    const int x = margin + 12 + (width - margin * 2 - 24) * step / 10;
    gfx.drawLine(margin + 12, graph_y + graph_h - 12, x, graph_y + 12, TFT_BLACK);
    gfx.drawLine(width - margin - 12, graph_y + graph_h - 12, x, graph_y + 12,
                 gfx.color888(90, 90, 90));
  }
}

void drawGrayScalePage()
{
  const int margin = 24;
  const int top = contentTop() + 12;
  const int width = gfx.width() - margin * 2;
  const int block_width = width / 16;
  const int bar_height = 140;

  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(1);
  gfx.setCursor(margin, top - 2);
  gfx.println("16 grayscale steps");

  for (int index = 0; index < 16; ++index) {
    const int x = margin + index * block_width;
    const int w = (index == 15) ? (margin + width - x) : block_width;
    const uint8_t gray = index * 17;
    gfx.fillRect(x, top + 22, w, bar_height, gfx.color888(gray, gray, gray));
    gfx.drawRect(x, top + 22, w, bar_height, TFT_BLACK);
    gfx.setTextColor(index > 7 ? TFT_WHITE : TFT_BLACK, gfx.color888(gray, gray, gray));
    gfx.setCursor(x + 4, top + 168);
    gfx.printf("%02d", index);
  }

  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setCursor(margin, top + 220);
  gfx.println("Dither / stripe blocks");

  const int sample_top = top + 248;
  const int sample_h = contentHeight() - 270;
  const int sample_w = (width - 24) / 3;
  for (int block = 0; block < 3; ++block) {
    const int x = margin + block * (sample_w + 12);
    gfx.drawRect(x, sample_top, sample_w, sample_h, TFT_BLACK);
    for (int y = sample_top + 1; y < sample_top + sample_h - 1; ++y) {
      for (int xx = x + 1; xx < x + sample_w - 1; ++xx) {
        bool on = false;
        if (block == 0) {
          on = ((xx + y) & 1) == 0;
        } else if (block == 1) {
          on = (xx % 6) < 3;
        } else {
          on = (y % 8) < 4;
        }
        if (on) {
          gfx.drawPixel(xx, y, TFT_BLACK);
        }
      }
    }
  }
}

void drawTypographyPage()
{
  const int x = 24;
  int y = contentTop() + 8;

  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(1);
  gfx.setCursor(x, y);
  gfx.println("Text rendering, alignment and contrast");

  y += 34;
  gfx.setTextSize(1);
  gfx.setCursor(x, y);
  gfx.println("Size 1: 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  y += 28;

  gfx.setTextSize(2);
  gfx.setCursor(x, y);
  gfx.println("Size 2: Hello EPD");
  y += 48;

  gfx.setTextSize(3);
  gfx.setCursor(x, y);
  gfx.println("Size 3");
  y += 62;

  gfx.fillRoundRect(x, y, gfx.width() - 48, 92, 12, TFT_BLACK);
  gfx.setTextColor(TFT_WHITE, TFT_BLACK);
  gfx.setTextSize(2);
  gfx.setCursor(x + 18, y + 18);
  gfx.println("Inverse text block");
  gfx.setTextSize(1);
  gfx.setCursor(x + 18, y + 56);
  gfx.println("Useful for checking edge ghosting and contrast.");

  y += 122;
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(1);
  gfx.drawRoundRect(x, y, gfx.width() - 48, 120, 12, TFT_BLACK);
  gfx.setCursor(x + 16, y + 18);
  gfx.println("ASCII ramp:");
  gfx.println("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
  gfx.println("The quick brown fox jumps over the lazy dog.");
  gfx.println("0O1Il5S8B  <- look for ambiguous glyph edges.");
}

void drawPatternPage()
{
  const int margin = 24;
  const int top = contentTop() + 12;
  const int width = gfx.width() - margin * 2;
  const int height = contentHeight();
  const int half_w = (width - 12) / 2;
  const int cell = minInt(24, half_w / 12);

  gfx.drawRect(margin, top, half_w, height - 24, TFT_BLACK);
  gfx.drawRect(margin + half_w + 12, top, half_w, height - 24, TFT_BLACK);

  for (int y = top + 1; y < top + height - 25; y += cell) {
    for (int x = margin + 1; x < margin + half_w - 1; x += cell) {
      if ((((x - margin) / cell) + ((y - top) / cell)) & 1) {
        gfx.fillRect(x, y, cell, cell, TFT_BLACK);
      }
    }
  }

  const int right_x = margin + half_w + 12;
  for (int step = 0; step < half_w - 4; step += 6) {
    gfx.drawLine(right_x + 2, top + 2, right_x + 2 + step, top + height - 28, TFT_BLACK);
    gfx.drawLine(right_x + half_w - 3, top + 2, right_x + half_w - 3 - step, top + height - 28,
                 gfx.color888(80, 80, 80));
  }

  const int strip_y = top + height - 18;
  for (int x = margin; x < margin + width; ++x) {
    const uint8_t gray = static_cast<uint8_t>((x - margin) * 255 / width);
    gfx.drawFastVLine(x, strip_y, 14, gfx.color888(gray, gray, gray));
  }
}

void drawStatusBar(const InputState& state)
{
  const int y = statusTop();
  gfx.fillRect(0, y, gfx.width(), kStatusHeight, TFT_WHITE);
  gfx.drawFastHLine(0, y, gfx.width(), TFT_BLACK);
  gfx.drawFastHLine(0, y + 1, gfx.width(), TFT_BLACK);

  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(1);
  gfx.setCursor(18, y + 14);
  gfx.printf("Page: %u/%u  Title: %s", current_page + 1, static_cast<unsigned>(kPageCount),
             kPageTitles[current_page]);

  gfx.setCursor(18, y + 38);
  for (size_t index = 0; index < kButtonCount; ++index) {
    gfx.printf("%s:%s  ", kButtonNames[index], state.buttons[index] ? "DOWN" : "UP");
  }

  gfx.setCursor(18, y + 62);
  gfx.printf("XL9555: %s  PORT: 0x%04X  IRQ:%s  TF:%s", io_ready ? "OK" : "FAIL", state.port,
             state.irq_low ? "LOW" : "HIGH", state.card_inserted ? "IN" : "OUT");

  gfx.setCursor(18, y + 84);
  gfx.printf("Full=%lu  Partial=%lu", static_cast<unsigned long>(full_refresh_count),
             static_cast<unsigned long>(partial_refresh_count));
}

void drawCurrentPage()
{
  gfx.fillScreen(TFT_WHITE);
  drawHeader(kPageTitles[current_page]);

  switch (current_page) {
    case 0:
      drawOverviewPage();
      break;
    case 1:
      drawGeometryPage();
      break;
    case 2:
      drawGrayScalePage();
      break;
    case 3:
      drawTypographyPage();
      break;
    case 4:
    default:
      drawPatternPage();
      break;
  }

  drawStatusBar(current_input);
}

void refreshFull(bool force_quality = false)
{
  ++full_refresh_count;
  drawCurrentPage();
  lgfx::epd_mode_t refresh_mode = preferredPageRefreshMode(current_page);
  const bool needs_maintenance_refresh = fast_full_refresh_streak >= kFastRefreshBurstLimit;
  if (force_quality || needs_maintenance_refresh) {
    refresh_mode = lgfx::epd_quality;
  }

  if (refresh_mode == lgfx::epd_quality) {
    fast_full_refresh_streak = 0;
  } else {
    ++fast_full_refresh_streak;
  }

  gfx.setEpdMode(refresh_mode);
  gfx.display(0, 0, gfx.width(), gfx.height());
  gfx.waitDisplay();
}

void refreshStatusOnly()
{
  ++partial_refresh_count;
  drawStatusBar(current_input);
  gfx.setEpdMode(lgfx::epd_fast);
  // Panel_EPD expects native buffer coordinates for regional updates.
  // The visible UI is rotated into portrait mode, so we convert the logical
  // status bar rect before asking the driver to do a partial refresh.
  const DisplayRect native_rect = logicalToNativeRect(0, statusTop(), gfx.width(), kStatusHeight);
  gfx.display(native_rect.x, native_rect.y, native_rect.w, native_rect.h);
  gfx.waitDisplay();
}

void handleStableInputChange(const InputState& previous, const InputState& current)
{
  current_input = current;

  for (size_t index = 0; index < kButtonCount; ++index) {
    if (!previous.buttons[index] && current.buttons[index]) {
      current_page = static_cast<uint8_t>(index);
      Serial.printf("Switch page -> %u (%s)\n", current_page + 1, kPageTitles[current_page]);
      refreshFull();
      return;
    }
  }

  refreshStatusOnly();
}

void pollExpander()
{
  if (!io_ready || !screen_ready) {
    return;
  }

  const uint16_t sampled_port = io_expander.digitalReadPort() & kUsedInputMask;
  if (sampled_port != last_sampled_port) {
    last_sampled_port = sampled_port;
    last_change_ms = millis();
    return;
  }

  if (sampled_port == current_input.port) {
    return;
  }

  if (millis() - last_change_ms < kDebounceMs) {
    return;
  }

  const InputState next_state = decodeInput(sampled_port);
  const InputState previous = current_input;
  handleStableInputChange(previous, next_state);
}

void showIoInitFailure()
{
  gfx.fillScreen(TFT_WHITE);
  drawHeader("XL9555 Error");
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(2);
  gfx.setCursor(24, contentTop() + 24);
  gfx.println("XL9555 init failed");
  gfx.setTextSize(1);
  gfx.println();
  gfx.println("Check I2C wiring and power, then reset the board.");
  gfx.println("Display test is running, but button switching is disabled.");
  drawStatusBar(current_input);
  gfx.setEpdMode(lgfx::epd_quality);
  gfx.display();
  gfx.waitDisplay();
}
}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);

#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
  Serial.enableReboot(false);
#endif

  pinMode(board::kPinExtIrq, INPUT_PULLUP);

  if (!gfx.init()) {
    Serial.println("EPD init failed. Check board memory_type / PSRAM configuration.");
    while (true) {
      delay(1000);
    }
  }
  gfx.setRotation(0);
  gfx.setAutoDisplay(false);
  screen_ready = true;

  io_ready = io_expander.begin(Wire, XL9555_SLAVE_ADDRESS0, board::kPinI2cSda, board::kPinI2cScl);
  if (io_ready) {
    Wire.setClock(400000);
    io_expander.configPins(kUsedInputMask, INPUT);
    io_expander.configPins(kUnusedMask, OUTPUT);
    io_expander.digitalWritePort(kUnusedMask, kUnusedMask);
    last_sampled_port = io_expander.digitalReadPort() & kUsedInputMask;
    current_input = decodeInput(last_sampled_port);
    Serial.printf("XL9555 ready, initial port=0x%04X\n", current_input.port);
    refreshFull(true);
  } else {
    current_input = decodeInput(0xFFFF);
    Serial.println("XL9555 init failed");
    showIoInitFailure();
  }
}

void loop()
{
  pollExpander();
  delay(20);
}
