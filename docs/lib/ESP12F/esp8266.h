#ifndef _ESP8266_H_
#define _ESP8266_H_

/***********************************************************************************************************

  ESP8266 WiFi Library for STM32
  Based on ATC Library Architecture

  Features:
  - WiFi Station & AP Mode
  - TCP/UDP Client & Server
  - HTTP Requests
  - DNS & IP Configuration
  - Multiple Connection Support

  Version:    1.0.0

***********************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "atc.h"
#include <stdbool.h>
#include <stdint.h>

/************************************************************************************************************
**************    Public Definitions
************************************************************************************************************/

#define ESP_TIMEOUT_DEFAULT           5000
#define ESP_TIMEOUT_WIFI_CONNECT      15000
#define ESP_TIMEOUT_SEND              10000
#define ESP_SSID_MAX_LEN              32
#define ESP_PASSWORD_MAX_LEN          64
#define ESP_IP_MAX_LEN                16
#define ESP_MAX_CONNECTIONS           5
#define ESP_RECEIVE_BUFFER_SIZE       2048

// WiFi Modes
#define ESP_WIFI_MODE_NULL            0
#define ESP_WIFI_MODE_STATION         1
#define ESP_WIFI_MODE_AP              2
#define ESP_WIFI_MODE_AP_STATION      3

// Encryption Types
#define ESP_ENCRYPT_OPEN              0
#define ESP_ENCRYPT_WPA_PSK           2
#define ESP_ENCRYPT_WPA2_PSK          3
#define ESP_ENCRYPT_WPA_WPA2_PSK      4

// Connection Types
#define ESP_CONN_TYPE_TCP             0
#define ESP_CONN_TYPE_UDP             1
#define ESP_CONN_TYPE_SSL             2

// Error Codes
#define ESP_OK                        0
#define ESP_ERROR                     -1
#define ESP_ERROR_TIMEOUT             -2
#define ESP_ERROR_INVALID_PARAM       -3
#define ESP_ERROR_NOT_CONNECTED       -4
#define ESP_ERROR_BUSY                -5
#define ESP_ERROR_NO_AP_FOUND         -6
#define ESP_ERROR_WRONG_PASSWORD      -7
#define ESP_ERROR_SEND_FAIL           -8
#define ESP_ERROR_RECEIVE_FAIL        -9
#define ESP_ERROR_MALLOC              -10

/************************************************************************************************************
**************    Public Structures
************************************************************************************************************/

typedef struct
{
  char                SSID[ESP_SSID_MAX_LEN + 1];
  int8_t              RSSI;
  uint8_t             Channel;
  uint8_t             Encryption;
  bool                IsConnected;

} ESP_WiFiInfo_TypeDef;

typedef struct
{
  char                IP[ESP_IP_MAX_LEN + 1];
  char                Gateway[ESP_IP_MAX_LEN + 1];
  char                Netmask[ESP_IP_MAX_LEN + 1];
  char                MAC[18];

} ESP_NetworkInfo_TypeDef;

typedef struct
{
  uint8_t             LinkID;
  uint8_t             Type;              // TCP or UDP
  char                RemoteIP[ESP_IP_MAX_LEN + 1];
  uint16_t            RemotePort;
  uint16_t            LocalPort;
  bool                IsConnected;

} ESP_Connection_TypeDef;

typedef struct
{
  ATC_HandleTypeDef*  hAtc;
  uint8_t             WiFiMode;
  ESP_WiFiInfo_TypeDef      WiFiInfo;
  ESP_NetworkInfo_TypeDef   NetworkInfo;
  ESP_Connection_TypeDef    Connections[ESP_MAX_CONNECTIONS];
  uint8_t             ReceiveBuffer[ESP_RECEIVE_BUFFER_SIZE];
  uint16_t            ReceiveLength;
  bool                IsInitialized;
  bool                EchoEnabled;

} ESP_HandleTypeDef;

typedef void (*ESP_DataReceivedCallback)(uint8_t LinkID, uint8_t* pData, uint16_t Length);

/************************************************************************************************************
**************    Public Functions - Initialization
************************************************************************************************************/

bool    ESP_Init(ESP_HandleTypeDef* hEsp, ATC_HandleTypeDef* hAtc);
bool    ESP_DeInit(ESP_HandleTypeDef* hEsp);
bool    ESP_Reset(ESP_HandleTypeDef* hEsp);
bool    ESP_Test(ESP_HandleTypeDef* hEsp);
bool    ESP_SetEcho(ESP_HandleTypeDef* hEsp, bool Enable);
bool    ESP_GetVersion(ESP_HandleTypeDef* hEsp, char* pVersion, uint16_t MaxLen);

/************************************************************************************************************
**************    Public Functions - WiFi Station Mode
************************************************************************************************************/

