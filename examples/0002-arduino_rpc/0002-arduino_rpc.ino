// This sketch demonstrates connecting and receiving RPC calls from 
// ThingsBoard using ThingsBoard SDK.
//
// Hardware:
//  - Arduino Uno
//  - ESP8266 connected to Arduino Uno

#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>


#if THINGSBOARD_ENABLE_PROGMEM
constexpr char WIFI_SSID[] PROGMEM = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] PROGMEM = "YOUR_WIFI_PASSWORD";
#else
constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
#endif

// See https://thingsboard.io/docs/getting-started-guides/helloworld/
// to understand how to obtain an access token
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char TOKEN[] PROGMEM = "YOUR_DEVICE_ACCESS_TOKEN";
#else
constexpr char TOKEN[] = "YOUR_DEVICE_ACCESS_TOKEN";
#endif

// Thingsboard we want to establish a connection too
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char THINGSBOARD_SERVER[] PROGMEM = "demo.thingsboard.io";
#else
constexpr char THINGSBOARD_SERVER[] = "demo.thingsboard.io";
#endif

// MQTT port used to communicate with the server, 1883 is the default unencrypted MQTT port,
// whereas 8883 would be the default encrypted SSL MQTT port
#if THINGSBOARD_ENABLE_PROGMEM
constexpr uint16_t THINGSBOARD_PORT PROGMEM = 1883U;
#else
constexpr uint16_t THINGSBOARD_PORT = 1883U;
#endif

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
#if THINGSBOARD_ENABLE_PROGMEM
constexpr uint16_t MAX_MESSAGE_SIZE PROGMEM = 128U;
#else
constexpr uint16_t MAX_MESSAGE_SIZE = 128U;
#endif

// Baud rate for the debugging serial connection
// If the Serial output is mangled, ensure to change the monitor speed accordingly to this variable
#if THINGSBOARD_ENABLE_PROGMEM
constexpr uint32_t SERIAL_DEBUG_BAUD PROGMEM = 9600U;
constexpr uint32_t SERIAL_ESP8266_DEBUG_BAUD PROGMEM = 9600U;
#else
constexpr uint32_t SERIAL_DEBUG_BAUD = 9600U;
constexpr uint32_t SERIAL_ESP8266_DEBUG_BAUD = 9600U;
#endif

#if THINGSBOARD_ENABLE_PROGMEM
constexpr const char RPC_JSON_METHOD[] PROGMEM = "example_json";
constexpr char CONNECTING_MSG[] PROGMEM = "Connecting to: (%s) with token (%s)";
constexpr const char RPC_TEMPERATURE_METHOD[] PROGMEM = "example_set_temperature";
constexpr const char RPC_SWITCH_METHOD[] PROGMEM = "example_set_switch";
constexpr const char RPC_TEMPERATURE_KEY[] PROGMEM = "temp";
constexpr const char RPC_SWITCH_KEY[] PROGMEM = "switch";
#else
constexpr const char RPC_JSON_METHOD[] = "example_json";
constexpr char CONNECTING_MSG[] = "Connecting to: (%s) with token (%s)";
constexpr const char RPC_TEMPERATURE_METHOD[] = "example_set_temperature";
constexpr const char RPC_SWITCH_METHOD[] = "example_set_switch";
constexpr const char RPC_TEMPERATURE_KEY[] = "temp";
constexpr const char RPC_SWITCH_KEY[] PROGMEM = "switch";
#endif


// Serial driver for ESP
SoftwareSerial soft(2U, 3U); // RX, TX
// Initialize the Ethernet client object
WiFiEspClient espClient;
// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);
// Initialize ThingsBoard instance with the maximum needed buffer size and the adjusted maximum amount of rpc key value pairs we want to send as a response
ThingsBoardSized<Default_Fields_Amount, 3U, Default_Attributes_Amount, 5U> tb(mqttClient, MAX_MESSAGE_SIZE);

// Statuses for subscribing to rpc
bool subscribed = false;


/// @brief Initalizes WiFi connection,
// will endlessly delay until a connection has been successfully established
void InitWiFi() {
#if THINGSBOARD_ENABLE_PROGMEM
  Serial.println(F("Connecting to AP ..."));
#else
  Serial.println("Connecting to AP ...");
#endif
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    // Delay 500ms until a connection has been successfully established
    delay(500);
#if THINGSBOARD_ENABLE_PROGMEM
    Serial.print(F("."));
#else
    Serial.print(".");
#endif
  }
#if THINGSBOARD_ENABLE_PROGMEM
  Serial.println(F("Connected to AP"));
#else
  Serial.println("Connected to AP");
