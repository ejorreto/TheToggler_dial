#include "M5Dial.h"
#include "task.h"
#include "Toggl.h"

#include "credentials.h"
#include "timeManager.h"
#include "sleepyDog.h"
#include "soundManager.h"

#include "StateMachine.h"
#include <ArduinoJson.h>
#include "wifiManager.h"

#define MAX_NUM_WORKSPACES 10
#define MAX_ENTRIES_PER_WORKSPACE 20
#define MAX_NUM_PROJECTS 20

typedef struct workspaceEntries_t
{
  int workspaceId;
  Task entries[MAX_ENTRIES_PER_WORKSPACE];
  uint8_t numOfEntries;
} workspaceEntries_t;

workspaceEntries_t workspaceEntries[MAX_NUM_WORKSPACES];
Project projects[MAX_NUM_PROJECTS];
uint32_t numProjectsReceived = 0;
uint8_t numWorkspacesConfigured = 0;

/* Global variables */
const int STATE_DELAY = 0;
SleepyDog sleepyDog(30000);
StateMachine machine = StateMachine();
Toggl toggl;
TimeManager timeManager;
long oldPosition = -999;
unsigned long lastTime = 0;
unsigned long currentTime = 0;

/* Tones management */
SoundManager soundManager;
uint8_t dialToneId;
uint8_t buttonToneId;
uint8_t readyToneId;
uint8_t errorToneId;

// Add WifiManager instance
WifiManager wifiManager;

/* Functions declaration */

// Workspaces and entries from JSON
bool readEntriesJSON();

/* State machine functions */
void entry_stateWorkplaceSelection();
void entry_stateTimeEntrySelection();
void do_stateWorkplaceSelection();
void do_stateTimeEntrySelection();
void do_stateLowPower();
State *stateWorksplaceSelection = machine.addState(&do_stateWorkplaceSelection);
State *stateTimeEntrySelection = machine.addState(&do_stateTimeEntrySelection);
State *stateLowPower = machine.addState(&do_stateLowPower);
State *nextState = nullptr;

/* Workspaces */
Workspace receivedWorkspaces[MAX_NUM_WORKSPACES];
uint32_t numReceivedWorkspaces = 0;
int registeredWorkspaceIndex = -1;

/**
 * @brief Turn off the screen
 *
 * @return * void
 */
void screenOff()
{
  // M5Dial.Display.powerSaveOn();
  M5Dial.Display.setBrightness(0);
}

/**
 * @brief Turn on the screen
 *
 */
void screenOn()
{
  lastTime = currentTime;
  // M5Dial.Display.powerSaveOff();
  M5Dial.Display.setBrightness(255);
}

/**
 * @brief Entry function for the workplace selection state
 *
 */
