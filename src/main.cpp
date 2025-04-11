
#include "M5Dial.h"
#include "task.h"
#include "Toggl.h"

#include "credentials.h"
#include "timeManager.h"

#include "StateMachine.h"

/* Global variables */
const int STATE_DELAY = 1000;
StateMachine machine = StateMachine();
Toggl toggl;
TimeManager timeManager;
long oldPosition = -999;

Task favouriteTasks[] = {
  Task(0, "Change workspace", 0),
  Task(1, "Stop tracking", 0),
  Task(2, "Reu I", projectOneId),
  Task(3, "Reu D", projectTwoId),
  Task(4, "General", projectTwoId),
  Task(5, "Task 4", projectTwoId),
  Task(6, "Task 5", projectTwoId)};

int numOfTasks = sizeof(favouriteTasks) / sizeof(favouriteTasks[0]);

/* Functions declaration */

// WiFi connection
bool wifiConnect();

// State machine functions
void stateWorkplaceSelection();
void stateTimeEntrySelection();

/* State machine configuration */
State *S0 = machine.addState(&stateWorkplaceSelection);
State *S1 = machine.addState(&stateTimeEntrySelection);
State *nextState = nullptr;

/**
 * @brief Workplace selection state function
 * 
 */
void stateWorkplaceSelection()
{
  if (machine.executeOnce)
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Select workplace",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
  if (M5Dial.BtnA.wasPressed())
  {
    Serial.println("---- Workplace selected");
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Workplace selected",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    delay(1000);
    nextState = S1;
  }
}

/**
 * @brief Time entry selection state function
 * 
 */
void stateTimeEntrySelection()
{
  if (machine.executeOnce)
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Select time entry",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }

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
  }

  TimeEntry newTimeEntry;
  if (M5Dial.BtnA.wasPressed())
  {
    String currentTime = "No time";

    if ((WiFi.status() != WL_CONNECTED))
    {
      wifiConnect();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
      int index = ((newPosition % numOfTasks) + numOfTasks) % numOfTasks;
      if(index==0)
      {
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Change workspace",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        delay(1000);
        nextState = S0;
      }
      else if (index == 1)
      {
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Stopping",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        Serial.println("---- Getting current entry");
        toggl.GetCurrentTimeEntry(&newTimeEntry);
        if (newTimeEntry.getId() == 0)
        {
          M5Dial.Display.clear();
          M5Dial.Display.drawString("Nothing to stop",
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
        }
        else
        {
          Serial.println("---- Stopping current entry");
          String httpCode = "";
          httpCode = toggl.StopTimeEntry(newTimeEntry);
          Serial.println(httpCode.c_str());
          Serial.println("Stopped time entry ID:");
          Serial.println(newTimeEntry.getId());
          M5Dial.Display.clear();
          std::string msg = "Stopped\r\n" + newTimeEntry.getDescription();
          M5Dial.Display.drawString(msg.c_str(),
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
          M5Dial.Encoder.readAndReset();
        }
      }
      else
      {
        String currentTime = timeManager.getCurrentTime("UTC");
        if (currentTime.length() > 1)
        {
          Serial.println(currentTime.c_str());

          M5Dial.Display.clear();
          M5Dial.Speaker.tone(6000, 20);
          M5Dial.Display.drawString(currentTime.c_str(),
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
          Serial.println("---- Creating a new entry");

          String tags = "";
          String timeID = toggl.CreateTimeEntry(favouriteTasks[index].getDescription().c_str(), tags, -1, currentTime.c_str(), favouriteTasks[index].getProjectId(), "TheToggler_dial", workspaceID, &newTimeEntry);
          Serial.println(timeID.c_str());
          Serial.println("New time entry ID:");
          Serial.println(newTimeEntry.getId());
          M5Dial.Display.clear();
          M5Dial.Speaker.tone(6000, 20);
          M5Dial.Display.drawString(newTimeEntry.getDescription().c_str(),
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
        }
        else
        {
          M5Dial.Display.clear();
          M5Dial.Display.drawString("No time",
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
        }
      }
    }
  }
}


/**
 * @brief Try to connect to the main WiFi network. If it fails, try to connect to the fallback network.
 * 
 * @return bool Wifi connection status
 */
bool wifiConnect()
{
  int numRetries = 5;
  M5Dial.Display.clear();
  M5Dial.Display.drawString("Connecting",
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2);
  while (WiFi.status() != WL_CONNECTED && numRetries > 0)
  {
    WiFi.begin(mainWifiSSID, mainWifiPass);
    delay(1000);
    numRetries--;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    numRetries = 5;
    while (WiFi.status() != WL_CONNECTED && numRetries > 0)
    {
      WiFi.begin(fallbackWifiSSID, fallbackWifiPass);
      delay(1000);
      numRetries--;
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Wifi ON",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
  else
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Wifi OFF",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
  delay(1000);
  return WiFi.status() == WL_CONNECTED;
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

  delay(1000);
  wifiConnect();

  toggl.setAuth(Token);
}

void loop()
{
  M5Dial.update();
  // The following way to transition to a new state is required as per https://github.com/jrullan/StateMachine/issues/13
  if (nextState)
  {
    machine.transitionTo(nextState);
    nextState = nullptr;
  }
  machine.run();
}
