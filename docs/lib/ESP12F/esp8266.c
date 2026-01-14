/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "esp8266.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/************************************************************************************************************
**************    Private Definitions
************************************************************************************************************/

#define ESP_CMD_BUFFER_SIZE           256

/************************************************************************************************************
**************    Private Variables
************************************************************************************************************/

static ESP_DataReceivedCallback gDataReceivedCallback = NULL;

/************************************************************************************************************
**************    Private Functions
************************************************************************************************************/

static bool ESP_WaitForResponse(ESP_HandleTypeDef* hEsp, const char* pExpected, uint32_t Timeout);
static bool ESP_SendCommand(ESP_HandleTypeDef* hEsp, const char* pCmd, const char* pExpectedResp, uint32_t Timeout);
static void ESP_ParseIPD(ESP_HandleTypeDef* hEsp, const char* pData);
static void ESP_EventCallback(const char* pEvent);

/************************************************************************************************************
**************    Public Functions - Initialization
************************************************************************************************************/

/**
  * @brief  Initializes the ESP8266 handle.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  hAtc: Pointer to the ATC handle.
  * @retval true if initialization is successful, false otherwise.
  */
bool ESP_Init(ESP_HandleTypeDef* hEsp, ATC_HandleTypeDef* hAtc)
{
  bool answer = false;
  do
  {
    if (hEsp == NULL || hAtc == NULL)
    {
      break;
    }
    memset(hEsp, 0, sizeof(ESP_HandleTypeDef));
    hEsp->hAtc = hAtc;

    // Test communication
    if (!ESP_Test(hEsp))
    {
      break;
    }

    // Disable echo for easier parsing
    if (!ESP_SetEcho(hEsp, false))
    {
      break;
    }

    // Enable multiple connections
    if (!ESP_Conn_SetMultiple(hEsp, true))
    {
      break;
    }

    hEsp->IsInitialized = true;
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  DeInitializes the ESP8266 handle.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if successful.
  */
bool ESP_DeInit(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;
  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    ESP_Conn_CloseAll(hEsp);
    ESP_WiFi_Disconnect(hEsp);

    memset(hEsp, 0, sizeof(ESP_HandleTypeDef));
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Resets the ESP8266 module.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if successful.
  */
bool ESP_Reset(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;
  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+RST\r\n", 1000, NULL, 5000, 1, "ready") <= 0)
    {
      break;
    }

    ATC_Delay(2000);
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Tests ESP8266 communication.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if ESP8266 responds.
  */
bool ESP_Test(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;
  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    // Try multiple times
    for (int i = 0; i < 3; i++)
    {
      if (ATC_SendReceive(hEsp->hAtc, "AT\r\n", 1000, NULL, 1000, 1, "OK") > 0)
      {
        answer = true;
        break;
      }
      ATC_Delay(500);
    }

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Enables or disables command echo.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  Enable: true to enable, false to disable.
  * @retval true if successful.
  */
bool ESP_SetEcho(ESP_HandleTypeDef* hEsp, bool Enable)
{
  bool answer = false;
  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    const char* cmd = Enable ? "ATE1\r\n" : "ATE0\r\n";
    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 1000, 1, "OK") <= 0)
    {
      break;
    }

    hEsp->EchoEnabled = Enable;
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Gets ESP8266 firmware version.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pVersion: Buffer to store version string.
  * @param  MaxLen: Maximum buffer length.
  * @retval true if successful.
  */
bool ESP_GetVersion(ESP_HandleTypeDef* hEsp, char* pVersion, uint16_t MaxLen)
{
  bool answer = false;
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL || pVersion == NULL || MaxLen == 0)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+GMR\r\n", 1000, &pResp, 2000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL)
    {
      strncpy(pVersion, pResp, MaxLen - 1);
      pVersion[MaxLen - 1] = '\0';
    }

    answer = true;

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - WiFi Station Mode
************************************************************************************************************/

/**
  * @brief  Sets WiFi mode.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  Mode: WiFi mode (STATION, AP, or BOTH).
  * @retval true if successful.
  */
bool ESP_WiFi_SetMode(ESP_HandleTypeDef* hEsp, uint8_t Mode)
{
  bool answer = false;
  char cmd[32];

  do
  {
    if (hEsp == NULL || Mode > ESP_WIFI_MODE_AP_STATION)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CWMODE=%d\r\n", Mode);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    hEsp->WiFiMode = Mode;
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Connects to a WiFi network.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pSSID: WiFi network SSID.
  * @param  pPassword: WiFi password.
  * @retval true if connected successfully.
  */
bool ESP_WiFi_Connect(ESP_HandleTypeDef* hEsp, const char* pSSID, const char* pPassword)
{
  bool answer = false;
  char cmd[ESP_CMD_BUFFER_SIZE];
  int result;

  do
  {
    if (hEsp == NULL || pSSID == NULL)
    {
      break;
    }

    // Set to station mode if not already
    if (hEsp->WiFiMode != ESP_WIFI_MODE_STATION && hEsp->WiFiMode != ESP_WIFI_MODE_AP_STATION)
    {
      if (!ESP_WiFi_SetMode(hEsp, ESP_WIFI_MODE_STATION))
      {
        break;
      }
    }

    // Build connection command
    if (pPassword != NULL && strlen(pPassword) > 0)
    {
      snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", pSSID, pPassword);
    }
    else
    {
      snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\"\r\n", pSSID);
    }

    // Send command and wait for connection
    result = ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, ESP_TIMEOUT_WIFI_CONNECT, 3,
                             "OK", "WIFI CONNECTED", "WIFI GOT IP");

    if (result <= 0)
    {
      break;
    }

    // Store connection info
    strncpy(hEsp->WiFiInfo.SSID, pSSID, ESP_SSID_MAX_LEN);
    hEsp->WiFiInfo.SSID[ESP_SSID_MAX_LEN] = '\0';
    hEsp->WiFiInfo.IsConnected = true;

    // Get IP address
    ESP_Network_GetIP(hEsp);

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Disconnects from WiFi network.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if successful.
  */
bool ESP_WiFi_Disconnect(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+CWQAP\r\n", 1000, NULL, 5000, 1, "OK") <= 0)
    {
      break;
    }

    hEsp->WiFiInfo.IsConnected = false;
    memset(hEsp->WiFiInfo.SSID, 0, sizeof(hEsp->WiFiInfo.SSID));
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Gets current WiFi connection status.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if connected.
  */
bool ESP_WiFi_GetStatus(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+CWJAP?\r\n", 1000, &pResp, 2000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL && strstr(pResp, "No AP") == NULL)
    {
      hEsp->WiFiInfo.IsConnected = true;
      answer = true;
    }
    else
    {
      hEsp->WiFiInfo.IsConnected = false;
    }

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Enables or disables auto-connect on boot.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  Enable: true to enable, false to disable.
  * @retval true if successful.
  */
bool ESP_WiFi_AutoConnect(ESP_HandleTypeDef* hEsp, bool Enable)
{
  bool answer = false;
  char cmd[32];

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CWAUTOCONN=%d\r\n", Enable ? 1 : 0);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Scans for available WiFi networks.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pNetworks: Array to store found networks.
  * @param  pCount: Pointer to store number of networks found.
  * @param  MaxCount: Maximum number of networks to return.
  * @retval true if successful.
  */
bool ESP_WiFi_ScanNetworks(ESP_HandleTypeDef* hEsp, ESP_WiFiInfo_TypeDef* pNetworks, uint8_t* pCount, uint8_t MaxCount)
{
  bool answer = false;
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL || pNetworks == NULL || pCount == NULL)
    {
      break;
    }

    *pCount = 0;

    if (ATC_SendReceive(hEsp->hAtc, "AT+CWLAP\r\n", 1000, &pResp, 10000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL)
    {
      // Parse response format: +CWLAP:(encryption),(ssid),(rssi),(mac),(channel)
      char* line = strtok((char*)pResp, "\n");
      while (line != NULL && *pCount < MaxCount)
      {
        if (strstr(line, "+CWLAP:") != NULL)
        {
          int enc, rssi, ch;
          char ssid[ESP_SSID_MAX_LEN + 1];

          if (sscanf(line, "+CWLAP:(%d,\"%[^\"]\",%d,%*[^,],%d)",
                     &enc, ssid, &rssi, &ch) == 4)
          {
            strncpy(pNetworks[*pCount].SSID, ssid, ESP_SSID_MAX_LEN);
            pNetworks[*pCount].SSID[ESP_SSID_MAX_LEN] = '\0';
            pNetworks[*pCount].RSSI = rssi;
            pNetworks[*pCount].Channel = ch;
            pNetworks[*pCount].Encryption = enc;
            (*pCount)++;
          }
        }
        line = strtok(NULL, "\n");
      }
    }

    answer = true;

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - WiFi AP Mode
************************************************************************************************************/

/**
  * @brief  Configures ESP8266 as Access Point.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pSSID: AP SSID.
  * @param  pPassword: AP password (min 8 characters, NULL for open).
  * @param  Channel: WiFi channel (1-13).
  * @param  Encryption: Encryption type.
  * @retval true if successful.
  */
bool ESP_AP_Configure(ESP_HandleTypeDef* hEsp, const char* pSSID, const char* pPassword,
                      uint8_t Channel, uint8_t Encryption)
{
  bool answer = false;
  char cmd[ESP_CMD_BUFFER_SIZE];

  do
  {
    if (hEsp == NULL || pSSID == NULL)
    {
      break;
    }

    if (Channel < 1 || Channel > 13)
    {
      break;
    }

    // Set AP mode
    if (!ESP_WiFi_SetMode(hEsp, ESP_WIFI_MODE_AP))
    {
      break;
    }

    // Configure AP
    if (pPassword != NULL && strlen(pPassword) >= 8)
    {
      snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"%s\",%d,%d\r\n",
               pSSID, pPassword, Channel, Encryption);
    }
    else
    {
      snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"\",%d,0\r\n",
               pSSID, Channel);
    }

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 5000, 1, "OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Gets number of connected devices in AP mode.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pCount: Pointer to store device count.
  * @retval true if successful.
  */
bool ESP_AP_GetConnectedDevices(ESP_HandleTypeDef* hEsp, uint8_t* pCount)
{
  bool answer = false;
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL || pCount == NULL)
    {
      break;
    }

    *pCount = 0;

    if (ATC_SendReceive(hEsp->hAtc, "AT+CWLIF\r\n", 1000, &pResp, 2000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL)
    {
      char* line = strtok((char*)pResp, "\n");
      while (line != NULL)
      {
        if (strstr(line, ",") != NULL)
        {
          (*pCount)++;
        }
        line = strtok(NULL, "\n");
      }
    }

    answer = true;

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - Network Configuration
************************************************************************************************************/

/**
  * @brief  Gets current IP address information.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if successful.
  */
bool ESP_Network_GetIP(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+CIFSR\r\n", 1000, &pResp, 2000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL)
    {
      // Parse STAIP and STAMAC
      char* ip_line = strstr(pResp, "+CIFSR:STAIP,\"");
      if (ip_line != NULL)
      {
        sscanf(ip_line, "+CIFSR:STAIP,\"%[^\"]\"", hEsp->NetworkInfo.IP);
      }

      char* mac_line = strstr(pResp, "+CIFSR:STAMAC,\"");
      if (mac_line != NULL)
      {
        sscanf(mac_line, "+CIFSR:STAMAC,\"%[^\"]\"", hEsp->NetworkInfo.MAC);
      }
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Sets static IP address.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pIP: IP address string.
  * @param  pGateway: Gateway address string.
  * @param  pNetmask: Netmask string.
  * @retval true if successful.
  */
bool ESP_Network_SetStaticIP(ESP_HandleTypeDef* hEsp, const char* pIP,
                             const char* pGateway, const char* pNetmask)
{
  bool answer = false;
  char cmd[ESP_CMD_BUFFER_SIZE];

  do
  {
    if (hEsp == NULL || pIP == NULL || pGateway == NULL || pNetmask == NULL)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSTA=\"%s\",\"%s\",\"%s\"\r\n",
             pIP, pGateway, pNetmask);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    strncpy(hEsp->NetworkInfo.IP, pIP, ESP_IP_MAX_LEN);
    strncpy(hEsp->NetworkInfo.Gateway, pGateway, ESP_IP_MAX_LEN);
    strncpy(hEsp->NetworkInfo.Netmask, pNetmask, ESP_IP_MAX_LEN);

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Enables or disables DHCP.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  Enable: true to enable DHCP.
  * @retval true if successful.
  */
bool ESP_Network_EnableDHCP(ESP_HandleTypeDef* hEsp, bool Enable)
{
  bool answer = false;
  char cmd[32];

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    // Mode: 0=soft-AP, 1=station, 2=both
    snprintf(cmd, sizeof(cmd), "AT+CWDHCP=%d,%d\r\n", 1, Enable ? 1 : 0);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Pings a host.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pHost: Hostname or IP address.
  * @param  pTime: Pointer to store ping time in ms.
  * @retval true if ping successful.
  */
bool ESP_Network_Ping(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t* pTime)
{
  bool answer = false;
  char cmd[ESP_CMD_BUFFER_SIZE];
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL || pHost == NULL)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+PING=\"%s\"\r\n", pHost);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, &pResp, 10000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL && pTime != NULL)
    {
      int time;
      if (sscanf(pResp, "+%d", &time) == 1)
      {
        *pTime = (uint16_t)time;
      }
    }

    answer = true;

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - TCP/UDP Connection
************************************************************************************************************/

/**
  * @brief  Establishes TCP connection.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pHost: Remote host address.
  * @param  Port: Remote port number.
  * @retval Link ID (0-4) on success, negative error code on failure.
  */
int ESP_Conn_TCPConnect(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t Port)
{
  int answer = ESP_ERROR;
  char cmd[ESP_CMD_BUFFER_SIZE];
  uint8_t link_id = 0;

  do
  {
    if (hEsp == NULL || pHost == NULL)
    {
      answer = ESP_ERROR_INVALID_PARAM;
      break;
    }

    // Find available link ID
    for (link_id = 0; link_id < ESP_MAX_CONNECTIONS; link_id++)
    {
      if (!hEsp->Connections[link_id].IsConnected)
      {
        break;
      }
    }

    if (link_id >= ESP_MAX_CONNECTIONS)
    {
      answer = ESP_ERROR_BUSY;
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n",
             link_id, pHost, Port);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 10000, 2, "OK", "CONNECT") <= 0)
    {
      answer = ESP_ERROR_NOT_CONNECTED;
      break;
    }

    // Store connection info
    hEsp->Connections[link_id].LinkID = link_id;
    hEsp->Connections[link_id].Type = ESP_CONN_TYPE_TCP;
    hEsp->Connections[link_id].RemotePort = Port;
    strncpy(hEsp->Connections[link_id].RemoteIP, pHost, ESP_IP_MAX_LEN);
    hEsp->Connections[link_id].IsConnected = true;

    answer = link_id;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Establishes UDP connection.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pHost: Remote host address.
  * @param  Port: Remote port number.
  * @param  LocalPort: Local port number.
  * @retval Link ID (0-4) on success, negative error code on failure.
  */
int ESP_Conn_UDPConnect(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t Port, uint16_t LocalPort)
{
  int answer = ESP_ERROR;
  char cmd[ESP_CMD_BUFFER_SIZE];
  uint8_t link_id = 0;

  do
  {
    if (hEsp == NULL || pHost == NULL)
    {
      answer = ESP_ERROR_INVALID_PARAM;
      break;
    }

    for (link_id = 0; link_id < ESP_MAX_CONNECTIONS; link_id++)
    {
      if (!hEsp->Connections[link_id].IsConnected)
      {
        break;
      }
    }

    if (link_id >= ESP_MAX_CONNECTIONS)
    {
      answer = ESP_ERROR_BUSY;
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d,0\r\n",
             link_id, pHost, Port, LocalPort);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 10000, 2, "OK", "CONNECT") <= 0)
    {
      answer = ESP_ERROR_NOT_CONNECTED;
      break;
    }

    hEsp->Connections[link_id].LinkID = link_id;
    hEsp->Connections[link_id].Type = ESP_CONN_TYPE_UDP;
    hEsp->Connections[link_id].RemotePort = Port;
    hEsp->Connections[link_id].LocalPort = LocalPort;
    strncpy(hEsp->Connections[link_id].RemoteIP, pHost, ESP_IP_MAX_LEN);
    hEsp->Connections[link_id].IsConnected = true;

    answer = link_id;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Closes a connection.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  LinkID: Connection link ID.
  * @retval true if successful.
  */
bool ESP_Conn_Close(ESP_HandleTypeDef* hEsp, uint8_t LinkID)
{
  bool answer = false;
  char cmd[32];

  do
  {
    if (hEsp == NULL || LinkID >= ESP_MAX_CONNECTIONS)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", LinkID);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 5000, 1, "OK") <= 0)
    {
      break;
    }

    memset(&hEsp->Connections[LinkID], 0, sizeof(ESP_Connection_TypeDef));
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Closes all connections.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if successful.
  */
bool ESP_Conn_CloseAll(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+CIPCLOSE=5\r\n", 1000, NULL, 5000, 1, "OK") <= 0)
    {
      break;
    }

    memset(hEsp->Connections, 0, sizeof(hEsp->Connections));
    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Sends data over a connection.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  LinkID: Connection link ID.
  * @param  pData: Data buffer to send.
  * @param  Length: Data length.
  * @retval true if successful.
  */
bool ESP_Conn_Send(ESP_HandleTypeDef* hEsp, uint8_t LinkID, const uint8_t* pData, uint16_t Length)
{
  bool answer = false;
  char cmd[64];

  do
  {
    if (hEsp == NULL || pData == NULL || Length == 0)
    {
      break;
    }

    if (LinkID >= ESP_MAX_CONNECTIONS || !hEsp->Connections[LinkID].IsConnected)
    {
      break;
    }

    // Send CIPSEND command
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", LinkID, Length);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, ">") <= 0)
    {
      break;
    }

    // Send actual data
    if (!ATC_Send(hEsp->hAtc, (const char*)pData, ESP_TIMEOUT_SEND))
    {
      break;
    }

    // Wait for SEND OK
    if (ATC_Receive(hEsp->hAtc, NULL, 5000, 1, "SEND OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Receives data from a connection.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  LinkID: Connection link ID.
  * @param  pBuffer: Buffer to store received data.
  * @param  MaxLen: Maximum buffer length.
  * @param  Timeout: Receive timeout in ms.
  * @retval Number of bytes received, or negative error code.
  */
int ESP_Conn_Receive(ESP_HandleTypeDef* hEsp, uint8_t LinkID, uint8_t* pBuffer,
                     uint16_t MaxLen, uint32_t Timeout)
{
  int answer = 0;
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL || pBuffer == NULL || MaxLen == 0)
    {
      answer = ESP_ERROR_INVALID_PARAM;
      break;
    }

    if (LinkID >= ESP_MAX_CONNECTIONS)
    {
      answer = ESP_ERROR_INVALID_PARAM;
      break;
    }

    // Wait for +IPD response
    if (ATC_Receive(hEsp->hAtc, &pResp, Timeout, 1, "+IPD") <= 0)
    {
      answer = ESP_ERROR_RECEIVE_FAIL;
      break;
    }

    if (pResp != NULL)
    {
      // Parse +IPD,<link>,<len>:<data>
      int link, len;
      char* data_start = NULL;

      if (sscanf(pResp, "+IPD,%d,%d:", &link, &len) == 2)
      {
        if (link == LinkID)
        {
          data_start = strchr(pResp, ':');
          if (data_start != NULL)
          {
            data_start++; // Skip ':'
            int copy_len = (len < MaxLen) ? len : MaxLen;
            memcpy(pBuffer, data_start, copy_len);
            answer = copy_len;
          }
        }
      }
    }

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Enables or disables multiple connections.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  Enable: true to enable multiple connections.
  * @retval true if successful.
  */
bool ESP_Conn_SetMultiple(ESP_HandleTypeDef* hEsp, bool Enable)
{
  bool answer = false;
  char cmd[32];

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPMUX=%d\r\n", Enable ? 1 : 0);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - TCP/UDP Server
************************************************************************************************************/

/**
  * @brief  Starts TCP server.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  Port: Server port number.
  * @param  Timeout: Connection timeout in seconds.
  * @retval true if successful.
  */
bool ESP_Server_Start(ESP_HandleTypeDef* hEsp, uint16_t Port, uint32_t Timeout)
{
  bool answer = false;
  char cmd[64];

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    // Enable multiple connections first
    if (!ESP_Conn_SetMultiple(hEsp, true))
    {
      break;
    }

    // Start server
    snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%d\r\n", Port);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    // Set timeout
    if (Timeout > 0)
    {
      snprintf(cmd, sizeof(cmd), "AT+CIPSTO=%lu\r\n", Timeout);
      ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK");
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Stops TCP server.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval true if successful.
  */
bool ESP_Server_Stop(ESP_HandleTypeDef* hEsp)
{
  bool answer = false;

  do
  {
    if (hEsp == NULL)
    {
      break;
    }

    if (ATC_SendReceive(hEsp->hAtc, "AT+CIPSERVER=0\r\n", 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Sets maximum number of server connections.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  MaxConn: Maximum connections (1-5).
  * @retval true if successful.
  */
bool ESP_Server_SetMaxConnections(ESP_HandleTypeDef* hEsp, uint8_t MaxConn)
{
  bool answer = false;
  char cmd[32];

  do
  {
    if (hEsp == NULL || MaxConn < 1 || MaxConn > 5)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSERVERMAXCONN=%d\r\n", MaxConn);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, NULL, 2000, 1, "OK") <= 0)
    {
      break;
    }

    answer = true;

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - HTTP Client
************************************************************************************************************/

/**
  * @brief  Performs HTTP GET request.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pURL: URL to request.
  * @param  pResponse: Buffer to store response.
  * @param  MaxLen: Maximum response length.
  * @retval Number of bytes received, or negative error code.
  */
int ESP_HTTP_GET(ESP_HandleTypeDef* hEsp, const char* pURL, char* pResponse, uint16_t MaxLen)
{
  int answer = ESP_ERROR;
  char host[128] = {0};
  char path[256] = {0};
  uint16_t port = 80;
  int link_id;

  do
  {
    if (hEsp == NULL || pURL == NULL || pResponse == NULL)
    {
      answer = ESP_ERROR_INVALID_PARAM;
      break;
    }

    // Parse URL (simplified, assumes http://host[:port]/path format)
    if (sscanf(pURL, "http://%[^:/]:%hu/%s", host, &port, path) < 2)
    {
      if (sscanf(pURL, "http://%[^/]/%s", host, path) < 1)
      {
        answer = ESP_ERROR_INVALID_PARAM;
        break;
      }
      port = 80;
    }

    // Connect to server
    link_id = ESP_Conn_TCPConnect(hEsp, host, port);
    if (link_id < 0)
    {
      answer = link_id;
      break;
    }

    // Build HTTP request
    char request[512];
    snprintf(request, sizeof(request),
             "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             path, host);

    // Send request
    if (!ESP_Conn_Send(hEsp, link_id, (uint8_t*)request, strlen(request)))
    {
      ESP_Conn_Close(hEsp, link_id);
      answer = ESP_ERROR_SEND_FAIL;
      break;
    }

    // Receive response
    answer = ESP_Conn_Receive(hEsp, link_id, (uint8_t*)pResponse, MaxLen, 10000);

    // Close connection
    ESP_Conn_Close(hEsp, link_id);

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Performs HTTP POST request.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pURL: URL to request.
  * @param  pData: Data to post.
  * @param  pResponse: Buffer to store response.
  * @param  MaxLen: Maximum response length.
  * @retval Number of bytes received, or negative error code.
  */
int ESP_HTTP_POST(ESP_HandleTypeDef* hEsp, const char* pURL, const char* pData,
                  char* pResponse, uint16_t MaxLen)
{
  int answer = ESP_ERROR;
  char host[128] = {0};
  char path[256] = {0};
  uint16_t port = 80;
  int link_id;

  do
  {
    if (hEsp == NULL || pURL == NULL || pData == NULL || pResponse == NULL)
    {
      answer = ESP_ERROR_INVALID_PARAM;
      break;
    }

    // Parse URL
    if (sscanf(pURL, "http://%[^:/]:%hu/%s", host, &port, path) < 2)
    {
      if (sscanf(pURL, "http://%[^/]/%s", host, path) < 1)
      {
        answer = ESP_ERROR_INVALID_PARAM;
        break;
      }
      port = 80;
    }

    // Connect
    link_id = ESP_Conn_TCPConnect(hEsp, host, port);
    if (link_id < 0)
    {
      answer = link_id;
      break;
    }

    // Build HTTP POST request
    char request[1024];
    snprintf(request, sizeof(request),
             "POST /%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n%s",
             path, host, (int)strlen(pData), pData);

    // Send request
    if (!ESP_Conn_Send(hEsp, link_id, (uint8_t*)request, strlen(request)))
    {
      ESP_Conn_Close(hEsp, link_id);
      answer = ESP_ERROR_SEND_FAIL;
      break;
    }

    // Receive response
    answer = ESP_Conn_Receive(hEsp, link_id, (uint8_t*)pResponse, MaxLen, 10000);

    ESP_Conn_Close(hEsp, link_id);

  } while (0);

  return answer;
}

/************************************************************************************************************
**************    Public Functions - DNS
************************************************************************************************************/

/**
  * @brief  Resolves hostname to IP address.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @param  pHostname: Hostname to resolve.
  * @param  pIP: Buffer to store IP address.
  * @retval true if successful.
  */
bool ESP_DNS_Resolve(ESP_HandleTypeDef* hEsp, const char* pHostname, char* pIP)
{
  bool answer = false;
  char cmd[ESP_CMD_BUFFER_SIZE];
  char* pResp = NULL;

  do
  {
    if (hEsp == NULL || pHostname == NULL || pIP == NULL)
    {
      break;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPDOMAIN=\"%s\"\r\n", pHostname);

    if (ATC_SendReceive(hEsp->hAtc, cmd, 1000, &pResp, 10000, 1, "OK") <= 0)
    {
      break;
    }

    if (pResp != NULL)
    {
      char* ip_start = strstr(pResp, "+CIPDOMAIN:");
      if (ip_start != NULL)
      {
        sscanf(ip_start, "+CIPDOMAIN:\"%[^\"]\"", pIP);
        answer = true;
      }
    }

  } while (0);

  return answer;
}

/***********************************************************************************************************/

/**
  * @brief  Process incoming events and data.
  * @param  hEsp: Pointer to the ESP8266 handle.
  * @retval None.
  */
void ESP_ProcessEvents(ESP_HandleTypeDef* hEsp)
{
  if (hEsp == NULL || hEsp->hAtc == NULL)
  {
    return;
  }

  // Let ATC library process events
  ATC_Loop(hEsp->hAtc);
}
