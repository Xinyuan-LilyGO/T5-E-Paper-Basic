#include <Arduino.h>

#include <algorithm>

#include <M5GFX.h>
#include <lgfx/v1/platforms/esp32/Bus_EPD.h>
#include <lgfx/v1/platforms/esp32/Panel_EPD.hpp>

extern "C" {
extern const uint8_t logo_png_start[];
extern const uint8_t logo_png_end[];
}

namespace board
{
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
// Edit these fields to generate another employee badge.
constexpr char kEmployeeName[] = "Your Name Here";
constexpr char kDepartment[] = "Your Department Here";
constexpr char kEmployeeId[] = "LGO-00000";
constexpr char kWebsite[] = "www.lilygo.cc";
constexpr char kQrPayload[] = "https://lilygo.cc/";

constexpr int kFrameX = 8;
constexpr int kFrameY = 8;
constexpr int kFrameW = 524;
constexpr int kFrameH = 944;

constexpr int kPortraitX = 105;
constexpr int kPortraitY = 190;
constexpr int kPortraitW = 330;
constexpr int kPortraitH = 310;
constexpr int kPortraitCropX = 45;
constexpr int kPortraitCropY = 108;
constexpr float kPortraitScale = 1.5f;

LilyGoEPaperBasicDisplay display;

uint32_t gray(uint8_t value)
{
  return display.color888(value, value, value);
}

void setCnFont(float scale = 1.0f, bool bold = false)
{
  display.setFont(bold ? &fonts::efontCN_16_b : &fonts::efontCN_16);
  display.setTextSize(scale);
}

String fitText(String text, int max_width)
{
  if (display.textWidth(text) <= max_width) return text;

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

void drawNode(int x, int y, uint32_t color)
{
  display.fillCircle(x, y, 4, TFT_WHITE);
  display.drawCircle(x, y, 4, color);
  display.drawCircle(x, y, 3, color);
}

void drawDottedLine(int x0, int x1, int y)
{
  for (int x = x0; x <= x1; x += 7) {
    display.fillCircle(x, y, 1, TFT_BLACK);
  }
}

void drawOuterFrame()
{
  display.fillScreen(TFT_WHITE);
  for (int inset = 0; inset < 4; ++inset) {
    display.drawRoundRect(kFrameX + inset, kFrameY + inset,
                          kFrameW - inset * 2, kFrameH - inset * 2,
                          24 - inset, TFT_BLACK);
  }

  display.fillTriangle(445, 12, 528, 12, 528, 64, gray(205));
  display.fillTriangle(445, 12, 476, 43, 528, 43, gray(205));
}

void drawHeaderDots()
{
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 5; ++col) {
      display.fillCircle(34 + col * 15, 32 + row * 15, 3,
                         gray(175 + row * 12));
    }
  }
  display.fillCircle(34, 92, 3, gray(215));
}

void drawHeaderCircuit()
{
  const uint32_t trace = gray(95);
  display.drawWideLine(350, 30, 425, 30, 1.2f, trace);
  display.drawWideLine(425, 30, 452, 57, 1.2f, trace);
  display.drawWideLine(452, 57, 487, 57, 1.2f, trace);

  display.drawWideLine(372, 47, 417, 47, 1.2f, trace);
  display.drawWideLine(417, 47, 442, 72, 1.2f, trace);
  display.drawWideLine(442, 72, 480, 72, 1.2f, trace);
  display.drawWideLine(480, 72, 506, 98, 1.2f, trace);

  display.drawWideLine(488, 58, 509, 37, 1.2f, trace);
  display.drawWideLine(509, 37, 520, 37, 1.2f, trace);
  display.drawWideLine(506, 98, 526, 98, 1.2f, trace);
  display.drawWideLine(496, 76, 496, 126, 1.2f, trace);

  drawNode(350, 30, trace);
  drawNode(372, 47, trace);
  drawNode(520, 37, trace);
  drawNode(496, 126, trace);
}

