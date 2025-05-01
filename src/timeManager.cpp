#include "timeManager.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

#define NTP_TIMEZONE "UTC"
#define NTP_SERVER1 "0.pool.ntp.org"
#define NTP_SERVER2 "1.pool.ntp.org"
#define NTP_SERVER3 "2.pool.ntp.org"

// Different versions of the framework have different SNTP header file names and
// availability.
#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define SNTP_ENABLED 1
#elif __has_include(<sntp.h>)
#include <sntp.h>
#define SNTP_ENABLED 1
#endif

TimeManager::TimeManager()
{
}

const String TimeManager::getCurrentTime(const String Timezone)
{
  static constexpr const char* const wd[7] = {"Sun", "Mon", "Tue", "Wed",
    "Thr", "Fri", "Sat"};
  configTzTime(Timezone.c_str(), NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

#if SNTP_ENABLED
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
  {
    Serial.print('.');
    delay(1000);
  }
#else
  delay(1600);
  struct tm timeInfo;
  while (!getLocalTime(&timeInfo, 1000))
  {
    Serial.print('.');
  };
#endif

  Serial.println("\r\n NTP Connected.");
  auto t = time(nullptr);
  auto tm = gmtime(&t);  // for UTC.
  Serial.printf("ESP32 UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                wd[tm->tm_wday], tm->tm_hour, tm->tm_min, tm->tm_sec);
                // (HH:mm:ss.ssssssZ)
  String now = formatTmToISO8601(tm);
  //  String(tm->tm_year + 1900) + "-" +
  //        String(tm->tm_mon + 1) + "-" + String(tm->tm_mday) + "T" +
  //        String(tm->tm_hour) + ":" + String(tm->tm_min) + ":" +
  //        String(tm->tm_sec) + "Z";
         Serial.println("Current time: " + now);
        return now;

  /** @todo improve error handling */
  int16_t HTTP_Code{};
  String Output{};
  HTTPClient http;

  http.begin("https://timeapi.io/api/time/current/zone?timeZone=" + Timezone, timeapi_io_ca);

  HTTP_Code = http.GET();

  if (HTTP_Code >= 200 && HTTP_Code <= 226)
  {
    // StaticJsonDocument<46> filter;
    // filter["dateTime"] = true;

    const size_t capacity = JSON_OBJECT_SIZE(4);
    JsonDocument doc;

    deserializeJson(doc, http.getString());

    const String TMP_Str = doc["dateTime"];
    Output = formatISODate(TMP_Str);
    Serial.println("Current time: " + Output);
  }
  else
  {
    /* Error getting the current time from the time API. Do nothing */
  }

  http.end();
  return Output;
}

String TimeManager::formatISODate(const String &dateTime)
{
  int decimalPos = dateTime.indexOf('.');
  if (decimalPos != -1)
  {
    return dateTime.substring(0, decimalPos) + "Z";
  }
  // If no decimal point found, just append Z
  return dateTime + "Z";
}

String TimeManager::formatTmToISO8601(const tm* timeinfo) {
  char buffer[25];
  snprintf(buffer, sizeof(buffer), 
           "%04d-%02d-%02dT%02d:%02d:%02dZ",
           timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1,
           timeinfo->tm_mday,
           timeinfo->tm_hour,
           timeinfo->tm_min,
           timeinfo->tm_sec);
  return String(buffer);
}