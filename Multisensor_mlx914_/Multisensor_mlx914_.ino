/*
 * This code is created by pad89
 * Change Wifi SSID and Password to connect to your Wifi conection
 * Change mqtt server, user, and password to match your server
 * Change mqtt topic to uniqe topic that represent your data
 * Add specific library to upload sketch to esp8266 board
 * Wiring is detailed in Multisensor_Sketch_wiring.png or Multisensor_Sketch_wiring.fzz
 */

#include "Wire.h"
#include <ESP8266WiFi.h>              // ESP8266
#include <Adafruit_MLX90614.h>        // MLX sensor 
#include <LiquidCrystal_I2C.h>        // LCD Screen
#include <PubSubClient.h>             // MQTT

#define wifi_ssid "BSLA"              // Wifi SSID
#define wifi_password "harlistrik08"  // Wifi Password

#define mqtt_server "192.168.0.10"    // IP address NUC
#define mqtt_user "hassio"            // MQTT username
#define mqtt_password "myhassio"      // MQTT password

// Mqtt topic sensor alert
#define temperature_topic_alert "sensor/temperatureAlertBoilerIDFA"

// Mqtt topic sensor RST
#define temperature_topic_R "sensor/temperatureRBoilerIDFA"
#define temperature_topic_S "sensor/temperatureSBoilerIDFA"
#define temperature_topic_T "sensor/temperatureTBoilerIDFA"

// Mqtt topic delta RST
#define temperature_topic_RS "sensor/temperatureRSBoilerIDFA"
#define temperature_topic_RT "sensor/temperatureRTBoilerIDFA"
#define temperature_topic_ST "sensor/temperatureSTBoilerIDFA"

#define MUX_Address 0x70 // TCA9548A Encoders address

Adafruit_MLX90614 mlx_1;
Adafruit_MLX90614 mlx_2;
Adafruit_MLX90614 mlx_3;

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    Serial.begin(9600);
    
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    
    Wire.setClock(100000);
    Wire.begin(4, 5);

    lcd.begin(16, 2);
    lcd.backlight();
    
    tcaselect(1);
    mlx_1.begin();
    
    tcaselect(2);
    mlx_2.begin();
    
    tcaselect(3);
    mlx_3.begin();
}

// Initialize I2C bus with TCA9548A I2C Multiplexer
void tcaselect(uint8_t i2c_bus) {
    if (i2c_bus > 7) return;
    
    Wire.beginTransmission(MUX_Address);
    Wire.write(1 << i2c_bus);
    Wire.endTransmission();
}

// Initialize wifi connection
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Connect to mqtt server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long lastMsg = 0;
int alert = 0; // normal - blue

float absDelta(float x, float y) {
  float z = x - y;
  return abs(z);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  bool cond1 = false;
  bool cond2 = false;
  bool cond3 = false;
  bool cond4 = false;
  
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    tcaselect(1);
    float temp_R = mlx_1.readObjectTempC();
    Serial.print("Temperature R:");
    Serial.println(String(temp_R).c_str());
    client.publish(temperature_topic_R, String(temp_R).c_str(), true);
    
    tcaselect(2);
    float temp_S = mlx_2.readObjectTempC();
    Serial.print("Temperature S:");
    Serial.println(String(temp_S).c_str());
    client.publish(temperature_topic_S, String(temp_S).c_str(), true);
  
    tcaselect(3);
    float temp_T = mlx_3.readObjectTempC();
    Serial.print("Temperature T:");
    Serial.println(String(temp_T).c_str());
    client.publish(temperature_topic_T, String(temp_T).c_str(), true);

    Serial.println();

    float temp_RS = absDelta(temp_R, temp_S);
    Serial.print("Temperature Delta RS:");
    Serial.println(String(temp_RS).c_str());
    client.publish(temperature_topic_RS, String(temp_RS).c_str(), true);

    float temp_RT = absDelta(temp_R, temp_T);
    Serial.print("Temperature Delta RT:");
    Serial.println(String(temp_RT).c_str());
    client.publish(temperature_topic_RT, String(temp_RT).c_str(), true);

    float temp_ST = absDelta(temp_S, temp_T);
    Serial.print("Temperature Delta ST:");
    Serial.println(String(temp_ST).c_str());
    client.publish(temperature_topic_ST, String(temp_ST).c_str(), true);

    // temperature alert condition
    // critical - white
    bool cond4 = ((temp_RS > 40 || temp_RT > 40 || temp_ST > 40) 
              || ((temp_RS > 20 || temp_RT > 20 || temp_ST > 20) && (temp_R > 75 || temp_S > 75 || temp_T > 75)) 
              || ((temp_RS > 5 || temp_RT > 5 || temp_ST > 5) && (temp_R > 100 || temp_S > 100 || temp_T > 100)));
    // high - red
    bool cond3 = ((((temp_RS > 20 && temp_RS <=40) || (temp_RT > 20 && temp_RT <=40) || (temp_ST > 20 && temp_ST <=40)) && (temp_R <= 75 || temp_S <= 75 || temp_T <= 75)) 
              || (((temp_RS > 10 && temp_RS <=20) || (temp_RT > 10 && temp_RT <=20) || (temp_ST > 10 && temp_ST <=20)) && ((temp_R > 75 && temp_R <= 100) || (temp_S > 75 && temp_S <= 100) || (temp_S > 75 && temp_S <= 100)))
              || ((temp_RS <= 5 || temp_RT <= 5 || temp_ST <= 5) && (temp_R > 100 || temp_S > 100 || temp_T > 100)));
    // medium - yellow
    bool cond2 = ((((temp_RS > 10 && temp_RS <=20) || (temp_RT > 10 && temp_RT <=20) || (temp_ST > 10 && temp_ST <=20)) && (temp_R <= 75 || temp_S <= 75 || temp_T <= 75)) 
              || (((temp_RS > 5 && temp_RS <=10) || (temp_RT > 5 && temp_RT <=10) || (temp_ST > 5 && temp_ST <=10)) && ((temp_R > 75 && temp_R <= 100) || (temp_S > 75 && temp_S <= 100) || (temp_S > 75 && temp_S <= 100))));
    // low - green
    bool cond1 = ((((temp_RS > 5 && temp_RS <=10) || (temp_RT > 5 && temp_RT <=10) || (temp_ST > 5 && temp_ST <=10)) && (temp_R <= 75 || temp_S <= 75 || temp_T <= 75)) 
              || ((temp_RS <= 5 || temp_RT <= 5 || temp_ST <= 5) && ((temp_R > 75 && temp_R <= 100) || (temp_S > 75 && temp_S <= 100) || (temp_S > 75 && temp_S <= 100))));
    
    if (cond4) {
      alert = 4;
    } else if (cond3){
      alert = 3;
    } else if (cond2) {
      alert = 2;
    } else if (cond1) {
      alert = 1;
    } else {
      alert = 0;
    }
    
    Serial.print("\nTemperature Alert:");
    Serial.println(String(alert).c_str());
    client.publish(temperature_topic_alert, String(alert).c_str(), true);

    // LCD print
    lcd.setCursor(0,0);
    lcd.print("R:");
    lcd.print(String(temp_R).c_str());
    
    lcd.setCursor(9,0);
    lcd.print("S:");
    lcd.print(String(temp_S).c_str());
    
    lcd.setCursor(0,1);
    lcd.print("T:");
    lcd.print(String(temp_T).c_str());
    
    Serial.println();
  }
    
}