void drawBrandHeader()
{
  drawHeaderDots();
  drawHeaderCircuit();

  display.setTextDatum(textdatum_t::top_center);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setFont(&fonts::FreeSansBold24pt7b);
  display.setTextSize(2.0f);
  display.drawString("LILYGO", display.width() / 2, 49);

  display.setFont(&fonts::FreeSansBold9pt7b);
  display.setTextSize(1.0f);
  display.drawString("C O N N E C T   /   C R E A T E   /   E M P O W E R",
                     display.width() / 2, 145);
  display.drawFastHLine(103, 172, 334, gray(205));
}

void drawHero()
{
  display.fillRoundRect(97, 182, 346, 326, 20, TFT_WHITE);
  display.drawRoundRect(97, 182, 346, 326, 20, gray(185));
  display.drawRoundRect(100, 185, 340, 320, 18, gray(225));

  const uint32_t logo_size =
      static_cast<uint32_t>(logo_png_end - logo_png_start);
  const bool image_ok =
      display.drawPng(logo_png_start, logo_size,
                      kPortraitX, kPortraitY, kPortraitW, kPortraitH,
                      kPortraitCropX, kPortraitCropY,
                      kPortraitScale, kPortraitScale,
                      datum_t::top_left);

  if (!image_ok) {
    Serial.println("[PNG] embedded logo.png decode failed");
    display.setTextDatum(textdatum_t::middle_center);
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    setCnFont(1.0f, true);
    display.drawString("人物图像加载失败", display.width() / 2, 345);
  }

  for (int index = 0; index < 3; ++index) {
    const int y = 422 + index * 24;
    const uint32_t shade = index == 2 ? TFT_BLACK : gray(125 + index * 38);
    display.drawWideLine(49, y + 13, 61, y, 2.3f, shade);
    display.drawWideLine(61, y, 73, y + 13, 2.3f, shade);
  }
}

enum class InfoIcon : uint8_t {
  kPerson,
  kBriefcase,
  kIdCard,
};

void drawPersonIcon(int cx, int cy)
{
  display.fillCircle(cx, cy - 10, 10, TFT_BLACK);
  display.fillArc(cx, cy + 17, 0, 18, 180, 360, TFT_BLACK);
  display.fillRect(cx - 18, cy + 10, 36, 8, TFT_BLACK);
}

void drawBriefcaseIcon(int cx, int cy)
{
  display.drawRoundRect(cx - 9, cy - 22, 18, 11, 3, TFT_BLACK);
  display.drawRoundRect(cx - 8, cy - 21, 16, 10, 3, TFT_BLACK);
  display.fillRoundRect(cx - 23, cy - 13, 46, 30, 4, TFT_BLACK);
  display.drawFastHLine(cx - 22, cy, 44, TFT_WHITE);
  display.fillRoundRect(cx - 4, cy - 3, 8, 8, 2, TFT_WHITE);
}

void drawIdCardIcon(int cx, int cy)
{
  display.fillRoundRect(cx - 24, cy - 18, 48, 36, 4, TFT_BLACK);
  display.fillRoundRect(cx - 19, cy - 13, 38, 26, 2, TFT_WHITE);
  display.fillCircle(cx - 10, cy - 5, 5, TFT_BLACK);
  display.fillArc(cx - 10, cy + 9, 0, 9, 180, 360, TFT_BLACK);
  display.drawFastHLine(cx + 1, cy - 7, 14, TFT_BLACK);
  display.drawFastHLine(cx + 1, cy, 14, TFT_BLACK);
  display.drawFastHLine(cx + 1, cy + 7, 10, TFT_BLACK);
}

void drawInfoIcon(InfoIcon icon, int cx, int cy)
{
  display.fillCircle(cx, cy, 29, gray(218));
  display.drawCircle(cx, cy, 29, gray(195));
  switch (icon) {
    case InfoIcon::kPerson:
      drawPersonIcon(cx, cy);
      break;
    case InfoIcon::kBriefcase:
      drawBriefcaseIcon(cx, cy);
      break;
    case InfoIcon::kIdCard:
      drawIdCardIcon(cx, cy);
      break;
  }
}

