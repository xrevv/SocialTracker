#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "secret.h"      // uncomment if using secret.h file with credentials
// #define TWI_TIMEOUT 3000                  // varies depending on network speed (msec), needs to be before TwitterWebAPI.h
#include <TwitterWebAPI.h>

#include <InstagramStats.h>
#include <JsonStreamingParser.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Esp.h>
#include <math.h>

#include <EEPROM_Rotate.h>

EEPROM_Rotate EEPROMr;
#define DATA_OFFSET 10

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET LED_BUILTIN // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClientSecure client;
InstagramStats instaStats(client);

unsigned long timeNow;

unsigned long delayBetweenChecks = 3600000; // mean time between api requests
unsigned long whenDueToCheck = 0;

unsigned int Tfollowers = 0;
char Tfollowerz[10];
unsigned int IGfollowers = 0;
unsigned int bIGfollowers = 0;
char IGfollowerz[10];
String IP;
String serial;
bool check = 1;
bool screen = 1;
bool night = 0;
bool restart = 0;

int timeH;

// Inputs
String userName = "xrev_v"; // from instagram url

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// ------------------------------------------------------------------------------------------------

#ifndef WIFICONFIG
char ssid[] = secret_ssid;         // your network SSID (name)
char password[] = secret_password; // your network key
#endif

std::string search_str = "@xrev_v";         // Default search word for twitter
const char *ntp_server = "pool.ntp.org";    // time1.google.com, time.nist.gov, pool.ntp.org
int timezone = +1;                          // US Eastern timezone -05:00 HRS
unsigned long twi_update_interval = 60 * 1; // (seconds) minimum 5s (180 API calls/15 min). Any value less than 5 is ignored!
                                            // times minutes

#ifndef TWITTERINFO // Obtain these by creating an app @ https://apps.twitter.com/
static char const consumer_key[] = secret_consumer_key;
static char const consumer_sec[] = secret_consumer_sec;
static char const accesstoken[] = secret_accesstoken;
static char const accesstoken_sec[] = secret_accesstoken_sec;
#endif

// ICONS
// TIG logo
static const unsigned char PROGMEM iminiTIG[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFC, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7F, 0xF0, 0x00, 0x00, 0x03, 0xFC, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7F, 0xF8, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7D, 0x9C, 0x00, 0x00, 0x1C, 0x03, 0x80, 0x3E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x8E, 0x00, 0x00, 0x38, 0x01, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x87, 0x00, 0x00, 0x70, 0x00, 0xE3, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x83, 0x80, 0x00, 0xE0, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x81, 0xC0, 0x00, 0xC0, 0x00, 0x3C, 0x7F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x80, 0xF0, 0x01, 0xC0, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x80, 0x3C, 0x01, 0x80, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x80, 0x1F, 0x01, 0x80, 0x00, 0x03, 0xBF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x80, 0x07, 0xC1, 0x80, 0x00, 0x07, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0x80, 0x01, 0xFD, 0x80, 0x00, 0x1E, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFD, 0xC0, 0x00, 0x3F, 0x80, 0x00, 0x18, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0xC0, 0x00, 0x03, 0x80, 0x00, 0x18, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x60, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x60, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x70, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x30, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x38, 0x00, 0x00, 0x00, 0x00, 0x60, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x18, 0x00, 0x00, 0x00, 0x00, 0x60, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x1C, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x06, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x07, 0x00, 0x00, 0x00, 0x01, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x03, 0x80, 0x00, 0x00, 0x03, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x01, 0xC0, 0x00, 0x00, 0x03, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0xE0, 0x00, 0x00, 0x06, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x70, 0x00, 0x00, 0x0E, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x38, 0x00, 0x00, 0x1C, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x1C, 0x00, 0x00, 0x38, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x0E, 0x00, 0x00, 0x70, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x3C, 0x00, 0x00, 0xE0, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0xF8, 0x00, 0x01, 0xC0, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x03, 0xE0, 0x00, 0x03, 0x80, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFC, 0x3F, 0x80, 0x00, 0x0F, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFC, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7F, 0xC0, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7F, 0xE0, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0xFF, 0xFC, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x83, 0xFF, 0xC0, 0x00, 0x00, 0x01, 0xFC, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};