#endif
#if ENCRYPTED
  espClient.setCACert(ROOT_CERT);
#endif
}

/// @brief Reconnects the WiFi uses InitWiFi if the connection has been removed
/// @return Returns true as soon as a connection has been established again
bool reconnect() {
  // Check to ensure we aren't connected yet
  const uint8_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return true;
}

void setup() {
  // initialize serial for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  // initialize serial for ESP module
  soft.begin(SERIAL_ESP8266_DEBUG_BAUD);
  delay(1000);

  // initialize ESP module
  WiFi.init(&soft);
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
#if THINGSBOARD_ENABLE_PROGMEM
    Serial.println(F("WiFi shield not present"));
#else
    Serial.println("WiFi shield not present");
#endif
    // don't continue
    while (true);
  }
  InitWiFi();
}

/// @brief Processes function for RPC call "example_json"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processGetJson(const JsonVariantConst &data, JsonDocument &response) {
#if THINGSBOARD_ENABLE_PROGMEM
  Serial.println(F("Received the json RPC method"));
#else
  Serial.println("Received the json RPC method");
#endif

  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

/// @brief Processes function for RPC call "example_set_temperature"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processTemperatureChange(const JsonVariantConst &data, JsonDocument &response) {
#if THINGSBOARD_ENABLE_PROGMEM
  Serial.println(F("Received the set temperature RPC method"));
#else
  Serial.println("Received the set temperature RPC method");
#endif

  // Process data
  const float example_temperature = data[RPC_TEMPERATURE_KEY];

#if THINGSBOARD_ENABLE_PROGMEM
  Serial.print(F("Example temperature: "));
#else
  Serial.print("Example temperature: ");
#endif
  Serial.println(example_temperature);

  // Ensure to only pass values do not store by copy, or if they do increase the MaxRPC template parameter accordingly to ensure that the value can be deserialized.RPC_Callback.
  // See https://arduinojson.org/v6/api/jsondocument/add/ for more information on which variables cause a copy to be created
  response["string"] = "exampleResponseString";
  response["int"] = 5;
  response["float"] = 5.0f;
  response["double"] = 10.0d;
  response["bool"] = true;
}

/// @brief Processes function for RPC call "example_set_switch"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processSwitchChange(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the set switch method");

  // Process data
  const bool switch_state = data[RPC_SWITCH_KEY];

#if THINGSBOARD_ENABLE_PROGMEM
  Serial.print(F("Example switch state: "));
#else
  Serial.print("Example switch state: ");
#endif
  Serial.println(switch_state);

  response.set(22.02);
}

void loop() {
  delay(1000);

  if (!reconnect()) {
    return;
  }

  if (!tb.connected()) {
    // Reconnect to the ThingsBoard server,
    // if a connection was disrupted or has not yet been established
    char message[Helper::detectSize(CONNECTING_MSG, THINGSBOARD_SERVER, TOKEN)];
    snprintf(message, sizeof(message), CONNECTING_MSG, THINGSBOARD_SERVER, TOKEN);
    Serial.println(message);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
#if THINGSBOARD_ENABLE_PROGMEM
      Serial.println(F("Failed to connect"));
#else
      Serial.println("Failed to connect");
#endif
      return;
    }
  }

  if (!subscribed) {
#if THINGSBOARD_ENABLE_PROGMEM
    Serial.println(F("Subscribing for RPC..."));
#else
    Serial.println("Subscribing for RPC...");
#endif
    constexpr uint8_t CALLBACK_SIZE = 3U;
    const RPC_Callback callbacks[CALLBACK_SIZE] = {
      // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
      { RPC_JSON_METHOD,           processGetJson },
      // Requires additional memory in the JsonDocument for 5 key-value pairs that do not copy their value into the JsonDocument itself
      { RPC_TEMPERATURE_METHOD,    processTemperatureChange },
      // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory
      { RPC_SWITCH_METHOD,         processSwitchChange }
    };
    // Perform a subscription. All consequent data processing will happen in
    // processTemperatureChange() and processSwitchChange() functions,
    // as denoted by callbacks array.
    const RPC_Callback* begin = callbacks;
    const RPC_Callback* end = callbacks + CALLBACK_SIZE;
    if (!tb.RPC_Subscribe(begin, end)) {
#if THINGSBOARD_ENABLE_PROGMEM
      Serial.println(F("Failed to subscribe for RPC"));
#else
      Serial.println("Failed to subscribe for RPC");
#endif
      return;
    }

#if THINGSBOARD_ENABLE_PROGMEM
    Serial.println(F("Subscribe done"));
#else
    Serial.println("Subscribe done");
#endif
    subscribed = true;
  }

  tb.loop();
}
