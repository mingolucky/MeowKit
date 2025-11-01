/**
 * @file wifi_spam.cpp
 * @author Mingo
 * @brief WiFi Spam app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */

#include "wifi_spam.h"

// ===== Configuration parameters ===== //
#define MAX_SSIDS 100                  // Maximum number of SSIDs
#define CHANNEL_SWITCH_MS 200          // Channel switching interval (ms)
const uint8_t channels[] = {1, 6, 11}; // 2.4GHz channel
uint8_t beaconPacket[128] = {
    0x80, 0x00, 0x00, 0x00,                         // Beacon frame type/control
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             // Destination MAC (broadcast)
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             // Source MAC (base value)
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             // BSSID
    0x00, 0x00,                                     // Segment/Sequence Number
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // Timestamp
    0xE8, 0x03,                                     // Beacon interval 1000ms
    0x21, 0x04,                                     // Capability Information
                                                    // Tag parameter starts
    0x00, 0x20,                                     // SSID tag header (length 32)
                                                    // SSID placeholder (bytes 38-69)
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Rate Parameters
    0x03, 0x01, 0x01                                            // Channel parameters (current channel)
};

// ===== Rap God Lyrics snippet ===== //
const char *rapGodLyrics[] = {
    "Look-I-was-gonna-go-easy", "But-im-not-gonna-just-let-you", "Supernova-in-the-sky",
    "You-get-one-shot-to-make", "The-whole-world-to-shake", "Rap-god-rap-god",
    "Lyrics-comin-at-you", "Supersonic-speed", "From-the-Microwave-Oven",
    "Moms-spaghetti", "Palms-are-sweaty", "Knees-weak-arms-heavy",
    "Vomit-on-his-sweater", "Snap-back-to-reality", "Oh-there-goes-gravity",
    "He-choked-hes-so-mad", "But-he-wont-give-up", "The-whole-crowd-goes-so-loud",
    "He-opens-his-mouth", "But-the-words-wont-come", "Hes-chokin-how",
    "Clocks-back-its-1995", "My-voice-shakes-Baby", "Im-sweatin-but-im-still-breathin",
    "This-is-the-moment", "You-own-it-you-better", "Never-let-it-go",
    "You-only-get-one-shot", "Do-not-miss-your-chance", "This-opportunity-comes",
    "Once-in-a-lifetime", "The-souls-escaping", "Through-this-hole-that-is-gaping",
    "This-world-is-mine", "For-the-taking", "Let-me-lead-the-way",
    "From-the-ashes-of-flames", "Like-a-phoenix-ill-rise", "Write-my-name",
    "In-the-book-of-greats", "Im-a-diamond-in-the-rough", "Shinin-polished-up",
    "Now-im-sittin-pretty", "Bubbly-and-champagne", "Life-is-lovely",
    "I-think-its-time", "To-blow-roof-off", "The-whole-place-uh",
    "Feels-like-im-Clark", "Transformin-into-Superman", "Bullet-proof",
    "No-one-can-stop", "The-rhyme-animal", "Lyrical-miracle",
    "Statistical-illogical", "Magical-miracle", "The-prophet-of-rap",
    "Lyrical-lunatic", "Futuristic-supernova", "Hyperactive-drastic",
    "Automatic-systematic", "Full-of-charm-and-charisma", "Drama-tic",
    "Ecstatic-kinetic", "Energy-eternal", "Inferno-burnin",
    "Eternal-returnin", "To-the-rap-game", "Like-I-never-left",
    "Ghost-writer-they-suspect", "But-theyll-never-know", "My-technique",
    "Is-so-unique", "Flow-so-sick", "Like-a-disease",
    "Infection-spreadin", "Through-the-speakers", "Virus-in-the-system",
    "Total-eclipse", "Of-the-rap-game", "Reign-supreme",
    "The-king-is-back", "Bow-down-to-the-crown", "No-contest",
    "Im-the-best", "Period-exclamation", "No-debate",
    "Fate-is-sealed", "Destiny-revealed", "Legacy-etched",
    "In-stone-forever", "Immortal-endeavor", "Eminem-the-legend"};

// ===== Runtime variables ===== //
uint8_t currentChannel = 0;
uint32_t lastChannelSwitch = 0;
uint8_t baseMAC[6];

void generateSSID(uint8_t index, char *buffer)
{
  // Generate a serial numbered SSID (001-100)
  const char *lyric = rapGodLyrics[index % (sizeof(rapGodLyrics) / sizeof(char *))];
  snprintf(buffer, 33, "%03d-%.27s", index + 1, lyric);
}

void generateMAC(uint8_t index, uint8_t *mac)
{
  // Generate a valid local management MAC address
  mac[0] = 0x02; // Locally managed address
  esp_fill_random(&mac[1], 5);
  mac[5] = (baseMAC[5] + index) % 0xFF;
}

void buildBeacon(uint8_t index)
{
  // Generate a dynamic SSID
  char ssid[33];
  generateSSID(index, ssid);
  uint8_t ssidLen = strlen(ssid);

  // Generate a virtual MAC
  uint8_t virtualMAC[6];
  generateMAC(index, virtualMAC);

  // Update Beacon frame
  memcpy(&beaconPacket[10], virtualMAC, 6);
  memcpy(&beaconPacket[16], virtualMAC, 6);
  beaconPacket[37] = ssidLen;         // SSID length
  memset(&beaconPacket[38], ' ', 32); // Clear the SSID area
  memcpy(&beaconPacket[38], ssid, ssidLen);
  beaconPacket[82] = channels[currentChannel]; // Current channel
}

