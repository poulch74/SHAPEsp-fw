#ifndef tmrlib_h
#define tmrlib_h

void handleIndex2(AsyncWebServerRequest *request);

void handleFavicon(AsyncWebServerRequest *request);

void handleLogin(AsyncWebServerRequest *request);

void handleLogoff(AsyncWebServerRequest *request);

void handleSecurity(AsyncWebServerRequest *request);

void handleIndex1(AsyncWebServerRequest *request);

void handleWiFiSettings(AsyncWebServerRequest *request);

String DbgArgMsg(AsyncWebServerRequest *request);

uint16_t crc16(const uint8_t *msg, int msg_len);

void handleNotFound(AsyncWebServerRequest *request);

bool is_auth();

bool ReadConfig();

void WriteConfig(bool def);

bool ReadTmrPrg();

void SaveTmrPrg(bool def);


#endif