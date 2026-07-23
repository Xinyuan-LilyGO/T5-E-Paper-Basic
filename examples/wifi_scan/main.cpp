#include <Arduino.h>
#include <WiFi.h>

void setup()
{
  Serial.begin(115200);
  delay(200);

#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
  Serial.enableReboot(false);
#endif

  Serial.println("\nWiFi Scanner");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop()
{
  Serial.println("Scanning...");
  int count = WiFi.scanNetworks();

  if (count == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.printf("%d network(s) found:\n", count);
    Serial.println("------------------------------------------------------------");
    Serial.printf("%-4s %-32s %-8s %-6s %s\n", "#", "SSID", "RSSI", "Chan", "Security");
    Serial.println("------------------------------------------------------------");
    for (int i = 0; i < count; ++i) {
      const char* auth;
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:          auth = "OPEN";    break;
        case WIFI_AUTH_WEP:           auth = "WEP";     break;
        case WIFI_AUTH_WPA_PSK:       auth = "WPA";     break;
        case WIFI_AUTH_WPA2_PSK:      auth = "WPA2";    break;
        case WIFI_AUTH_WPA_WPA2_PSK:  auth = "WPA/2";   break;
        case WIFI_AUTH_WPA2_ENTERPRISE: auth = "WPA2-E"; break;
        case WIFI_AUTH_WPA3_PSK:      auth = "WPA3";    break;
        default:                      auth = "UNKNOWN"; break;
      }
      Serial.printf("%-4d %-32s %-8d %-6d %s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    auth);
    }
    Serial.println("------------------------------------------------------------");
  }

  WiFi.scanDelete();
  Serial.println("Next scan in 10 seconds.\n");
  delay(10000);
}