void sendBeacons()
{
  for (int i = 0; i < MAX_SSIDS; i++)
  {
    buildBeacon(i);
    esp_wifi_80211_tx(WIFI_IF_AP, beaconPacket, 62 + 32, false); // Fixed length sending
    delayMicroseconds(150);                                      // Controlling the sending interval
  }
}

namespace MOONCAKE::APPS
{
  WiFiSpam::WiFiSpam(DEVICES *device)
      : AppAbility(), _device(device), currentLine(0)
  {
    setAppInfo().name = "WiFi Spam";
  }

  // Auxiliary method: Display text on the background image
  void WiFiSpam::displayTextOnBackground(const char *text, bool newLine)
  {
    if (!_device)
      return;

    // Set the text style - green foreground, transparent background
    _device->Lcd.setTextColor(TFT_GREEN); // Only set the foreground color, the background is transparent
    _device->Lcd.setFont(&fonts::efontCN_16);

    // Calculate display position
    int lineHeight = 18; // Font height + spacing
    int startY = 0;      // Starting Y position
    int startX = 10;     // Starting X position, leaving margin on the left

    if (newLine)
    {
      currentLine++;
      _device->Lcd.setCursor(startX, startY + (currentLine - 1) * lineHeight);
    }

    _device->Lcd.print(text);
  }

  // Clean up the display area
  void WiFiSpam::clearDisplayArea()
  {
    if (!_device)
      return;

    currentLine = 0;
    // Redisplay background image
    _device->Lcd.fillScreen(TFT_BLACK);
    _device->Lcd.pushImage(0, 0, wifi_killer.width, wifi_killer.height, (uint16_t *)wifi_killer.data);
  }

  void WiFiSpam::onOpen()
  {
    if (!_device)
    {
      return;
    }

    // show background
    clearDisplayArea();
    displayTextOnBackground("Starting...");
    delay(300);

    // Get the base MAC address
    esp_read_mac(baseMAC, ESP_MAC_WIFI_STA);
    baseMAC[0] = 0x02; // Set to local management address

    displayTextOnBackground("MAC init");
    delay(200);

    // WiFi Initialization
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_start();
    esp_wifi_set_channel(channels[currentChannel], WIFI_SECOND_CHAN_NONE);

    displayTextOnBackground("WiFi OK");
    delay(200);

    char buffer[64];
    sprintf(buffer, "Channel: %d", channels[currentChannel]);
    displayTextOnBackground(buffer);

    sprintf(buffer, "Fake SSIDs: %d", MAX_SSIDS);
    displayTextOnBackground(buffer);

    displayTextOnBackground("Running");
    displayTextOnBackground("Press B to exit");
  }

  void WiFiSpam::onRunning()
  {
    if (!_device)
    {
      return;
    }

    // Channel Switching
    if (millis() - lastChannelSwitch > CHANNEL_SWITCH_MS)
    {
      currentChannel = (currentChannel + 1) % 3;
      esp_wifi_set_channel(channels[currentChannel], WIFI_SECOND_CHAN_NONE);
      lastChannelSwitch = millis();
    }

    // Send Beacons in batches
    sendBeacons();

    // Status display - updates every 2 seconds to reduce flickering
    static uint32_t lastPrint = 0;
    static uint32_t packetsSent = 0;

    if (millis() - lastPrint > 2000)
    {
      packetsSent += MAX_SSIDS; // Send MAX_SSIDS packets each time

      // Partial update display information
      // Update only the dynamic information area
      int textStartY = 140; // Dynamic information display area
      int textHeight = 60;  // Dynamic information area height

      // Redraw the dynamic information area of ​​the background image
      _device->Lcd.pushImage(0, textStartY, wifi_killer.width, textHeight,
                             (uint16_t *)wifi_killer.data + (textStartY * wifi_killer.width));

      // Set text style
      _device->Lcd.setTextColor(TFT_GREEN);
      _device->Lcd.setFont(&fonts::efontCN_16);

      // Display dynamic information
      char buffer[64];
      _device->Lcd.setCursor(10, textStartY + 5);
      sprintf(buffer, "Channel: %d", channels[currentChannel]);
      _device->Lcd.print(buffer);

      _device->Lcd.setCursor(10, textStartY + 25);
      sprintf(buffer, "Packets: %d", packetsSent);
      _device->Lcd.print(buffer);

      _device->Lcd.setCursor(10, textStartY + 45);
      sprintf(buffer, "Free: %d KB", ESP.getFreeHeap() / 1024);
      _device->Lcd.print(buffer);

      lastPrint = millis();
    }
  }

  void WiFiSpam::onClose()
  {
    // Safe cleanup without UI
    try
    {
      esp_wifi_stop();
    }
    catch (...)
    {
    }
    try
    {
      esp_wifi_deinit();
    }
    catch (...)
    {
    }
    _device->Lcd.fillScreen(TFT_BLACK);
  }
}
