typedef struct Param
{
   const char    *name;
   const char    *value;
} ESP_PARAM, *PESP_PARAM;

#pragma pack(push,1)
typedef struct espcfg
{
   uint16_t crc;
   char payload[2000];

} ESP_CFG_S, *PESP_CFG_S;

typedef union __ESP_CFG_U
{
   ESP_CFG_S s;
   uint8_t   b[2048];
} ESP_CFG;
#pragma pack(pop)

DynamicJsonBuffer jsonBuffer;
JsonObject *config;
