#ifndef tmrlib_h
#define tmrlib_h

size_t sendFile(String fname, String content_type);

void handleIndex2();

void handleFavicon();

void handleLogin();

void handleLogoff();

void handleSecurity();

void handleIndex1();

void handleWiFiSettings();

String DbgArgMsg();

uint16_t crc16(const uint8_t *msg, int msg_len);

void UrlRedirect(String url);

void handleNotFound();

bool is_auth();

bool ReadConfig();

void WriteConfig(bool def);

bool ReadTmrPrg();

void SaveTmrPrg(bool def);


#endif