void entry_stateWorkplaceSelection()
{
  togglApiErrorCode_t errorCode = TOGGL_API_EC_OK;
  /* This will be executed only when entering the state */
  if (wifiManager.isConnected() == false)
  {
    wifiManager.connect(settingsJson);
  }

  if (wifiManager.isConnected())
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("Getting",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    M5Dial.Display.drawString("workspaces",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2 + 30);

    errorCode = toggl.getWorkSpaces(receivedWorkspaces, MAX_NUM_WORKSPACES, &numReceivedWorkspaces);

    if (errorCode != TOGGL_API_EC_OK)
    {
      soundManager.playSound(errorToneId);
      M5Dial.Display.clear();
      M5Dial.Display.drawString("Error " + String(errorCode),
                                M5Dial.Display.width() / 2,
                                M5Dial.Display.height() / 2);
      /** @todo Display a more comprehensive error in the screen */
      delay(1000);
      nextState = stateWorksplaceSelection;
    }
    else
    {

      if (numReceivedWorkspaces == 0)
      {
        M5Dial.Display.clear();
        M5Dial.Display.drawString("No workspaces",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        soundManager.playSound(errorToneId);
      }
      else
      {
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Select",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        M5Dial.Display.drawString("workspace",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2 + 30);
        soundManager.playSound(readyToneId);
      }
    }
  }
  else
  {
    M5Dial.Display.clear();
    M5Dial.Display.drawString("No wifi",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    soundManager.playSound(errorToneId);
    delay(1000);
    nextState = stateWorksplaceSelection;
  }
}

/**
 * @brief Entry function for the time entry selection state
 *
 */
void entry_stateTimeEntrySelection()
{
  int numOfTasks = workspaceEntries[registeredWorkspaceIndex].numOfEntries;
  sleepyDog.feed();
  M5Dial.Display.clear();
  M5Dial.Display.drawString(String(numOfTasks) + " time entries",
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2);
  soundManager.playSound(readyToneId);
}

/**
 * @brief Workplace selection state function
 *
 */
void do_stateWorkplaceSelection()
{
  togglApiErrorCode_t errorCode = TOGGL_API_EC_OK;
  if (machine.executeOnce)
  {
    entry_stateWorkplaceSelection();
  }

  /* This will be executed cyclically while in this state */
  if (numReceivedWorkspaces > 0)
  {
    long newPosition = M5Dial.Encoder.read();

    if (newPosition != oldPosition)
    {
      lastTime = currentTime;
      soundManager.playSound(dialToneId);
      M5Dial.Display.clear();
      oldPosition = newPosition;
      Serial.println(newPosition);
      M5Dial.Display.drawString(receivedWorkspaces[((newPosition % numReceivedWorkspaces) + numReceivedWorkspaces) % numReceivedWorkspaces].getName().c_str(),
                                M5Dial.Display.width() / 2,
                                M5Dial.Display.height() / 2);
    }

    if (M5Dial.BtnA.wasPressed())
    {
      soundManager.playSound(buttonToneId);
      sleepyDog.feed();
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

      /** @todo This might be moved to the entry function of the state for time entry selection */
      /* And lets assign the project name to each of the configured tasks in the selected workspace */
      toggl.getProjects(projects, MAX_NUM_PROJECTS, &numProjectsReceived, workspaceEntries[registeredWorkspaceIndex].workspaceId);
      for (int i = 0; i < workspaceEntries[registeredWorkspaceIndex].numOfEntries; i++)
      {
        for (int j = 0; j < numProjectsReceived; j++)
        {
          if (workspaceEntries[registeredWorkspaceIndex].entries[i].getProjectId() == projects[j].getId())
          {
            workspaceEntries[registeredWorkspaceIndex].entries[i].setProjectName(projects[j].getName());
            // Serial.println("Set Project name: " + String(workspaceEntries[registeredWorkspaceIndex].entries[i].getProjectName().c_str()));
          }
        }
      }
      nextState = stateTimeEntrySelection;
    }
  }
}

/**
 * @brief Time entry selection state function
 *
 */
void do_stateTimeEntrySelection()
{
  int numOfTasks = workspaceEntries[registeredWorkspaceIndex].numOfEntries;
  Task *selectedTasks = workspaceEntries[registeredWorkspaceIndex].entries;
  /** @todo Add protection in case none of the received workspaces is in the registeredWorkspaces map */

  if (machine.executeOnce)
  {
    entry_stateTimeEntrySelection();
  }

  long newPosition = M5Dial.Encoder.read();
  if (newPosition != oldPosition)
  {
    sleepyDog.feed();
    soundManager.playSound(dialToneId);
    M5Dial.Display.clear();
    oldPosition = newPosition;
    Serial.println(newPosition);

    M5Dial.Display.drawString(selectedTasks[((newPosition % numOfTasks) + numOfTasks) % numOfTasks].getDescription().c_str(),
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    M5Dial.Display.drawString((selectedTasks[((newPosition % numOfTasks) + numOfTasks) % numOfTasks].getProjectName()).c_str(),
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2 + 30);
  }

  if (M5Dial.BtnA.wasPressed())
  {
    sleepyDog.feed();
    soundManager.playSound(buttonToneId);
    String currentTimestamp = "No time";

    if (wifiManager.isConnected() == false)
    {
      wifiManager.connect(settingsJson);
    }

    if (wifiManager.isConnected())
    {
      int index = ((newPosition % numOfTasks) + numOfTasks) % numOfTasks;
      if (index == 0)
      {
        nextState = stateWorksplaceSelection;
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
          soundManager.playSound(errorToneId);
          delay(1000);
        }
        else if (errorCode != TOGGL_API_EC_OK)
        {
          /* There is no entry currently running */
          M5Dial.Display.clear();
          M5Dial.Display.drawString("Error get: " + String(errorCode),
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
          soundManager.playSound(errorToneId);
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
            soundManager.playSound(errorToneId);
            delay(1000);
            nextState = stateTimeEntrySelection;
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
            soundManager.playSound(readyToneId);
          }
        }
      }
      else
      {
        TimeEntry newTimeEntry;
        /* Lets create a new time entry */
        M5Dial.Display.clear();
        M5Dial.Display.drawString("Getting UTC",
                                  M5Dial.Display.width() / 2,
                                  M5Dial.Display.height() / 2);
        String currentTimestamp = timeManager.getCurrentTime("UTC");
        sleepyDog.feed(); /* Avoid going to low power inmediately after getting the current time */
        if (currentTimestamp.length() > 1)
        {
          Serial.println(currentTimestamp.c_str());

          M5Dial.Display.clear();
          M5Dial.Display.drawString(currentTimestamp.c_str(),
                                    M5Dial.Display.width() / 2,
                                    M5Dial.Display.height() / 2);
          Serial.println("---- Creating a new entry");

          String tags = "";

          Serial.println("Description: " + String(selectedTasks[index].getDescription().c_str()));
          Serial.println("Project ID: " + String(selectedTasks[index].getProjectId()));
          Serial.println("Workspace ID: " + String(workspaceEntries[registeredWorkspaceIndex].workspaceId));

          togglApiErrorCode_t errorCode = toggl.CreateTimeEntry(selectedTasks[index].getDescription().c_str(), tags, -1, currentTimestamp.c_str(), selectedTasks[index].getProjectId(), "TheToggler_dial",
                                                                workspaceEntries[registeredWorkspaceIndex].workspaceId, &newTimeEntry);
          if (errorCode == TOGGL_API_EC_OK)
          {
            M5Dial.Display.clear();
            soundManager.playSound(buttonToneId);
            /* Show info about the newly created time entry from the API */
            M5Dial.Display.setTextSize(0.45);
            M5Dial.Display.drawString(newTimeEntry.getAt().c_str(),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextSize(0.75);
            M5Dial.Display.drawString(newTimeEntry.getDescription().c_str(),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2);
            M5Dial.Display.drawString(newTimeEntry.getProjectName().c_str(),
                                      M5Dial.Display.width() / 2,
                                      M5Dial.Display.height() / 2 + 30);
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
          soundManager.playSound(errorToneId);
        }
      }
    }
  }

  /* Move to the low power state if the encoder or the button were not used in some time */
  if (sleepyDog.isSleeping())
  {
    nextState = stateLowPower;
  }
}

void do_stateLowPower()
{
  if (machine.executeOnce)
  {
    screenOff();
    wifiManager.disconnect();
  }

  long newPosition = M5Dial.Encoder.read();

  if (newPosition != oldPosition)
  {
    /* Turn screen on when the dial is moved */
    soundManager.playSound(dialToneId);
    screenOn();
    nextState = stateTimeEntrySelection;
  }
}

bool readEntriesJSON()
{
  // M5Dial.Display.clear();
  // M5Dial.Display.drawString("Reading entries",
  //                           M5Dial.Display.width() / 2,
  //                           M5Dial.Display.height() / 2);
  JsonDocument doc;
  DeserializationError jsonErrorCode = deserializeJson(doc, settingsJson);
  if (jsonErrorCode != DeserializationError::Ok)
  {
    doc.clear();
    Serial.println("Error deserializing JSON: " + String(jsonErrorCode.c_str()));
  }
  else
  {
    serializeJsonPretty(doc, Serial); // for debugging
    JsonArray configuredWorkspacesJSON = doc["thetoggler"]["workspaces"].as<JsonArray>();
    Serial.println("Number of workspaces configured:" + String(configuredWorkspacesJSON.size()));
    if (configuredWorkspacesJSON.size() == 0)
    {
      doc.clear();
      M5Dial.Display.clear();
      M5Dial.Display.drawString("No workspaces configured",
                                M5Dial.Display.width() / 2,
                                M5Dial.Display.height() / 2);
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
              /** @todo give feedback of this in the display */
            }
            Serial.println("Entry description: " + String(workspaceEntries[numWorkspacesConfigured].entries[workspaceEntries[numWorkspacesConfigured].numOfEntries - 1].getDescription().c_str()));
          }

          numWorkspacesConfigured++;
        }
      }

      doc.clear();
    }
  }

  return true;
}

void setup()
{
  Serial.begin(115200);
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  M5Dial.Display.setTextColor(GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
  M5Dial.Display.setTextSize(0.75);
  M5Dial.Display.setRotation(2);

  // Register common sounds
  soundManager.registerSound(1000, 40, dialToneId);   // High pitch dial rotation sound
  soundManager.registerSound(6000, 20, buttonToneId); // Button press sound
  soundManager.registerSound(12000, 20, readyToneId); // Action required by the user
  soundManager.registerSound(1000, 100, errorToneId);   // Error sound

  wifiManager.connect(settingsJson);
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
