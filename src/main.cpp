
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

void setup()
{
  Serial.begin(115200);
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  M5Dial.Display.setTextColor(GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
  M5Dial.Display.setTextSize(0.75);
  toggl.init(wifiSSID, wifiPass);
  toggl.setAuth(Token);
  Serial.println("Wifi conectada");
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
  if (M5Dial.BtnA.wasPressed())
  {
    // M5Dial.Encoder.readAndReset();
    M5Dial.Display.clear();
    M5Dial.Speaker.tone(6000, 20);
    M5Dial.Display.drawString("Trackeando",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    Serial.println(toggl.getWorkSpace().c_str());
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
