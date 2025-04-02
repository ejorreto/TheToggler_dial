
#include "M5Dial.h"
#include "task.h"
#include "Toggl.h"

#include "credentials.h"

Task favouriteTasks[] = {
    Task(1, "Task 1"),
    Task(2, "Task 2"),
    Task(3, "Task 3"),
    Task(4, "Task 4"),
    Task(5, "Task 5")};

int numOfTasks = sizeof(favouriteTasks) / sizeof(favouriteTasks[0]);

Toggl toggl;

String formatISODate(const String &dateTime)
{
  int decimalPos = dateTime.indexOf('.');
  if (decimalPos != -1)
  {
    return dateTime.substring(0, decimalPos) + "Z";
  }
  // If no decimal point found, just append Z
  return dateTime + "Z";
}

void setup()
{
  int numRetries = 5;
  Serial.begin(115200);
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  M5Dial.Display.setTextColor(GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
  M5Dial.Display.setTextSize(0.75);
  M5Dial.Display.clear();
  M5Dial.Display.drawString("Wifi main",
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED && numRetries > 0)
  {
    WiFi.begin(mainWifiSSID, mainWifiPass);
    delay(1000);
    numRetries--;
  }
  numRetries = 5;
  if (WiFi.status() != WL_CONNECTED)
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Wifi fallback",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    delay(1000);
    while (WiFi.status() != WL_CONNECTED && numRetries > 0)
    {
      WiFi.begin(fallbackWifiSSID, fallbackWifiPass);
      delay(1000);
      numRetries--;
    }
  }

  toggl.setAuth(Token);
  if (WiFi.status() == WL_CONNECTED)
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Wifi conectada",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
  else
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Wifi no conectada",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
  delay(1000);
}

long oldPosition = -999;

void loop()
{
  M5Dial.update();
  long newPosition = M5Dial.Encoder.read();
  if (newPosition != oldPosition)
  {
    M5Dial.Speaker.tone(8000, 20);
    M5Dial.Display.clear();
    oldPosition = newPosition;
    Serial.println(newPosition);
    M5Dial.Display.drawString(favouriteTasks[((newPosition % numOfTasks) + numOfTasks) % numOfTasks].getDescription().c_str(),
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    // M5Dial.Display.drawString(String(newPosition),
    //                           M5Dial.Display.width() / 2,
    //                           M5Dial.Display.height() / 2);
  }
  TimeEntry newTimeEntry;
  if (M5Dial.BtnA.wasPressed())
  {
    // M5Dial.Encoder.readAndReset();

    String currentTime = "No time";

    if ((WiFi.status() == WL_CONNECTED))
    {
      Serial.println("---- Getting current entry");
      toggl.GetCurrentTimeEntry(&newTimeEntry);
      Serial.println("---- Stopping current entry");
      String httpCode = "";
      httpCode = toggl.StopTimeEntry(newTimeEntry);
      Serial.println(httpCode.c_str());
      Serial.println("Stopped time entry ID:");
      Serial.println(newTimeEntry.getId());
      currentTime = formatISODate(toggl.getCurrentTime("UTC"));
      Serial.println(currentTime.c_str());
      M5Dial.Display.clear();
      M5Dial.Speaker.tone(6000, 20);
      M5Dial.Display.drawString(currentTime.c_str(),
                                M5Dial.Display.width() / 2,
                                M5Dial.Display.height() / 2);
      Serial.println("---- Creating a new entry");

      String tags = "";
      String timeID = toggl.CreateTimeEntry(favouriteTasks[((newPosition % numOfTasks) + numOfTasks) % numOfTasks].getDescription().c_str(), tags, -1, currentTime.c_str(), 0, "TheToggler_dial", workspaceID, &newTimeEntry);
      Serial.println(timeID.c_str());
      Serial.println("New time entry ID:");
      Serial.println(newTimeEntry.getId());
    }
  }
  if (M5Dial.BtnA.pressedFor(5000))
  {
    M5Dial.Encoder.readAndReset();
    // M5Dial.Encoder.write(100);
    M5Dial.Display.drawString("Trackeando",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
}
