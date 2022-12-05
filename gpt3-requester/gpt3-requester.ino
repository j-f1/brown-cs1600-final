#include <WiFi101.h>
#include <HttpClient.h>

// WiFi setup
char ssid[] = "Brown-Guest";
char password[] = "";
int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
HttpClient httpClient(wifiClient);
bool sending = false;
bool success = false;

// GPT-3 API setup
char openAiHostname[] = "https://api.openai.com";
char completionsEndpoint[] = "/v1/completions";

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
    if (httpClient.responseStatusCode() >= 200 && httpClient.responseStatusCode() <= 299) {
    
    } else {
      Serial.print("Response returned a non-success status code: ");
      Serial.println(httpClient.responseStatusCode());
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(err);
  }

  httpClient.stop();
  delay(2500);
}

// ======================= GPT-3 calls ========================

JSONVar createOpenAiApiRequest(String unvoweledWord) {
  JSONVar requestPayload;

  String basePrompt = "These words are missing their vowels. Please fill them in.\n  mpt -> empty\n  dtrrt -> deteriorate\n  dmrct -> demarcate\n  ";
  String prompt = basePrompt + unvoweledWord.toLowerCase() + " ->";

  requestPayload["model"] = "text-ada-001";
  requestPayload["prompt"] = prompt;
  requestPayload["max_tokens"] = "20";
  requestPayload["temperature"] = 0;
  requestPayload["stop"] = "\n";

  return requestPayload;
}

JSONVar getTextCompletion() {
  int err = httpClient.post(openAiHostname, completionsEndpoint);
  sending = true;
  success = false;

  if (!err) {
    if (httpClient.responseStatusCode() >= 200 && httpClient.responseStatusCode() <= 299) {
      Serial.println("Successful response received");
      success = true;
      // TODO: handle payload here, see https://github.com/amcewen/HttpClient/blob/master/HttpClient.h
    } else {
      Serial.print("Response returned a non-success status code: ");
      Serial.println(httpClient.responseStatusCode());
    }
  } else {
    Serial.print("Request received error: ");
    switch (err) {
      case HTTP_ERROR_CONNECTION_FAILED:
        Serial.println("connection failed (-1)");
        break;
      case HTTP_ERROR_API:
        Serial.println("unexpected call (-2); HttpClient class used incorrectly.");
        break;
      case HTTP_ERROR_TIMED_OUT:
        Serial.println("timed out (-3)");
        break;
      case HTTP_ERROR_INVALID_RESPONSE:
        Serial.println("invalid response (-4); is it an HTTP server?");
        break;
      default:
        Serial.print("unexpected error (");
        Serial.print(err);
        Serial.println(")");
        break;
    }
  }
  sending = false;
}
