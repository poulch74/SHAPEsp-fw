typedef struct Param
{
   const char    *name;
   const char    *value;
} ESP_PARAM, *PESP_PARAM;

#pragma pack(push,1)

typedef struct __ESP_CFG_
{
   uint8_t payload[2048];
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


    JsonObject& root() { 
      return _jsonVariant;
    }

  private:
    DynamicJsonBuffer _jsonBuffer;
    JsonVariant _jsonVariant;
};

JsonBundle config;

