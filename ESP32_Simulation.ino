#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

// WiFi & MQTT Configuration
const char* ssid = "DSPD LAB-2.4G"; //Wi-FI Name and It's should be 2.4G not 5G
const char* password = "dspd@4321"; //Wi-FI Password
const char* mqtt_server = "10.3.14.168";  //"10.3.14.61";  // PC IP (MQTT broker) //192.168.238.212

WiFiClient espClient;
PubSubClient client(espClient);

// NTP Settings
//const char* ntpServer = "time.nist.gov"; //"129.6.15.28"; //"pool.ntp.org";  // Reliable global NTP pool
const long gmtOffset_sec = 19800;        // GMT+5:30 for IST
const int daylightOffset_sec = 0;

unsigned long lastMsgTime = 0;
const long interval = 3000;

void setup_wifi() {
  Serial.println("🔌 Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++retries > 20) {
      Serial.println("\n❌ WiFi Failed!");
      return;
    }
  }

  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void sync_time() {
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.google.com", "time.nist.gov"); // "129.6.15.28");
  delay(100);
  Serial.print("⏳ Syncing time");
  struct tm timeinfo;
  for (int i = 0; i < 10; i++) {
    if (getLocalTime(&timeinfo)) {
      Serial.println("\n✅ Time synchronized.");
      return;
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\n❌ Failed to synchronize time.");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("🔁 Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("✅ Connected to MQTT broker!");
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.print(client.state());
      Serial.println(" — retrying in 3 sec");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  delay(2000);     // Let WiFi stabilize
  sync_time();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  unsigned long now = millis();
  if (now - lastMsgTime > interval) {
    lastMsgTime = now;

    float t = random(25, 45);   // Simulated temperature
    float h = random(40, 80);   // Simulated humidity


    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char dateStr[11];  // YYYY-MM-DD
      char timeStr[9];   // HH:MM:SS
      strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

      
      String payload = "{";
      payload += "\"date\":\"" + String(dateStr) + "\",";
      payload += "\"time\":\"" + String(timeStr) + "\",";
      payload += "\"temp\":" + String(t, 2) + ",";
      payload += "\"humid\":" + String(h, 2);
      payload += "}";
     

      Serial.println("📤 Sending MQTT message: " + payload);
      client.publish("Sensor_data", payload.c_str());
    } else {
      Serial.println("⚠️ Time not available, skipping message.");
    }
  }
}