// Follower icon
static const unsigned char PROGMEM iInstagram[] = {
    0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF8, 0x3C, 0x00, 0x00, 0x3C,
    0x78, 0x00, 0x00, 0x1E, 0x70, 0x00, 0x00, 0x8E, 0x60, 0x00, 0x01, 0xC6, 0x60, 0x00, 0x01, 0xE6,
    0x60, 0x07, 0xE1, 0xC6, 0x60, 0x1F, 0xF8, 0x06, 0x60, 0x3F, 0xFC, 0x06, 0x60, 0x78, 0x1E, 0x06,
    0x60, 0x70, 0x0E, 0x06, 0x60, 0xE0, 0x07, 0x06, 0x60, 0xE0, 0x07, 0x06, 0x60, 0xE0, 0x07, 0x06,
    0x60, 0xE0, 0x07, 0x06, 0x60, 0xE0, 0x07, 0x06, 0x60, 0xE0, 0x07, 0x06, 0x60, 0x70, 0x0E, 0x06,
    0x60, 0x78, 0x1E, 0x06, 0x60, 0x3F, 0xFC, 0x06, 0x60, 0x1F, 0xF8, 0x06, 0x60, 0x07, 0xE0, 0x06,
    0x60, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x06, 0x70, 0x00, 0x00, 0x0E, 0x78, 0x00, 0x00, 0x1E,
    0x3C, 0x00, 0x00, 0x3C, 0x1F, 0xFF, 0xFF, 0xF8, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00};

// Twitter icon
static const unsigned char PROGMEM iTwitter[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x0C, 0x00,
    0x0E, 0x00, 0x3F, 0x00, 0x0F, 0x00, 0x7F, 0x80, 0x0F, 0x80, 0xFF, 0xFC, 0x0F, 0xC0, 0xFF, 0xF8,
    0x0F, 0xF8, 0xFF, 0xF0, 0x0F, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xC0, 0x0F, 0xFF, 0xFF, 0xC0,
    0x0F, 0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xC0, 0x03, 0xFF, 0xFF, 0x80,
    0x01, 0xFF, 0xFF, 0x80, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x7F, 0xFE, 0x00,
    0x00, 0x3F, 0xFC, 0x00, 0x01, 0xFF, 0xF8, 0x00, 0x3F, 0xFF, 0xE0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
    0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

String toStringIp(IPAddress ip)
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

unsigned long api_mtbs = twi_update_interval * 1000; // mean time between api requests
unsigned long api_lasttime = 0;
bool twit_update = false;
std::string search_msg = "No Message Yet!";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntp_server, timezone * 3600, 60000); // NTP server pool, offset (in seconds), update interval (in milliseconds)
TwitterClient tcr(timeClient, consumer_key, consumer_sec, accesstoken, accesstoken_sec);

bool eWrite(unsigned int x)
{
  uint8_t digits[5];

  for (int i = 0; i < 5; i++)
  {
    digits[i] = x % 10; // remainder of division with 10 gets the last digit
    x /= 10;            // dividing by ten chops off the last digit.
    EEPROMr.write((i + 1) * DATA_OFFSET, digits[i]);
  }

  if (EEPROMr.commit())
  {
    Serial.println("EEPROM successfully committed");
    return 1;
  }
  else
  {
    Serial.println("ERROR! EEPROM commit failed");
    return 0;
  }
}

unsigned int eRead()
{
  unsigned int data = 0;
  uint8_t digits[5];
  for (int i = 0; i < 5; i++)
  {
    digits[i] = EEPROMr.read((i + 1) * DATA_OFFSET);
    data += digits[i] * pow(10, i);
  }
  Serial.println();

  return data;
}

void getInstagramStatsForUser()
{
  Serial.println("Getting instagram user stats for " + userName);
  InstagramUserStats response = instaStats.getUserStats(userName);
  Serial.println("Response:");
  Serial.print("Number of followers: ");
  Serial.println(response.followedByCount);
  serial += String(timeClient.getFormattedTime() + " - " + "Instagram: " + String(response.followedByCount) + "\n");
  IGfollowers = response.followedByCount;
  if (IGfollowers)
  {
    bIGfollowers = IGfollowers;
    eWrite(bIGfollowers);
  }
  itoa(bIGfollowers, IGfollowerz, 10);
}

