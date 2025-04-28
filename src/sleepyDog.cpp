#include "sleepyDog.h"
#include <Arduino.h>



SleepyDog::SleepyDog(uint32_t i_timeToSleepMs)
{
  timeToSleepMs = i_timeToSleepMs;
}

SleepyDog::~SleepyDog()
{
}

void SleepyDog::feed()
{
  lastFeedTime = millis();
}

bool SleepyDog::isSleeping()
{
  if (millis() - lastFeedTime > timeToSleepMs)
  {
    return true;
  }
  else
  {
    return false;
  }
}
