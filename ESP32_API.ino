#include <WiFi.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "ESP32_Sorter";
const char* password = "12345678";
WiFiServer server(80);

// Motor and Servo Configurations
#define DIR 26
#define STEP 25
#define ENA 27
#define SERVO_PIN 32
Servo servo;

// Sorting Configuration
#define STEPS_PER_REV 200
#define STEPS_PER_POSITION 40
int current_position = 0;

// Request Queue Management
#define MAX_QUEUE_SIZE 10
struct SortRequest {
  String requestId;
  String grupo;
  bool processed;
};
SortRequest requestQueue[MAX_QUEUE_SIZE];
int queueFront = 0;
int queueRear = -1;
int queueCount = 0;

// Function Prototypes
void enqueueRequest(const String& requestId, const String& grupo);
bool dequeueRequest(SortRequest& request);
void moveToPositionFromGrupo(const char* grupo);
void moveToPosition(int target);
void openCompuerta();
void processRequestQueue();

void setup() {
  Serial.begin(115200);
  
  // WiFi Setup
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point Started. IP: ");
  Serial.println(IP);
  server.begin();
  
  // Motor and Servo Initialization
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(ENA, OUTPUT);
  digitalWrite(ENA, LOW);
  
  servo.attach(SERVO_PIN);
  servo.write(0);
}

void loop() {
  // Handle new client connections
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client Connected");
    
    // Read the HTTP request
    String request = client.readStringUntil('\r');
    Serial.println("Request Received: " + request);
    
    // Validate request method and endpoint
    if (request.startsWith("POST /sort ")) {
      // Parse headers to find Request ID and Content-Length
      String requestId = "";
      int contentLength = -1;
      
      while (client.connected()) {
        String header = client.readStringUntil('\r');
        header.trim();
        
        if (header.length() == 0) break;
        
        if (header.startsWith("X-Request-ID:")) {
          requestId = header.substring(14);
        }
        
        if (header.startsWith("Content-Length:")) {
          contentLength = header.substring(15).toInt();
        }
      }

      // If Content-Length is not provided or invalid, send error response
      if (contentLength <= 0) {
        client.println("HTTP/1.1 400 Bad Request");
        client.println("Content-Type: application/json");
        client.println();
        client.print("{\"error\":\"Invalid or missing Content-Length\"}");
        client.stop();
        return;
      }
      
      // Read JSON payload
      String jsonPayload = "";
      for (int i = 0; i < contentLength; i++) {
        if (client.available()) {
          char c = client.read();
          jsonPayload += c;
        }
      }
      
      // Parse JSON
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, jsonPayload);
      
      if (!error && doc["data"]["grupo"]) {
        const char* grupo = doc["data"]["grupo"];
        
        // Enqueue request
        enqueueRequest(requestId, String(grupo));
        
        // Send success response
        client.println("HTTP/1.1 202 Accepted");
        client.println("Content-Type: application/json");
        client.println();
        client.print("{\"status\":\"queued\",\"requestId\":\"" + requestId + "\"}");
      } else {
        // Send error response
        client.println("HTTP/1.1 400 Bad Request");
        client.println("Content-Type: application/json");
        client.println();
        client.print("{\"error\":\"Invalid request\"}");
      }
      
      client.stop();
    } else {
      // If the request method or endpoint is invalid
      client.println("HTTP/1.1 405 Method Not Allowed");
      client.println("Content-Type: application/json");
      client.println();
      client.print("{\"error\":\"Method not allowed\"}");
      client.stop();
    }
  }
  
  // Process request queue
  processRequestQueue();
  
  delay(50);  // Small delay to prevent CPU overload
}

void processRequestQueue() {
  if (queueCount > 0) {
    SortRequest request;
    if (dequeueRequest(request)) {
      Serial.println("Processing request: " + request.requestId);
      moveToPositionFromGrupo(request.grupo.c_str());
    }
  }
}

void enqueueRequest(const String& requestId, const String& grupo) {
  if (queueCount < MAX_QUEUE_SIZE) {
    queueRear = (queueRear + 1) % MAX_QUEUE_SIZE;
    requestQueue[queueRear] = {requestId, grupo, false};
    queueCount++;
    Serial.println("Request enqueued: " + requestId);
  } else {
    Serial.println("Queue full, request dropped");
  }
}

bool dequeueRequest(SortRequest& request) {
  if (queueCount > 0) {
    request = requestQueue[queueFront];
    queueFront = (queueFront + 1) % MAX_QUEUE_SIZE;
    queueCount--;
    return true;
  }
  return false;
}

void moveToPositionFromGrupo(const char* grupo) {
  int target_position = 0;

  if (strcmp(grupo, "plastic") == 0) {
    target_position = 1 * STEPS_PER_POSITION;
  } else if (strcmp(grupo, "other") == 0) {
    target_position = 2 * STEPS_PER_POSITION;
  } else if (strcmp(grupo, "paper") == 0) {
    target_position = 3 * STEPS_PER_POSITION;
  } else if (strcmp(grupo, "bio") == 0) {
    target_position = 4 * STEPS_PER_POSITION;
  } else {
    target_position = 5 * STEPS_PER_POSITION; // Default (unknown group)
  }

  moveToPosition(target_position);
}

void moveToPosition(int target) {
  int steps_to_move = target - current_position;
  if (steps_to_move != 0) {
    digitalWrite(DIR, steps_to_move > 0 ? HIGH : LOW);
    steps_to_move = abs(steps_to_move);
    
    for (int i = 0; i < steps_to_move; i++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(1000);
      digitalWrite(STEP, LOW);
      delayMicroseconds(1000);
    }
    
    current_position = target;
    delay(1000);
    openCompuerta();
  }
  
  Serial.print("Current Position: ");
  Serial.println(current_position / STEPS_PER_POSITION + 1);
}

void openCompuerta() {
  servo.write(90);
  delay(1000);
  servo.write(0);
  Serial.println("Gate opened and closed");
}
