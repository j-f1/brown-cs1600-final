#include "shared.h"

WiFiClient client;
int status = WL_IDLE_STATUS;  // the WiFi radio's status

void setup_wifi() {
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to: ");
    Serial.println(ssid);
    if (strlen(pass)) {
      status = WiFi.begin(ssid, pass);
    } else {
      status = WiFi.begin(ssid);
    }
    delay(1000);
  }
  Serial.println("Connected to WiFi!");
}

// Requests completions from GPT-3, writing a space-separated list of words
// into `result` on success.
bool makeRequest(const char *word, char *result, size_t result_len) {
  Serial.println("Sending request");
  if (client.connect(host, 80)) {
    client.println("PUT / HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("Content-Type: text/plain; charset=utf-8");
    client.print("Auth-Token: ");
    client.println(api_key);
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(strlen(word));
    client.println();
    client.println(word);
    Serial.println("Sent request");
  } else {
    Serial.println("Failed to fetch webpage");
    return false;
  }

  char sep[] = "\r\n\r\n";
  int sep_idx = 0;
  bool copying = false;
  size_t result_idx = 0;
  while (client.connected()) {
    if (!client.available()) continue;
    char c = client.read();
    if (copying) {
      if (result_idx == result_len - 1) {
        result[result_idx] = 0;
        return true;
      }
      result[result_idx++] = c;
    } else if (c == sep[sep_idx]) {
      sep_idx++;
      if (sep_idx == 4) {
        copying = true;
      }
    } else {
      sep_idx = 0;
    }
  }
  Serial.println("received all!");
  result[result_idx] = 0;
  return true;
}
