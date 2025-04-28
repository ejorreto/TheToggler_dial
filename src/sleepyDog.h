#ifndef SLEEPY_DOG_H
#define SLEEPY_DOG_H

#include <stdint.h>

class SleepyDog
{
private:
  /* data */
  uint32_t timeToSleepMs;
  uint32_t lastFeedTime;

public:
  SleepyDog(uint32_t i_timeToSleepMs = 10000);
  ~SleepyDog();
  /**
   * @brief Keep the dog awake by feeding it
   * 
   */
  void feed();

  /** 
   * @brief Check if the dog is sleeping
   */
  bool isSleeping();
};

#endif