void print()
{
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(WHITE);

  if (bIGfollowers)
  {
    display.drawBitmap(0, 0, iInstagram, 32, 32, 1);
    display.setCursor(32, 4);
    display.println(bIGfollowers);
  }
  else
  {
    display.setTextSize(2);
    display.setCursor(16, 0);
    display.println(timeClient.getFormattedTime());
    display.setTextSize(3);
  }

  display.drawBitmap(0, 32, iTwitter, 32, 32, 1);
  display.setCursor(32, 36);
  display.println(Tfollowers);

  display.display();
}

void extractJSON(String tmsg)
{
  const char *msg2 = const_cast<char *>(tmsg.c_str());
  const size_t bufferSize = 5 * JSON_ARRAY_SIZE(0) + 4 * JSON_ARRAY_SIZE(1) + 3 * JSON_ARRAY_SIZE(2) + 3 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 8 * JSON_OBJECT_SIZE(3) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2 * JSON_OBJECT_SIZE(10) + JSON_OBJECT_SIZE(24) + JSON_OBJECT_SIZE(43) + 6060;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject &response = jsonBuffer.parseObject(msg2);

  if (!response.success())
  {
    Serial.println("Failed to parse JSON!");
    Serial.println(msg2);
    return;
  }

  String namet = response["name"];
  String followers_count = response["followers_count"];
  String text = namet + " has " + followers_count + " followers.";
  search_msg = std::string(text.c_str(), text.length());

  jsonBuffer.clear();
  delete[] msg2;
}

void extractTweetText(String tmsg)
{
  Serial.print("Recieved Message Length");
  long msglen = tmsg.length();
  Serial.print(": ");
  Serial.println(msglen);
  if (msglen <= 32)
    return;

  String searchstr = ",\"name\":\"";
  unsigned int searchlen = searchstr.length();
  int pos1 = -1, pos2 = -1;
  for (long i = 0; i <= msglen - searchlen; i++)
  {
    if (tmsg.substring(i, searchlen + i) == searchstr)
    {
      pos1 = i + searchlen;
      break;
    }
  }
  searchstr = "\",\"";
  searchlen = searchstr.length();
  for (long i = pos1; i <= msglen - searchlen; i++)
  {
    if (tmsg.substring(i, searchlen + i) == searchstr)
    {
      pos2 = i;
      break;
    }
  }
  String text = tmsg.substring(pos1, pos2);

  searchstr = ",\"followers_count\":";
  searchlen = searchstr.length();
  int pos3 = -1, pos4 = -1;
  for (long i = pos2; i <= msglen - searchlen; i++)
  {
    if (tmsg.substring(i, searchlen + i) == searchstr)
    {
      pos3 = i + searchlen;
      break;
    }
  }
  searchstr = ",";
  searchlen = searchstr.length();
  for (long i = pos3; i <= msglen - searchlen; i++)
  {
    if (tmsg.substring(i, searchlen + i) == searchstr)
    {
      pos4 = i;
      break;
    }
  }
  String usert = tmsg.substring(pos3, pos4);

  if (text.length() > 0)
  {
    text = text + " has " + usert + " followers.";
    search_msg = std::string(text.c_str(), text.length());
  }
  usert.toCharArray(Tfollowerz, sizeof(Tfollowerz));
  Tfollowers = usert.toInt();
  serial += String(String(timeClient.getFormattedTime()) + " - " + "Twitter: " + usert + "\n");
}

void Twitter(void)
{
  extractTweetText(tcr.searchUser(search_str));
  Serial.print("Search: ");
  Serial.println(search_str.c_str());
  Serial.print("MSG: ");
  Serial.println(search_msg.c_str());
  api_lasttime = millis();
}

