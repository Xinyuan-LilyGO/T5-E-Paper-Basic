#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// T5 E-Paper Basic SD card pin definitions
namespace board
{
constexpr int kPinSdMosi = 38;
constexpr int kPinSdSck  = 39;
constexpr int kPinSdMiso = 40;
constexpr int kPinSdCs   = 47;
constexpr uint32_t kSdSpiClock = 16000000;
}  // namespace board

static const char* kTestFilePath = "/sd_test.txt";
static const char* kTestPayload  = "T5-E-Paper-Basic SD test\n";

static const char* cardTypeName(uint8_t t)
{
  switch (t) {
    case CARD_MMC:  return "MMC";
    case CARD_SD:   return "SDSC";
    case CARD_SDHC: return "SDHC/SDXC";
    default:        return "UNKNOWN";
  }
}

static void listDir(const char* path, uint8_t depth = 0)
{
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    Serial.printf("  [ERR] cannot open directory: %s\n", path);
    return;
  }

  File entry = dir.openNextFile();
  while (entry) {
    for (uint8_t i = 0; i < depth; ++i) Serial.print("  ");
    if (entry.isDirectory()) {
      Serial.printf("[DIR]  %s\n", entry.name());
      if (depth < 1) listDir(entry.path(), depth + 1);
    } else {
      Serial.printf("[FILE] %-32s  %lu bytes\n", entry.name(),
                    static_cast<unsigned long>(entry.size()));
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
}

static bool runWriteReadTest()
{
  // Write
  SD.remove(kTestFilePath);
  File f = SD.open(kTestFilePath, FILE_WRITE);
  if (!f) {
    Serial.println("  [ERR] open for write failed");
    return false;
  }
  const size_t written = f.print(kTestPayload);
  f.close();
  if (written != strlen(kTestPayload)) {
    Serial.printf("  [ERR] wrote %u bytes, expected %u\n",
                  static_cast<unsigned>(written),
                  static_cast<unsigned>(strlen(kTestPayload)));
    return false;
  }
  Serial.printf("  write : %u bytes -> OK\n", static_cast<unsigned>(written));

  // Read back
  f = SD.open(kTestFilePath, FILE_READ);
  if (!f) {
    Serial.println("  [ERR] open for read failed");
    return false;
  }
  String readback;
  while (f.available()) readback += static_cast<char>(f.read());
  f.close();

  if (readback != kTestPayload) {
    Serial.println("  [ERR] readback mismatch");
    Serial.printf("  expected : %s", kTestPayload);
    Serial.printf("  got      : %s", readback.c_str());
    return false;
  }
  Serial.println("  read  : content verified -> OK");

  // Cleanup
  if (!SD.remove(kTestFilePath)) {
    Serial.println("  [WARN] test file removal failed");
    return false;
  }
  Serial.println("  clean : test file removed -> OK");
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n========== SD Card Test ==========");

  // Pull CS high before SPI init so the card sees a clean deselect
  pinMode(board::kPinSdCs, OUTPUT);
  digitalWrite(board::kPinSdCs, HIGH);
  delay(10);

  // Init SPI bus
  SPI.begin(board::kPinSdSck, board::kPinSdMiso, board::kPinSdMosi, board::kPinSdCs);
  Serial.printf("SPI  : MOSI=%d  SCK=%d  MISO=%d  CS=%d\n",
                board::kPinSdMosi, board::kPinSdSck,
                board::kPinSdMiso, board::kPinSdCs);

  // Mount SD — retry up to 3 times to survive warm-reset state residue
  bool mounted = false;
  for (int attempt = 1; attempt <= 3 && !mounted; ++attempt) {
    SD.end();
    delay(20);
    mounted = SD.begin(board::kPinSdCs, SPI, board::kSdSpiClock);
    if (!mounted) {
      Serial.printf("MOUNT: attempt %d failed, retrying...\n", attempt);
      delay(100);
    }
  }
  if (!mounted) {
    Serial.println("MOUNT: FAILED - no card or wiring error");
    Serial.println("==================================");
    return;
  }
  Serial.println("MOUNT: OK");

  // Card info
  const uint8_t card_type = SD.cardType();
  if (card_type == CARD_NONE) {
    Serial.println("TYPE : none detected");
    Serial.println("==================================");
    return;
  }
  Serial.printf("TYPE : %s\n", cardTypeName(card_type));
  Serial.printf("SIZE : %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));
  Serial.printf("TOTAL: %llu MB\n", SD.totalBytes() / (1024ULL * 1024ULL));
  Serial.printf("USED : %llu MB\n", SD.usedBytes() / (1024ULL * 1024ULL));

  // Directory listing
  Serial.println("---------- Root Listing ----------");
  listDir("/");

  // Write / read / delete test
  Serial.println("---------- Write/Read Test -------");
  const bool rw_ok = runWriteReadTest();

  Serial.println("---------- Result ----------------");
  Serial.printf("RESULT: %s\n", rw_ok ? "PASS" : "FAIL");
  Serial.println("==================================");
}

void loop()
{
  delay(10000);
}
