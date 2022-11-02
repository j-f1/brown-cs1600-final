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

char host[] = "dummyjson.com";
char endpoint[] = "/todos/1";

void setup() {
  // Initialize serial connection
  Serial.begin(9600);
  while (!Serial);

  // Attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect");
    status = WiFi.begin(ssid);
    if (status != WL_CONNECTED) {
      delay(5000); 
    }
  }
  Serial.println("Connected to wifi");
}

void loop() {
  Serial.println("Sending get request");
  int err = httpClient.get(host, endpoint);

  if (!err) {
    Serial.println(httpClient.responseStatusCode());
  } else {
    Serial.print("Error code: ");
    Serial.println(err);
  }

  httpClient.stop();
  delay(2500);
}
