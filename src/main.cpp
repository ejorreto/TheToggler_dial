
#include "M5Dial.h"
#include "task.h"
#include "Toggl.h"

#include "credentials.h"
#include "timeManager.h"

#include "StateMachine.h"
#include <ArduinoJson.h>

#define MAX_NUM_WORKSPACES 10

typedef struct workspaceAndTasks
{
  int workspaceId;
  Task *tasks;
  uint8_t numOfTasks;
} workspaceAndTasks;

workspaceAndTasks registeredWorkspaces[MAX_NUM_WORKSPACES] = {0};

/* Global variables */
const int STATE_DELAY = 1000;
StateMachine machine = StateMachine();
Toggl toggl;
TimeManager timeManager;
long oldPosition = -999;

Task workTasks[] = {
    Task(0, "Change workspace", 0),
    Task(1, "Stop tracking", 0),
    Task(2, "Reu I", projectOneId),
    Task(3, "Reu D", projectTwoId),
    Task(4, "General", projectTwoId),
    Task(5, "Task 4", projectTwoId),
    Task(6, "Task 5", projectTwoId)};

Task personalTasks[] = {
    Task(0, "Change workspace", 0),
    Task(1, "Stop tracking", 0),
    Task(2, "Design", projectThreeId),
    Task(3, "Implementation", projectThreeId)};

void setupTasksMap()
{
  registeredWorkspaces[0].workspaceId = workspaceWorkId;
  registeredWorkspaces[0].tasks = workTasks;
  registeredWorkspaces[0].numOfTasks = sizeof(workTasks) / sizeof(workTasks[0]);
  registeredWorkspaces[1].workspaceId = workspacePersonalId;
  registeredWorkspaces[1].tasks = personalTasks;
  registeredWorkspaces[1].numOfTasks = sizeof(personalTasks) / sizeof(personalTasks[0]);
}

/* Functions declaration */

// WiFi connection
bool wifiConnectJSON();

// State machine functions
void stateWorkplaceSelection();
void stateTimeEntrySelection();

/* State machine configuration */
State *S0 = machine.addState(&stateWorkplaceSelection);
State *S1 = machine.addState(&stateTimeEntrySelection);
State *nextState = nullptr;

/* Workspaces */
Workspace workspaces[MAX_NUM_WORKSPACES];
uint32_t receivedWorkspaces = 0;
int registeredWorkspaceIndex = -1;

/**
 * @brief Workplace selection state function
 *
 */
