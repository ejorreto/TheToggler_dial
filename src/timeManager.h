#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>

class TimeManager
{
private:

  /**
   * @brief Convert tm structure to ISO 8601 format
   * 
   * @param timeinfo Pointer to tm structure
   * @return String Time in ISO 8601 format 2025-05-01T06:08:29Z
   */
  String formatTmToISO8601(const tm* timeinfo);

public:
/**
 * @brief Construct a new Time Manager object
 * 
 */
  TimeManager();

  /**
   * @brief Get the current time in ISO 8601 format (HH:mm:ss.ssssssZ)
   * 
   * @param Timezone Timezone string (e.g., "UTC", "Europe/Madrid"). Has to be "UTC" for Toggl API
   * @return const String 
   */
  const String getCurrentTime(const String Timezone);
};

#endif // TIMEMANAGER_H
