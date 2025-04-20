#include "timeManager.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

TimeManager::TimeManager()
{
}

const String TimeManager::getCurrentTime(const String Timezone)
{
  /** @todo improve error handling */
  int16_t HTTP_Code{};
  String Output{};
  HTTPClient http;

  http.begin("https://timeapi.io/api/time/current/zone?timeZone=" + Timezone, timeapi_io_ca);

  HTTP_Code = http.GET();

  if (HTTP_Code >= 200 && HTTP_Code <= 226)
  {
    StaticJsonDocument<46> filter;
    filter["dateTime"] = true;

    const size_t capacity = JSON_OBJECT_SIZE(4);
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, http.getString(), DeserializationOption::Filter(filter));

    const String TMP_Str = doc["dateTime"];
    Output = formatISODate(TMP_Str);
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