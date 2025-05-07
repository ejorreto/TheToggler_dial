#include "timeManager.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

#define NTP_SERVER1 "0.pool.ntp.org"
#define NTP_SERVER2 "1.pool.ntp.org"
#define NTP_SERVER3 "2.pool.ntp.org"

#include <esp_sntp.h> /* For ESP32-S3, other versions might use different headers */

TimeManager::TimeManager()
{
}

const String TimeManager::getCurrentTime(const String Timezone)
{
  configTzTime(Timezone.c_str(), NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
  {
    /** @todo Add a protection limit to avoid entering an infinite loop */
    Serial.print('.');
    delay(1000);
  }

  Serial.println("\r\n NTP Connected.");
  auto t = time(nullptr);
  /** @todo use something besides gmtime, that uses UTC by default and would be better allowing using a different time zone for flexibility */
  auto tm = gmtime(&t);

  String now = formatTmToISO8601(tm);
  Serial.println("Current time: " + now);

  return now;
}

String TimeManager::formatTmToISO8601(const tm *timeinfo)
{
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