void drawInfoRow(int top, InfoIcon icon, const char* label, const char* value)
{
  constexpr int kIconX = 72;
  constexpr int kTextX = 121;
  constexpr int kTextW = 376;
  constexpr int kRowH = 75;

  drawInfoIcon(icon, kIconX, top + 34);

  display.setTextDatum(textdatum_t::top_left);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  setCnFont(1.45f, true);
  display.drawString(label, kTextX, top + 4);

  setCnFont(1.08f, false);
  display.drawString(fitText(value, kTextW), kTextX, top + 40);
  drawDottedLine(kTextX, 496, top + kRowH - 1);
}

void drawInformation()
{
  drawInfoRow(516, InfoIcon::kPerson, "姓名 / Name", kEmployeeName);
  drawInfoRow(591, InfoIcon::kBriefcase, "部门 / Department", kDepartment);
  drawInfoRow(666, InfoIcon::kIdCard, "工号 / ID", kEmployeeId);
}

void drawVerificationCircuit()
{
  const uint32_t light = gray(178);
  const uint32_t dark = gray(105);

  display.drawWideLine(21, 770, 70, 770, 1.0f, light);
  display.drawWideLine(70, 770, 91, 791, 1.0f, light);
  display.drawWideLine(91, 791, 154, 791, 1.0f, light);
  display.drawWideLine(154, 791, 173, 810, 1.0f, light);
  display.drawWideLine(173, 810, 205, 810, 1.0f, light);

  display.drawWideLine(21, 794, 60, 794, 1.0f, dark);
  display.drawWideLine(60, 794, 83, 817, 1.0f, dark);
  display.drawWideLine(83, 817, 131, 817, 1.0f, dark);
  display.drawWideLine(131, 817, 154, 840, 1.0f, dark);
  display.drawWideLine(154, 840, 204, 840, 1.0f, dark);

  display.drawWideLine(21, 821, 54, 821, 1.0f, light);
  display.drawWideLine(54, 821, 78, 845, 1.0f, light);
  display.drawWideLine(78, 845, 121, 845, 1.0f, light);
  display.drawWideLine(121, 845, 139, 863, 1.0f, light);
  display.drawWideLine(139, 863, 198, 863, 1.0f, light);

  display.drawWideLine(519, 770, 470, 770, 1.0f, light);
  display.drawWideLine(470, 770, 449, 791, 1.0f, light);
  display.drawWideLine(449, 791, 386, 791, 1.0f, light);
  display.drawWideLine(386, 791, 367, 810, 1.0f, light);
  display.drawWideLine(367, 810, 335, 810, 1.0f, light);

  display.drawWideLine(519, 794, 480, 794, 1.0f, dark);
  display.drawWideLine(480, 794, 457, 817, 1.0f, dark);
  display.drawWideLine(457, 817, 409, 817, 1.0f, dark);
  display.drawWideLine(409, 817, 386, 840, 1.0f, dark);
  display.drawWideLine(386, 840, 336, 840, 1.0f, dark);

  display.drawWideLine(519, 821, 486, 821, 1.0f, light);
  display.drawWideLine(486, 821, 462, 845, 1.0f, light);
  display.drawWideLine(462, 845, 419, 845, 1.0f, light);
  display.drawWideLine(419, 845, 401, 863, 1.0f, light);
  display.drawWideLine(401, 863, 342, 863, 1.0f, light);

  drawNode(205, 810, light);
  drawNode(204, 840, dark);
  drawNode(198, 863, light);
  drawNode(335, 810, light);
  drawNode(336, 840, dark);
  drawNode(342, 863, light);
}

