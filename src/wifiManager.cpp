#include "wifiManager.h"

WifiManager::WifiManager()
    : lastSuccessfulSSID(""), lastSuccessfulPassword("")
{
}

void WifiManager::scanNetworks()
{
  M5Dial.Display.clear();
  M5Dial.Display.drawString("Scanning",
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2);
  M5Dial.Display.drawString("networks...",
                            M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2 + 30);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  lastScanResult = WiFi.scanNetworks();
  Serial.println("Network scan completed");
  Serial.println("Found " + String(lastScanResult) + " networks");
}

bool WifiManager::isNetworkAvailable(const char *ssid) const
{
  if (lastScanResult <= 0)
  {
    return false;
  }

  for (int i = 0; i < lastScanResult; ++i)
  {
    if (WiFi.SSID(i).equals(ssid))
    {
      Serial.println("Network found: " + String(ssid) + " (Signal: " + String(WiFi.RSSI(i)) + " dBm)");
      return true;
    }
  }
  return false;
}

WifiManager::wifi_connect_status_t WifiManager::connect(const String &settingsJson)
{
  wifi_connect_status_t status = WIFI_CONNECT_ERROR_FAILED;
  bool isConnected = false;

  /* Try last successful connection first if available */
  if (lastSuccessfulSSID.length() > 0U)
  {
    displayStatus(lastSuccessfulSSID.c_str());
    isConnected = tryConnection(lastSuccessfulSSID.c_str(),
                              lastSuccessfulPassword.c_str(), 2U);

    if (isConnected)
    {
      displayStatus();
      delay(1000U);
      return WIFI_CONNECT_SUCCESS;
    }
    Serial.println("Last successful connection failed: " + lastSuccessfulSSID);
  }

  /* If last connection failed or no previous connection, scan and try other networks */
  scanNetworks();

  if (lastScanResult <= 0)
  {
    Serial.println("No networks found");
    displayStatus();
    delay(1000U);
    return WIFI_CONNECT_ERROR_NO_NETWORKS;
  }

  /* Parse JSON configuration */
  JsonDocument doc;
  const DeserializationError jsonErrorCode = deserializeJson(doc, settingsJson);
  if (jsonErrorCode != DeserializationError::Ok)
  {
    doc.clear();
    Serial.println("Error deserializing JSON: " + String(jsonErrorCode.c_str()));
    return WIFI_CONNECT_ERROR_JSON;
  }

  const JsonArray networks = doc["thetoggler"]["network"].as<JsonArray>();
  const size_t networkCount = networks.size();
  Serial.println("Number of wifi networks configured: " + String(networkCount));

  if (networkCount == 0U)
  {
    doc.clear();
    displayStatus();
    delay(1000U);
    return WIFI_CONNECT_ERROR_NO_NETWORKS;
  }

  /* Try each available network in JSON */
  for (const JsonVariant &network : networks)
  {
    if (isConnected)
    {
      break;
    }

    const String currentSSID = network["ssid"].as<String>();
    const String currentPassword = network["password"].as<String>();

    if (isNetworkAvailable(currentSSID.c_str()))
    {
      isConnected = tryConnection(currentSSID.c_str(),
                                  currentPassword.c_str(),
                                  MAX_RETRIES);

      if (isConnected)
      {
        lastSuccessfulSSID = currentSSID;
        lastSuccessfulPassword = currentPassword;
        break;
      }
    }
    else
    {
      Serial.println("Network not available: " + currentSSID);
    }
  }
  doc.clear();

  // Free memory from scan results
  WiFi.scanDelete();

  displayStatus();
  delay(1000U);

  return isConnected ? WIFI_CONNECT_SUCCESS : WIFI_CONNECT_ERROR_FAILED;
}

void WifiManager::disconnect()
{
  WiFi.disconnect();
  displayStatus();
}

bool WifiManager::tryConnection(const char *ssid, const char *password, uint8_t numRetries)
{
  while ((WiFi.status() != WL_CONNECTED) && (numRetries > 0U))
  {
    displayStatus(ssid);

    Serial.println("Trying wifi: " + String(ssid));
    const wl_status_t connectionStatus = WiFi.begin(ssid, password);
    Serial.println("Connection status: " + String(connectionStatus));
    delay(DELAY_BETWEEN_RETRIES_MS);
    numRetries--;
  }

  return WiFi.status() == WL_CONNECTED;
}

void WifiManager::displayStatus(const char *ssid) const
{
  M5Dial.Display.clear();
  if (ssid != nullptr)
  {
    M5Dial.Display.drawString("Connecting to",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    M5Dial.Display.drawString(ssid,
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2 + 30);
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    M5Dial.Display.drawString("Wifi ON",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
    M5Dial.Display.drawString(WiFi.SSID().c_str(),
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2 + 30);
    Serial.println(WiFi.localIP());
  }
  else
  {
    M5Dial.Display.drawString("Wifi OFF",
                              M5Dial.Display.width() / 2,
                              M5Dial.Display.height() / 2);
  }
}