bool    ESP_WiFi_SetMode(ESP_HandleTypeDef* hEsp, uint8_t Mode);
bool    ESP_WiFi_Connect(ESP_HandleTypeDef* hEsp, const char* pSSID, const char* pPassword);
bool    ESP_WiFi_Disconnect(ESP_HandleTypeDef* hEsp);
bool    ESP_WiFi_GetStatus(ESP_HandleTypeDef* hEsp);
bool    ESP_WiFi_AutoConnect(ESP_HandleTypeDef* hEsp, bool Enable);
bool    ESP_WiFi_ScanNetworks(ESP_HandleTypeDef* hEsp, ESP_WiFiInfo_TypeDef* pNetworks, uint8_t* pCount, uint8_t MaxCount);

/************************************************************************************************************
**************    Public Functions - WiFi AP Mode
************************************************************************************************************/

bool    ESP_AP_Configure(ESP_HandleTypeDef* hEsp, const char* pSSID, const char* pPassword, uint8_t Channel, uint8_t Encryption);
bool    ESP_AP_GetConnectedDevices(ESP_HandleTypeDef* hEsp, uint8_t* pCount);

/************************************************************************************************************
**************    Public Functions - Network Configuration
************************************************************************************************************/

bool    ESP_Network_GetIP(ESP_HandleTypeDef* hEsp);
bool    ESP_Network_SetStaticIP(ESP_HandleTypeDef* hEsp, const char* pIP, const char* pGateway, const char* pNetmask);
bool    ESP_Network_EnableDHCP(ESP_HandleTypeDef* hEsp, bool Enable);
bool    ESP_Network_GetMAC(ESP_HandleTypeDef* hEsp, char* pMAC);
bool    ESP_Network_SetMAC(ESP_HandleTypeDef* hEsp, const char* pMAC);
bool    ESP_Network_Ping(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t* pTime);

/************************************************************************************************************
**************    Public Functions - TCP/UDP Connection
************************************************************************************************************/

int     ESP_Conn_TCPConnect(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t Port);
int     ESP_Conn_UDPConnect(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t Port, uint16_t LocalPort);
int     ESP_Conn_SSLConnect(ESP_HandleTypeDef* hEsp, const char* pHost, uint16_t Port);
bool    ESP_Conn_Close(ESP_HandleTypeDef* hEsp, uint8_t LinkID);
bool    ESP_Conn_CloseAll(ESP_HandleTypeDef* hEsp);
bool    ESP_Conn_Send(ESP_HandleTypeDef* hEsp, uint8_t LinkID, const uint8_t* pData, uint16_t Length);
int     ESP_Conn_Receive(ESP_HandleTypeDef* hEsp, uint8_t LinkID, uint8_t* pBuffer, uint16_t MaxLen, uint32_t Timeout);
bool    ESP_Conn_SetMultiple(ESP_HandleTypeDef* hEsp, bool Enable);

/************************************************************************************************************
**************    Public Functions - TCP/UDP Server
************************************************************************************************************/

bool    ESP_Server_Start(ESP_HandleTypeDef* hEsp, uint16_t Port, uint32_t Timeout);
bool    ESP_Server_Stop(ESP_HandleTypeDef* hEsp);
bool    ESP_Server_SetMaxConnections(ESP_HandleTypeDef* hEsp, uint8_t MaxConn);

/************************************************************************************************************
**************    Public Functions - HTTP Client
************************************************************************************************************/

int     ESP_HTTP_GET(ESP_HandleTypeDef* hEsp, const char* pURL, char* pResponse, uint16_t MaxLen);
int     ESP_HTTP_POST(ESP_HandleTypeDef* hEsp, const char* pURL, const char* pData, char* pResponse, uint16_t MaxLen);
bool    ESP_HTTP_SetHeader(ESP_HandleTypeDef* hEsp, const char* pHeader, const char* pValue);

/************************************************************************************************************
**************    Public Functions - DNS
************************************************************************************************************/

bool    ESP_DNS_Resolve(ESP_HandleTypeDef* hEsp, const char* pHostname, char* pIP);
bool    ESP_DNS_SetServer(ESP_HandleTypeDef* hEsp, const char* pDNS1, const char* pDNS2);

/************************************************************************************************************
**************    Public Functions - Advanced Features
************************************************************************************************************/

bool    ESP_Sleep_DeepSleep(ESP_HandleTypeDef* hEsp, uint32_t TimeMs);
bool    ESP_Sleep_LightSleep(ESP_HandleTypeDef* hEsp, uint32_t TimeMs);
bool    ESP_SetTransmitPower(ESP_HandleTypeDef* hEsp, uint8_t Power);
bool    ESP_SetCountry(ESP_HandleTypeDef* hEsp, const char* pCountryCode);

/************************************************************************************************************
**************    Public Functions - Callbacks
************************************************************************************************************/

void    ESP_SetDataReceivedCallback(ESP_HandleTypeDef* hEsp, ESP_DataReceivedCallback Callback);
void    ESP_ProcessEvents(ESP_HandleTypeDef* hEsp);

#ifdef __cplusplus
}
#endif
#endif /* _ESP8266_H_ */