void drawQrPanel()
{
  drawVerificationCircuit();

  display.setTextDatum(textdatum_t::top_center);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setFont(&fonts::Font0);
  display.setTextSize(1.0f);
  display.drawString("SCAN / VERIFY", display.width() / 2, 748);

  display.fillRoundRect(210, 760, 120, 120, 10, TFT_WHITE);
  display.drawRoundRect(210, 760, 120, 120, 10, TFT_BLACK);
  display.drawRoundRect(212, 762, 116, 116, 8, gray(120));
  display.qrcode(kQrPayload, 224, 774, 92, 2, false);
}

void drawGlobe(int cx, int cy)
{
  display.drawCircle(cx, cy, 20, TFT_WHITE);
  display.drawEllipse(cx, cy, 9, 20, TFT_WHITE);
  display.drawFastHLine(cx - 18, cy - 7, 36, TFT_WHITE);
  display.drawFastHLine(cx - 20, cy, 40, TFT_WHITE);
  display.drawFastHLine(cx - 18, cy + 7, 36, TFT_WHITE);
}

void drawFooter()
{
  constexpr int kRibbonY = 884;
  constexpr int kRibbonH = 33;

  display.fillRect(14, kRibbonY, 512, kRibbonH, gray(205));
  display.fillRect(14, kRibbonY, 88, kRibbonH, TFT_BLACK);
  display.fillTriangle(102, kRibbonY, 132, kRibbonY + kRibbonH,
                       102, kRibbonY + kRibbonH, TFT_BLACK);
  display.fillRect(132, kRibbonY, 222, kRibbonH, TFT_WHITE);
  display.fillTriangle(354, kRibbonY, 384, kRibbonY,
                       354, kRibbonY + kRibbonH, gray(205));

  display.drawWideLine(132, kRibbonY, 354, kRibbonY, 1.4f, TFT_BLACK);
  display.drawWideLine(132, kRibbonY + kRibbonH, 354,
                       kRibbonY + kRibbonH, 1.4f, TFT_BLACK);
  display.drawWideLine(354, kRibbonY, 379,
                       kRibbonY + kRibbonH, 1.4f, TFT_BLACK);

  drawGlobe(61, kRibbonY + kRibbonH / 2);

  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setFont(&fonts::FreeSansBold12pt7b);
  display.setTextSize(1.0f);
  display.drawString(kWebsite, 240, kRibbonY + kRibbonH / 2);

  for (int stripe = 0; stripe < 6; ++stripe) {
    const int x = 403 + stripe * 15;
    display.drawWideLine(x, 908, x + 15, 891, 3.0f, TFT_BLACK);
  }

  display.fillRoundRect(14, 917, 512, 29, 10, TFT_BLACK);
  display.fillRect(14, 917, 512, 11, TFT_BLACK);
  display.fillCircle(36, 932, 3, TFT_WHITE);
  display.drawFastHLine(40, 932, 48, TFT_WHITE);
  display.fillCircle(504, 932, 3, TFT_WHITE);
  display.drawFastHLine(452, 932, 48, TFT_WHITE);

  display.setTextDatum(textdatum_t::middle_center);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.setFont(&fonts::Font2);
  display.setTextSize(1.0f);
  display.drawString("B U I L D   T H E   F U T U R E   T O G E T H E R",
                     display.width() / 2, 932);
}

void drawBadge()
{
  drawOuterFrame();
  drawBrandHeader();
  drawHero();
  drawInformation();
  drawQrPanel();
  drawFooter();
}
}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);

#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
  Serial.enableReboot(false);
#endif

  Serial.println("\n========== LILYGO EMPLOYEE BADGE ==========");
  if (!display.init()) {
    Serial.println("[EPD] init failed");
    while (true) delay(1000);
  }

  display.setRotation(0);
  display.setColorDepth(4);
  display.setAutoDisplay(false);
  Serial.printf("[EPD] ready: %d x %d\n", display.width(), display.height());

  drawBadge();
  display.setEpdMode(lgfx::epd_quality);
  display.display();
  display.waitDisplay();
  Serial.println("[EPD] employee badge displayed");
}

void loop()
{
  delay(1000);
}