void stateWorkplaceSelection()
{
  togglApiErrorCode_t errorCode = TOGGL_API_EC_OK;
  if (machine.executeOnce)
  {
    /* This will be executed only when entering the state */
    if ((WiFi.status() != WL_CONNECTED))
    {
      wifiConnectJSON();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      errorCode = toggl.getWorkSpaces(workspaces, MAX_NUM_WORKSPACES, &receivedWorkspaces);

      if (errorCode != TOGGL_API_EC_OK)
      {
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Error " + String(errorCode),
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        /** @todo Display a more comprehensive error in the screen */
        delay(1000);
        nextState = S0;
      }
      else
      {
        if (receivedWorkspaces == 0)
        {
          M5Dial.Display.clear();
          M5Dial.Display.drawString("No workspaces",
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
        }
        else
        {
          M5Dial.Display.clear();
          M5Dial.Display.drawString("Select workplace",
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
        }
      }
    }
  }

  /* This will be executed cyclically while in this state */
  if (receivedWorkspaces > 0)
  {
    long newPosition = M5Dial.Encoder.read();

    if (newPosition != oldPosition)
    {
      M5Dial.Speaker.tone(8000, 20);
      M5Dial.Display.clear();
      oldPosition = newPosition;
      Serial.println(newPosition);
      M5Dial.Display.drawString(workspaces[((newPosition % receivedWorkspaces) + receivedWorkspaces) % receivedWorkspaces].getName().c_str(),
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
      /* Lets find the tasks for the selected workspace */
      for (int i = 0; i < MAX_NUM_WORKSPACES; i++)
      {
        if (registeredWorkspaces[i].workspaceId == workspaces[((newPosition % receivedWorkspaces) + receivedWorkspaces) % receivedWorkspaces].getId())
        {
          registeredWorkspaceIndex = i;
          Serial.println("Using registered workspace: " + String(registeredWorkspaces[i].workspaceId));
          break;
        }
      }
      delay(1000);
      nextState = S1;
    }
  }
}

/**
 * @brief Time entry selection state function
 *
 */
void stateTimeEntrySelection()
{
  int numOfTasks = registeredWorkspaces[registeredWorkspaceIndex].numOfTasks;
  Task *selectedTasks = registeredWorkspaces[registeredWorkspaceIndex].tasks;
  /** @todo Add protection in case none of the received workspaces is in the registeredWorkspaces map */

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

    M5Dial.Display.drawString(selectedTasks[((newPosition % numOfTasks) + numOfTasks) % numOfTasks].getDescription().c_str(),
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }

  if (M5Dial.BtnA.wasPressed())
  {
    M5Dial.Speaker.tone(6000, 20);
    String currentTime = "No time";

    if ((WiFi.status() != WL_CONNECTED))
    {
      wifiConnectJSON();
    }

    if ((WiFi.status() == WL_CONNECTED))
    {
      int index = ((newPosition % numOfTasks) + numOfTasks) % numOfTasks;
      if (index == 0)
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
        TimeEntry currentTimeEntry;
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Stopping",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        Serial.println("---- Getting current entry");
        togglApiErrorCode_t errorCode = toggl.GetCurrentTimeEntry(&currentTimeEntry);
        if (errorCode == TOGGL_API_EC_NO_CURRENT_TIME_ENTRY)
        {
          Serial.println("No current time entry");
          M5Dial.Display.clear();
          M5Dial.Display.drawString("No current entry",
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
          delay(1000);
        }
        else if (errorCode != TOGGL_API_EC_OK)
        {
          /* There is no entry currently running */
          M5Dial.Display.clear();
          M5Dial.Display.drawString("Error get: " + String(errorCode),
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
        }
        else
        {
          Serial.println("---- Stopping current entry");
          togglApiErrorCode_t errorCode = TOGGL_API_EC_OK;
          errorCode = toggl.StopTimeEntry(currentTimeEntry);
          if (errorCode != TOGGL_API_EC_OK)
          {
            M5Dial.Display.clear();
            M5Dial.Display.drawString("Error stop:" + String(errorCode),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2);
            delay(1000);
            nextState = S1;
          }
          else
          {
            Serial.println("Stopped time entry ID:");
            Serial.println(currentTimeEntry.getId());
            M5Dial.Display.clear();
            std::string msg = "Stopped\r\n" + currentTimeEntry.getDescription();
            M5Dial.Display.drawString(msg.c_str(),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2);
            M5Dial.Encoder.readAndReset();
          }
        }
      }
      else
      {
        TimeEntry newTimeEntry;
        /* Lets create a new time entry */
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

          Serial.println("Description: " + String(selectedTasks[index].getDescription().c_str()));
          Serial.println("Project ID: " + String(selectedTasks[index].getProjectId()));
          Serial.println("Workspace ID: " + String(registeredWorkspaces[registeredWorkspaceIndex].workspaceId));

          togglApiErrorCode_t errorCode = toggl.CreateTimeEntry(selectedTasks[index].getDescription().c_str(), tags, -1, currentTime.c_str(), selectedTasks[index].getProjectId(), "TheToggler_dial",
                                                                registeredWorkspaces[registeredWorkspaceIndex].workspaceId, &newTimeEntry);
          if (errorCode == TOGGL_API_EC_OK)
          {
            M5Dial.Display.clear();
            M5Dial.Speaker.tone(6000, 20);
            M5Dial.Display.drawString(newTimeEntry.getDescription().c_str(),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2);
          }
          else
          {
            M5Dial.Display.clear();
            M5Dial.Display.drawString("Error create: " + String(errorCode),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2);
          }
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
 * @brief Connect to wifi using the credentials in the JSON configuration string. It will retry a number of times on each wifi until connected.
 *
 * @return true if connected
 * @return false if not connected
 */

bool wifiConnectJSON()
{
  int numRetries = 5;
  M5Dial.Display.clear();
  M5Dial.Display.drawString("Connecting",
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2);
  JsonDocument doc;
  DeserializationError jsonErrorCode = deserializeJson(doc, settingsJson);
  if (jsonErrorCode != DeserializationError::Ok)
  {
    doc.clear();
    Serial.println("Error deserializing JSON: " + String(jsonErrorCode.c_str()));
  }
  else
  {
    // serializeJsonPretty(doc, Serial); // for debugging
    JsonArray data = doc["thetoggler"]["network"].as<JsonArray>();
    Serial.println("Number of wifi networks configured: " + String(data.size()));
    if (data.size() == 0)
    {
      doc.clear();
      M5Dial.Display.clear();
      M5Dial.Display.drawString("No wifi settings",
                                M5Dial.Display.width() / 2,
                                M5Dial.Display.height() / 2);
      delay(1000);
    }
    else
    {
      for (JsonVariant item : data)
      {
        numRetries = 5;

        while (WiFi.status() != WL_CONNECTED && numRetries > 0)
        {
          Serial.println("Trying wifi: " + String(item["ssid"].as<String>().c_str()));
          WiFi.begin(item["ssid"].as<String>().c_str(), item["password"].as<String>().c_str());
          delay(2000);
          numRetries--;
        }
      }
      doc.clear();
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
  wifiConnectJSON();

  toggl.setAuth(Token);

  setupTasksMap();
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
