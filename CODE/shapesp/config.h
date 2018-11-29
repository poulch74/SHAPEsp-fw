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
   uint8_t   b[2002];
} ESP_CFG;
#pragma pack(pop)

struct JsonBundle {
  public:
    void parse(const char* json) {
      _jsonVariant = _jsonBuffer.parseObject(json);
    }

    void clear() {
      _jsonBuffer.clear();
    }


    const JsonObject& root() const { 
      return _jsonVariant;
    }

  private:
    DynamicJsonBuffer _jsonBuffer;
    JsonVariant _jsonVariant;
};

JsonBundle config;

//DynamicJsonBuffer jsonBuffer;
//JsonObject *config;
