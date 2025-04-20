
#include "M5Dial.h"
#include "task.h"
#include "Toggl.h"

#include "credentials.h"
#include "timeManager.h"

#include "StateMachine.h"
#include <ArduinoJson.h>

#define MAX_NUM_WORKSPACES 10
#define MAX_ENTRIES_PER_WORKSPACE 20

typedef struct workspaceEntries_t
{
  int workspaceId;
  Task entries[MAX_ENTRIES_PER_WORKSPACE];
  uint8_t numOfEntries;
} workspaceEntries_t;

workspaceEntries_t workspaceEntries[MAX_NUM_WORKSPACES];
uint8_t numWorkspacesConfigured = 0;

/* Global variables */
const int STATE_DELAY = 1000;
StateMachine machine = StateMachine();
Toggl toggl;
TimeManager timeManager;
long oldPosition = -999;

/* Functions declaration */

// WiFi connection
bool wifiConnectJSON();

// Workspaces and entries from JSON
bool readEntriesJSON();

// State machine functions
void stateWorkplaceSelection();
void stateTimeEntrySelection();

/* State machine configuration */
State *S0 = machine.addState(&stateWorkplaceSelection);
State *S1 = machine.addState(&stateTimeEntrySelection);
State *nextState = nullptr;

/* Workspaces */
Workspace receivedWorkspaces[MAX_NUM_WORKSPACES];
uint32_t numReceivedWorkspaces = 0;
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
      errorCode = toggl.getWorkSpaces(receivedWorkspaces, MAX_NUM_WORKSPACES, &numReceivedWorkspaces);

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
        if (numReceivedWorkspaces == 0)
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
  if (numReceivedWorkspaces > 0)
  {
    long newPosition = M5Dial.Encoder.read();

    if (newPosition != oldPosition)
    {
      M5Dial.Speaker.tone(8000, 20);
      M5Dial.Display.clear();
      oldPosition = newPosition;
      Serial.println(newPosition);
      M5Dial.Display.drawString(receivedWorkspaces[((newPosition % numReceivedWorkspaces) + numReceivedWorkspaces) % numReceivedWorkspaces].getName().c_str(),
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
        if (workspaceEntries[i].workspaceId == receivedWorkspaces[((newPosition % numReceivedWorkspaces) + numReceivedWorkspaces) % numReceivedWorkspaces].getId())
        {
          registeredWorkspaceIndex = i;
          Serial.println("Using registered workspaceEntries: " + String(workspaceEntries[i].workspaceId));
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
  int numOfTasks = workspaceEntries[registeredWorkspaceIndex].numOfEntries;
  Task *selectedTasks = workspaceEntries[registeredWorkspaceIndex].entries;
  /** @todo Add protection in case none of the received workspaces is in the registeredWorkspaces map */

  if (machine.executeOnce)
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString(String(numOfTasks) + " time entries",
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
          Serial.println("Workspace ID: " + String(workspaceEntries[registeredWorkspaceIndex].workspaceId));

          togglApiErrorCode_t errorCode = toggl.CreateTimeEntry(selectedTasks[index].getDescription().c_str(), tags, -1, currentTime.c_str(), selectedTasks[index].getProjectId(), "TheToggler_dial",
                                                                workspaceEntries[registeredWorkspaceIndex].workspaceId, &newTimeEntry);
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

bool readEntriesJSON()
{
  M5Dial.Display.clear();
  M5Dial.Display.drawString("Reading entries",
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
    JsonArray configuredWorkspacesJSON = doc["thetoggler"]["workspaces"].as<JsonArray>();
    Serial.println("Number of workspaces configured: " + String(configuredWorkspacesJSON.size()));
    if (configuredWorkspacesJSON.size() == 0)
    {
      doc.clear();
      M5Dial.Display.clear();
      M5Dial.Display.drawString("No workspaces configured",
                                M5Dial.Display.width() / 2,
                                M5Dial.Display.height() / 2);
      delay(1000);
    }
    else
    {
      if (configuredWorkspacesJSON.size() > MAX_NUM_WORKSPACES)
      {
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Too many workspaces",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        delay(1000);
      }
      else
      {
        for (JsonVariant item : configuredWorkspacesJSON)
        {
          workspaceEntries[numWorkspacesConfigured].workspaceId = item["workspaceID"];
          JsonArray entries = item["entries"].as<JsonArray>();
          Serial.println("Number of entries in workspace " + String(item["workspaceID"].as<int>()) + ": " + String(entries.size()));
          for (JsonVariant entry : entries)
          {
            if (workspaceEntries[numWorkspacesConfigured].numOfEntries < MAX_ENTRIES_PER_WORKSPACE)
            {
              workspaceEntries[numWorkspacesConfigured].entries[workspaceEntries[numWorkspacesConfigured].numOfEntries].setDescription(entry["description"]);
              workspaceEntries[numWorkspacesConfigured].entries[workspaceEntries[numWorkspacesConfigured].numOfEntries].setProjectId(entry["projectID"]);
              workspaceEntries[numWorkspacesConfigured].numOfEntries++;
            }
            else
            {
              Serial.println("Too many entries in workspace");
            }
            Serial.println("Entry description: " + String(workspaceEntries[numWorkspacesConfigured].entries[workspaceEntries[numWorkspacesConfigured].numOfEntries - 1].getDescription().c_str()));
          }

          numWorkspacesConfigured++;
        }
      }

      doc.clear();
    }
  }

  delay(1000);
  return true;
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
  readEntriesJSON();
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
