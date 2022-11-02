#include <WiFi101.h>
#include <HttpClient.h>

// WiFi setup
char ssid[] = "Brown-Guest";
char password[] = "";
int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
HttpClient httpClient(wifiClient);

// GPT-3 API setup
//char host[] = "https://api.openai.com";
//char endpoint[] = "/v1/completions";

char host[] = "https://dummyjson.com/todos";
char endpoint[] = "/todos";

void setup() {
  // Initialize serial connection
  Serial.begin(9600);
  while (!Serial);

  // Attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect");
    status = WiFi.begin(ssid);
    Serial.println("Test");
    delay(5000);
  }
  Serial.println("Connected to wifi");
}

void loop() {
  Serial.println("Sending get request");
  int responseCode = httpClient.get(host, endpoint);
  Serial.print("Response code: ");
  Serial.println(responseCode);
  delay(2500);
}