void setup(void)
{
  // Setup internal LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Begin Serial
  Serial.begin(115200);
  Serial.println();

  // Begin EEPROM
  EEPROMr.size(4);
  EEPROMr.begin(4096);

  bIGfollowers = eRead();
  Serial.print("EEPROM loaded - IG = ");
  Serial.println(bIGfollowers);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  // Print logo
  display.clearDisplay();
  display.drawBitmap(0, 0, iminiTIG, 128, 64, 1);
  display.display();

  // WiFi Connection
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to ");
  Serial.print(ssid);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected. yay!");
  Serial.print("IP address: ");
  IPAddress ip = WiFi.localIP();
  IP = toStringIp(ip);
  Serial.println(ip);

  // Print IP in the screen
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(ip);
  display.display();

  delay(100);
  // Connect to NTP and force-update time
  tcr.startNTP();
  Serial.println("NTP Synced");
  delay(100);

  if (twi_update_interval < 5)
    api_mtbs = 5000; // Cant update faster than 5s.

  server.begin();

  delay(1000);

  // If using ESP8266 Core 2.5 RC, uncomment the following
  client.setInsecure();

  Twitter();
}

void loop()
{

  timeNow = millis();

  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {                                // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /restart") >= 0)
            {
              restart = 1;
              client.println("<meta http-equiv='refresh' content='0; URL=http://" + IP + "'>");
            }
            if (header.indexOf("GET /log") >= 0)
            {
              serial += String(timeClient.getFormattedTime() + " - " + "Checking LOGs" + "\n");
              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<title>SocialTracker for " + userName + "</title>");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the buttons
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; color: white;}");
              client.println("h1 {text-align: center;}");
              client.println("h2 {text-align: center;}");
              client.println("p {text-align: center;}");
              client.println("div {text-align: center;}");
              client.println(".button { background-color: #E50914; border: none; color: white; width: 120px; height: 60px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #E50914; border: none; color: white; width: 35%; height: 60px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println("</style></head>");

              // Web Page Heading
              client.println("<body style=\"background-color:black;\">");
              client.println("<h1>SocialTracker for " + userName + "</h1>");
              client.println("<p><h2><pre>" + serial + "</pre></h2></p>");
              client.println("<p><h1><a href=\"/\"><button class=\"button2\">BACK</button></a></h1></p>");
              client.println("<p><h1><a href=\"/restart\"><button class=\"button2\">RESTART</button></a></h1></p>");

              client.println("</body></html>");
            }
            else
            {
              // Turns the screen on and off + check
              if (header.indexOf("GET /screen/on") >= 0)
              {
                serial += String(timeClient.getFormattedTime() + " - " + "Screen ON" + "\n");
                Serial.println("Screen on");
                screen = 1;
                print();
                client.println("<meta http-equiv='refresh' content='0; URL=http://" + IP + "'>");
              }
              else if (header.indexOf("GET /screen/off") >= 0)
              {
                serial += String(timeClient.getFormattedTime() + " - " + "Screen OFF" + "\n");
                Serial.println("Screen off");
                screen = 0;
                display.clearDisplay();
                display.display();
                client.println("<meta http-equiv='refresh' content='0; URL=http://" + IP + "'>");
              }
              else if (header.indexOf("GET /update") >= 0)
              {
                serial += String(timeClient.getFormattedTime() + " - " + "Update" + "\n");
                timeNow = millis();
                getInstagramStatsForUser();
                whenDueToCheck = timeNow + delayBetweenChecks;
                if (screen == 1)
                {
                  print();
                }
                client.println("<meta http-equiv='refresh' content='0; URL=http://" + IP + "'>");
              }
              else if (header.indexOf("GET /check/on") >= 0)
              {
                serial += String(timeClient.getFormattedTime() + " - " + "Check ON" + "\n");
                check = 1;
                client.println("<meta http-equiv='refresh' content='0; URL=http://" + IP + "'>");
              }
              else if (header.indexOf("GET /check/off") >= 0)
              {
                serial += String(timeClient.getFormattedTime() + " - " + "Check OFF" + "\n");
                check = 0;
                client.println("<meta http-equiv='refresh' content='0; URL=http://" + IP + "'>");
              }

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<title>SocialTracker for " + userName + "</title>");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // CSS to style the buttons
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; color: white;}");
              client.println("h1 {text-align: center;}");
              client.println("p {text-align: center;}");
              client.println("div {text-align: center;}");
              client.println(".button { background-color: #E50914; border: none; color: white; width: 120px; height: 60px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #E50914; border: none; color: white; width: 35%; height: 60px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println("</style></head>");

              // Web Page Heading
              client.println("<body style=\"background-color:black;\">");
              client.println("<h1>SocialTracker for " + userName + "</h1>");
              client.println("<h1><p>");

              // Table
              client.println("<table style="
                             "margin-left:auto;margin-right:auto;border-spacing:5px;"
                             ">");
              client.println("<thead>");
              client.println("<tr>");
              client.println("<th>Instagram</th>");
              client.println("<th>Twitter</th>");
              client.println("</tr>");
              client.println("</thead>");
              client.println("<tbody>");
              client.println("<tr>");
              client.println("<td>");
              client.println(IGfollowerz);
              client.println("</td>");
              client.println("<td>");
              client.println(Tfollowerz);
              client.println("</td>");
              client.println("</tr>");
              client.println("</tbody>");
              client.println("</table>");

              client.println("</p></h1>");

              client.println("<table style="
                             "margin-left:auto;margin-right:auto;border-spacing:5px;"
                             ">");
              client.println("<thead>");
              client.println("<tr>");
              client.println("<th>");
              // Display current state, and ON/OFF buttons for the screen
              client.println("<h1><p>Screen</p></h1>");
              // If the screen is off, it displays the ON button
              if (screen == 1)
              {
                client.println("<p><a href=\"/screen/off\"><button class=\"button\">ON</button></a></p>");
              }
              else
              {
                client.println("<p><a href=\"/screen/on\"><button class=\"button\">OFF</button></a></p>");
              }
              client.println("</th>");
              client.println("<th>");
              unsigned long minutes = (whenDueToCheck - timeNow) / 60000;
              unsigned long seconds = (whenDueToCheck - timeNow - (minutes * 60000)) / 1000;
              client.println("<h1><p>Check </p></h1>");

              if (check == 1)
              {
                client.println("<p><a href=\"/check/off\"><button class=\"button\">ON</button></a></p>");
              }
              else
              {
                client.println("<p><a href=\"/check/on\"><button class=\"button\">OFF</button></a></p>");
              }

              client.println("</th>");
              client.println("</tr>");
              client.println("</thead>");
              client.println("</table>");

              if (check == 1)
              {
                client.println("<p><h1>");
                client.println(minutes);
                client.println(":");
                client.println(seconds);
                client.println("</p></h1>");
              }

              client.println("<p><h1><a href=\"/update\"><button class=\"button2\">UPDATE</button></a></h1></p>");
              client.println("<p><h1><a href=\"/log\"><button class=\"button2\">LOG</button></a></h1></p>");

              client.println("</body></html>");

              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            }
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  timeNow = millis();

  if (restart && (timeNow - millis() >= 10000))
  {
    Serial.println("Restarting");
    ESP.restart();
  }

  if (check == 1)
  {
    if (timeNow > api_lasttime + api_mtbs)
    {
      Twitter();
    }

    if (timeNow > whenDueToCheck)
    {
      getInstagramStatsForUser();
      whenDueToCheck = timeNow + delayBetweenChecks;
    }
  }
  else
  {
    whenDueToCheck = timeNow + delayBetweenChecks;
  }

  if (screen == 1)
  {
    print();
  }

  timeH = timeClient.getHours();

  // ? comment if - PC POWER

  if (timeH == 8 && night == 1)
  {
    check = 1;
    screen = 1;
    serial += String(timeClient.getFormattedTime() + " - " + "Good morning" + "\n");
    getInstagramStatsForUser();
    night = 0;
  }
  if (timeH == 0 && night == 0)
  {
    check = 0;
    screen = 0;
    display.clearDisplay();
    display.display();
    serial += String(timeClient.getFormattedTime() + " - " + "Goodnight" + "\n");
    night = 1;
  }
}