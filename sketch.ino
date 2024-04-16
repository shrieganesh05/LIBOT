/*Defining 1 unit to be circumference of the wheel or 1 revolution.
  Alternate units can also be set for example 1 unit can be 100 cm.*/
#include <Stepper.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include "PubSubClient.h"
const char *ssid = "Wokwi-GUEST"; // Enter your Wi-Fi name
const char *password = "";  // Enter Wi-Fi password
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "spider";
const char *mqtt_username = "shrieganesh";
const char *mqtt_password = "imsg1708";
const int mqtt_port = 1883;
const int chipSelect = 5;
char over[] = "Done";
int i = 0, j = 0;
extern int p = 0, q = 0;
String data;
char value[3][5][6];
char input_book[100];
float x_coordinate[] = {0.5, 1.5, 2.5, 3.5, 4.5};
float y_coordinate[] = {2.0, 4.5, 7.0};
float currentX = -1.5; // Current X position of the robot (in u units)
float currentY = 0.5; // Current Y position of the robot (in u units)
const int motor1StepPin = 12;
const int motor1DirPin = 13;
const int motor2StepPin = 25;
const int motor2DirPin = 26;
const int STEPS_PER_REVOLUTION = 200;
int delta_y, delta_x;
int count = 0;
int step = 157;
const int timerInterval = 1000000;
volatile bool timerFlag = false;
Stepper steppermotor1(STEPS_PER_REVOLUTION, motor1StepPin, motor1DirPin);
Stepper steppermotor2(STEPS_PER_REVOLUTION, motor2StepPin, motor2DirPin);

WiFiClient espClient;
PubSubClient client(espClient);

int LED_STATE = LOW;
hw_timer_t * timer = NULL;      //H/W timer defining (Pointer to the Structure)
void callback(char *topic, byte *payload, unsigned int length);
void check(char book[]);
void return_back();

void IRAM_ATTR onTimer() {
  timerFlag = true;
}
void setup() {
  // Connecting to a WiFi network
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi");
    delay(500);
  }
  Serial.println("Connected to the Wi-Fi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "mqttx_5547db6f";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());

    // Attempt to connect to the MQTT broker
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");

      // Once connected, subscribe to the topic
      client.publish(topic, "Enter a book name");
      while (!client.subscribe(topic));
    }
    else {
      Serial.println("Failed to connect to MQTT broker. Retrying in 5 seconds...");
      Serial.print(client.state());
      delay(5000);
    }
    timer = timerBegin(0, 80, true); // Timer 0, prescaler 80 (1MHz), count up
    timerAttachInterrupt(timer, &onTimer, true); // Attach the interrupt handler
    timerAlarmWrite(timer, timerInterval, true); // Set the timer interval
    timerAlarmEnable(timer); // Enable the timer
  }

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  File csvfile = SD.open("/database.csv");

  if (csvfile) {
    while (csvfile.available()) {
      if (j == 4) {
        data = csvfile.readStringUntil('\n');
      }
      else {
        data = csvfile.readStringUntil(',');
      }
      data.toCharArray(value[i][j], 6);
      if (j == 4) {
        j = 0;
        i++;
      }
      else {
        j++;
      }
    }
    csvfile.close();
  }
  else {
    Serial.println("Error opening file!");
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.println("Message:");
  for (int i = 0; i < length; i++) {
    input_book[i] = (char) payload[i];
  }
  Serial.println(input_book);
  if (memcmp(input_book, over, 4) == 0) {
    return_back();
  }
  else {
    check(input_book);
  }
}



void loop() {
  client.loop();
}

void check(char book[]) {
  int c = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      if (memcmp(book, value[i][j], 5) == 0) {
        c++;
        p = i;
        q = j;
      }
      else {
        continue;
      }
    }
  }
  if (c == 1) {
    move(p, q);
  }
}

void move(int x, int y) {
  float deltaX = x_coordinate[y] - currentX;
  float deltaY = y_coordinate[x] - currentY;
  currentX = x_coordinate[y];
  currentY = y_coordinate[x];
  ++count;
  if (count == 1) {
    down(deltaY);
    turn(step);
    right(deltaX);
    delay(2000);
    Serial.println("Libot Reached Book");
  }
  else {
    if (deltaX == 0 && deltaY == 0) {
      Serial.println("Taking another copy of same book");
    }
    else {
      if (deltaY == 0) {
        if (deltaX > 0) {
          right(deltaX);
        }
        else {
          left(-deltaX);
        }
      }
      else if (deltaY > 0) {
        left(currentX);
        turn(-step);
        down(deltaY);
        turn(step);
        right(x_coordinate[x]);
      }
      else {
        left(currentX);
        turn(-step);
        up(deltaY);
        turn(step);
        right(x_coordinate[x]);
      }
    }
  }
}
void return_back() {
  if (currentX == -1.5 && currentY == 0.5) {
    Serial.println("Already in lobby");
  }
  else {
    left(currentX + 1.5);
    up(currentY - 0.5);
  }
  delay(2000);
  Serial.println("Libot Reached Lobby");
}
void up(float p) {
  delta_y = p * STEPS_PER_REVOLUTION ;
  Serial.print("Up ");
  Serial.println(p);
  steppermotor1.setSpeed(100);
  steppermotor2.setSpeed(100);
  while (delta_y > 0) {
    steppermotor1.step(-1);
    steppermotor2.step(1);
    --delta_y;
  }
  delay(2000);
}
void down(float p) {
  delta_y = p * STEPS_PER_REVOLUTION ;
  Serial.print("Down ");
  Serial.println(p);
  steppermotor1.setSpeed(100);
  steppermotor2.setSpeed(100);
  while (delta_y > 0) {
    steppermotor1.step(1);
    steppermotor2.step(-1);
    --delta_y;
  }
  delay(2000);
}
void left(float q) {
  delta_x = q * STEPS_PER_REVOLUTION ;
  Serial.print("Left ");
  Serial.println(q);
  steppermotor1.setSpeed(100);
  steppermotor2.setSpeed(100);
  while (delta_x > 0) {
    steppermotor1.step(-1);
    steppermotor2.step(1);
    --delta_x;
  }
  delay(2000);
}
void right(float q) {
  delta_x = q * STEPS_PER_REVOLUTION ;
  Serial.print("Right ");
  Serial.println(q);
  steppermotor1.setSpeed(100);
  steppermotor2.setSpeed(100);
  while (delta_x > 0) {
    steppermotor1.step(1);
    steppermotor2.step(-1);
    --delta_x;
  }
  delay(2000);
}
void turn(int step) {
  steppermotor1.setSpeed(100);
  steppermotor2.setSpeed(100);
  if (timerFlag) {
    timerFlag = false; // Reset the flag

    // Code to be executed every timerInterval (1 second) goes here
    steppermotor2.step(step);
    steppermotor1.step(0);
  }
  delay(2000);
}