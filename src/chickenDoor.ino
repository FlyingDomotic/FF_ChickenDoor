#define CHICKEN_DOOR_VERSION "24.5.5-8"   // Version of this code

#include <ESP8266WiFi.h>			      // Wifi (embeded)
#include <Arduino.h>                // Arduino (embeded)
#include <Wire.h>                   // I2C (embeded)
#include <AsyncMqttClient.h>	      // Asynchronous MQTT client https://github.com/marvinroger/async-mqtt-client
#include <ArduinoJson.h>			      // JSON documents https://github.com/bblanchon/ArduinoJson
#include <Adafruit_INA219.h>        // INA219 current sensor https://github.com/adafruit/Adafruit_INA219
#include <NtpClientLib.h>			      // NTP client https://github.com/gmag11/NtpClient
#include <JC_sunrise.h>             // Sun rise/set computations https://github.com/JChristensen/JC_Sunrise
#include <chickenDoorParameters.h>  // Constants for this program

//    ### Variables ###

#define XQUOTE(x) #x
#define QUOTE(x) XQUOTE(x)

//  *** Wifi stuff ***
WiFiEventHandler onStationModeConnectedHandler; // Event handler called when WiFi is connected
WiFiEventHandler onStationModeGotIPHandler;     // Event handler called when WiFi got an IP

//  *** Asynchronous MQTT client ***
AsyncMqttClient mqttClient;                       // Asynchronous MQTT client
bool mqttStatusReceived = false;                  // True if we received initial status at startup

//  *** NTP & Sun set/rise***
JC_Sunrise sunParams {latitude, longitude, zenith}; // Sun parameters (lat, lon, type of sunset/rise required)
unsigned long lastNtpTest = 0;                      // Time of last NTP test
int lastComputeDay = 0;                             // Day number of last computation (to reload sun set/rise when day changes or at startup)
time_t nowTime = 0;                                 // Last current time (from NTP loop)
time_t sunOpen  = 0;                                // Today's time to open door
time_t sunClose = 0;                                // Today's time to close door
uint16_t nowInt = 0;                                // Now in integer minutes (hours * 100) + minutes
uint16_t sunOpenInt = 0;                            // Sun door open in integer minutes (hours * 100) + minutes
uint16_t sunCloseInt = 0;                           // Sun door close in integer minutes (hours * 100) + minutes

// Sun states (self explained)
enum sunStates {
  sunUnknown, 
  beforeOpen,  
  betwenOpenAndClose,
  afterClose
};
const char sunStatesText0[] PROGMEM = "Unknown";
const char sunStatesText1[] PROGMEM = "Before open";
const char sunStatesText2[] PROGMEM = "Between open and close";
const char sunStatesText3[] PROGMEM = "After close";
const char *const sunStatesTable[] PROGMEM = {sunStatesText0, sunStatesText1, sunStatesText2, sunStatesText3};

//  *** Door ***

// Door states (self explained)
enum doorStates {
	doorUnknown,
  doorClosed,
	doorOpening,
	doorOpened,
	doorClosing,
  doorStopped
};

const char doorStatesText0[] PROGMEM = "Unknown";
const char doorStatesText1[] PROGMEM = "Closed";
const char doorStatesText2[] PROGMEM = "Opening";
const char doorStatesText3[] PROGMEM = "Opened";
const char doorStatesText4[] PROGMEM = "Closing";
const char doorStatesText5[] PROGMEM = "Stopped";

const char *const doorStatesTable[] PROGMEM = {doorStatesText0, doorStatesText1, doorStatesText2, doorStatesText3, doorStatesText4, doorStatesText5};

// Alarm states (self explained)
enum alarmStates {  
  alarmNone,
  alarmChickenDetected,
  alarmDoorBlockedOpening,
  alarmDoorBlockedClosing,
  alarmOpeningTooLong,
  alarmClosingTooLong,
  alarmStoppedbyUser
};

const char alarmStatesText0[] PROGMEM = "None";
const char alarmStatesText1[] PROGMEM = "Chicken detected";
const char alarmStatesText2[] PROGMEM = "Door blocked opening";
const char alarmStatesText3[] PROGMEM = "Door blocked closing";
const char alarmStatesText4[] PROGMEM = "Opening too long";
const char alarmStatesText5[] PROGMEM = "Closing too long";
const char alarmStatesText6[] PROGMEM = "Stopped by user";

const char *const alarmStatesTable[] PROGMEM = {alarmStatesText0, alarmStatesText1, alarmStatesText2, alarmStatesText3, alarmStatesText4, alarmStatesText5, alarmStatesText6};

Adafruit_INA219 motorSensor;        // INA219 I2C class
sunStates sunState = sunUnknown;    // Sun current state
doorStates doorState = doorUnknown; // Door current state
alarmStates alarmState = alarmNone; // Alarm current state
bool manualMode = false;            // Are we in manual moden, only accepting external commands?
bool chickenDetected = false;       // Do we currently detect chicken (close to) door?
bool doorUncertainPosition = true;  // Is door position uncertain? (reset after a full open or close)
float motorVoltage = 0;             // Last mesured motor voltage
uint16_t motorIntensity = 0;        // Last mesured motor intesity
float doorOpenPercentage = 0;       // Current door open percentage (0-100%). Valid if doorUncertainPosition is false.
float doorStartPercentage = 0;      // Door open percentage at start of mouvment. Valid if doorUncertainPosition is false.
unsigned long motorStartTime = 0;   // Time of motor start
unsigned long statusSentTime = 0;   // Time of last status message sent
#if RELAY_ON == LOW
  #define RELAY_OFF HIGH            // Define RELAY_OFF opposite to RELAY_ON
#else
  #define RELAY_OFF LOW
#endif

//  *** Illumination ***
#define ILLUMINATION_SIZE 60                // Average illumination is made on 60 readings
int illumination = 0;                       // Average illumination of last minute
int lastLuminosities[ILLUMINATION_SIZE+1];  // Detailled illumination of last minute
uint8_t illuminationPtr = 0;                // Pointer of next illumination to store
unsigned long illuminationReadTime = 0;     // Last time we read limunosity

//    ### Functions ###

//  *** Message output ***

//  Signal something to the user
//    Write message on SERIAL_PORT, if defined. Can be indefined (no message), Serial (USB port) or Serial1 (D4 pin)...
//    Input:
//      _format: Format of string to be displayed (printf format)
//      ...: relevant parameters
static void signal(const char* _format, ...) {
  // Make the message
  char msg[512];                                        // Message buffer (message truncated if longer than this)
  va_list arguments;                                    // Variable argument list
  va_start(arguments, _format);                         // Read arguments after _format
  vsnprintf_P(msg, sizeof(msg), _format, arguments);    // Return buffer containing requested format with given arguments
  va_end(arguments);                                    // End of argument list
  #ifdef SERIAL_PORT
    SERIAL_PORT.println(msg);                           // Print on SERIAL_PORT
  #endif
  if (mqttServer[0]) {                                  // MQTT server defined?
    uint16_t result = mqttClient.publish(mqttSignalTopic, 0, true, msg);  // ... publish message to signal topic
    #ifdef SERIAL_PORT
      SERIAL_PORT.printf(PSTR("Publish %s to %s returned %d\n"), msg, mqttSignalTopic, result);
    #endif
  }
}

//  *** Wifi stuff ***

// WiFi setup
void wiFiSetup() {
  onStationModeConnectedHandler = WiFi.onStationModeConnected(&onWiFiConnected);  // Declare connection callback
  onStationModeGotIPHandler = WiFi.onStationModeGotIP(&onWiFiGotIp);              // Declare got IP callback

  signal(PSTR("Connecting to WiFi..."));
	WiFi.hostname(nodeName);									                                      // Defines this module name
  WiFi.mode(WIFI_STA);                                                            // We want station mode (connect to an existing SSID)
  #ifdef SERIAL_PORT
    SERIAL_PORT.printf(PSTR("SSID: %s, key: %s\n"), wifiSSID, wifiKey);
  #endif
  WiFi.begin(wifiSSID, wifiKey);                                                  // SSID to connect to
}

// Executed when WiFi is connected
static void onWiFiConnected(WiFiEventStationModeConnected data) {
  signal(PSTR("Connected to WiFi"));
}

// Executed when IP address has been given
static void onWiFiGotIp(WiFiEventStationModeGotIP data) {
  signal(PSTR("Got IP address"));
  mqttConnect();  // Connect to MQTT
}

//  *** Asynchronous MQTT client ***

// MQTT setup
void mqttSetup() {
  if (!mqttServer[0]) {                                     // If MQTT server is not defined
    return;
  }
  mqttClient.setServer(mqttServer, mqttPort);             // Set server IP (or name), and port
  mqttClient.setClientId(nodeName);                       // Set client id (= nodeName)
  mqttClient.setCredentials(mqttUsername, mqttPassword);  // Set MQTT user and password
  mqttClient.onMessage(&onMqttMessage);                   // On message (when subscribed item is received) callback
  mqttClient.onConnect(&onMqttConnect);                   // On connect (when MQTt is connected) callback
  mqttClient.setWill(mqttLastWillTopic, 1, true,          // Last will topic
  	"{\"state\":\"down\"}");

}

// Executed when a subscribed message is received
//  Input:
//    topic: topic of received message
//    payload: content of message (WARNING: without ending numm character, use len)
//    properties: MQTT properties associated with thos message
//    len: payload length
//    index: index of this message (for long messages)
//    total: total message count (for long messages)
static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
	char message[len+1];                                  // Allocate size for message plus null ending character
	strncpy(message, payload, len);                       // Copy message on given len
	message[len] = 0;  // Add zero at end
  signal(PSTR("Received: %s"), message);
  if (!strcmp(topic, mqttCommandTopic)) {
    commandReceived(message);                           // We received a message from command topic
  } else if (!strcmp(topic, mqttSettingsTopic)) {
    settingsReceived(message);                          // We received a message from settings topic
  } else if (!strcmp(topic, mqttStatusTopic)) {
    statusReceived(message);                            // We received a message from status topic
  } else {
    signal(PSTR("Can't understand %s as topic"), topic);
  }
}

//Executed when MQTT is connected
static void onMqttConnect(bool sessionPresent) {
  signal(PSTR("MQTT connected"));
  uint16_t result = mqttClient.publish(mqttLastWillTopic, 0, true,  // Last will topic
    "{\"state\":\"up\",\"id\":\"" QUOTE(PROG_NAME) "\",\"version\":\"" CHICKEN_DOOR_VERSION "\"}");
  #ifdef SERIAL_PORT
    SERIAL_PORT.printf(PSTR("Publish to %s returned %d\n"), mqttLastWillTopic, result);
  #endif
  mqttClient.subscribe(mqttCommandTopic, 0);  // Subscribe to command topic
  mqttClient.subscribe(mqttStatusTopic, 0);   // Subscribe to status topic
  mqttClient.subscribe(mqttSettingsTopic, 0); // Subscribe to settings topic
}

// Establish connection with MQTT server
static void mqttConnect() {
  if (mqttServer[0] && !mqttClient.connected()) {  // If not yet connected and server defined
    signal(PSTR("Connecting to MQTT..."));
    mqttClient.connect();         // Start connection
  }
}

//  *** NTP & sun set/rise ***

//  NTP setup
void ntpSetup(){
		NTP.begin("pool.ntp.org",timeZone, dayLight, timeMinutes);  // Start NTP
    NTP.setDSTZone(dstZone);                                    // Set DST zone
}

// NTP loop
void ntpLoop() {
  if ((millis() - lastNtpTest) > 60000) {                                       // Check NTP every minute
    lastNtpTest = millis();                                                     // Save time of last test
    time_t nowTime = now();                                                     // Get current time
    if (NTP.SyncStatus()) {                                                     // If NTP is synchronized
      if (day() != lastComputeDay) {                                            // Sun set/rise not computed for today
        lastComputeDay = day();  // Extract day number
        time_t sunSet = 0;                                                      // Today's sunset
        time_t sunRise = 0;                                                     // Today's sunrise
        uint16_t sunRiseInt = 0;                                                // Sun rise  in integer minutes (hours * 100) + minutes
        uint16_t sunSetInt = 0;                                                 // Sun set in integer minutes (hours * 100) + minutes
        uint16_t timeZoneOffset = (timeZone * 60) + timeMinutes                 // Compute time zone offset to apply
          + (NTP.isSummerTimePeriod(nowTime) ? 60 : 0);                         // Add one hour if summer time is active
        sunParams.calculate(nowTime, timeZoneOffset, sunRise, sunSet);          // Calculate sun set and sun rise
        struct tm * timeInfo;                                                   // Temporary time structure
        time_t clearTime = 0;                                                   // A null time
        timeInfo = gmtime(&clearTime);                                          // Clear timeInfo
        timeInfo->tm_min = 1;                                                   // Set one minute
        time_t oneMinute = mktime(timeInfo);                                    // Convert to time_t
        sunOpen = sunRise + (sunOffsetMinutes * oneMinute);                     // Add sun offset to sunrise
        sunClose = sunSet - (sunOffsetMinutes * oneMinute);                     // Add sun offset to sunset
        timeInfo = localtime (&nowTime);                                        // Convert time to time structure
        // Convert all times to integer value directly readable in decimal (but useless for computation)
        nowInt = (timeInfo->tm_hour * 100) + timeInfo->tm_min;                  // Compute as hour * 100 + minutes
        timeInfo = localtime (&sunRise);
        sunRiseInt = (timeInfo->tm_hour * 100) + timeInfo->tm_min;
        timeInfo = localtime (&sunSet);
        sunSetInt = (timeInfo->tm_hour * 100) + timeInfo->tm_min;
        timeInfo = localtime (&sunOpen);
        sunOpenInt = (timeInfo->tm_hour * 100) + timeInfo->tm_min;
        timeInfo = localtime (&sunClose);
        sunCloseInt = (timeInfo->tm_hour * 100) + timeInfo->tm_min;
        signal(PSTR("Now %d, SunRise %d, SunSet %d, open %d, close %d"), nowInt, sunRiseInt, sunSetInt, sunOpenInt, sunCloseInt);
        sendStatus();
      }
      sunStates newState = beforeOpen;  // By default, before sun rise
      if (nowTime >= sunOpen) {                                               // Are we at or over sun rise ?
        if (nowTime < sunClose) {                                             // Are we before sun set
          newState = betwenOpenAndClose;
        } else {
          newState = afterClose;
        }
      }
      switch (newState) {                                                     // Depending on new state...
        case beforeOpen: 
          if (newState != sunState) {                                         // Signal if sun state changes
            signal(PSTR("Now before sun opening"));
          }
          if (doorState != doorClosed && !manualMode) {                       // If door not closed and not in manual mode
            closeDoor();
          }
          break;
        case betwenOpenAndClose: 
          if (newState != sunState) {                                         // Signal if sun state changes
            signal(PSTR("Now after sun opening"));
          }
          if (doorState != doorOpened  && !manualMode) {                      // If door not opened  and not in manual mode
            openDoor();
          }
          break;
        case afterClose: 
          if (newState != sunState) {                                         // Signal if sun state changes
            signal(PSTR("Now after sun closing"));
          }
          if (doorState != doorClosed && !manualMode) {                       // If door not closed and not in manual mode
            closeDoor();
          }
          break;
        default: 
          signal(PSTR("Can't understand sunState %d"), sunState);
      }
      sunState = newState;                                                    // Load new state
    } else {
      signal(PSTR("NTP not yet synchronized!"));
    }
  }
}

//  *** Door ***

// Door setup
void doorSetup(){
  digitalWrite(openPin, RELAY_OFF);     // Force open relay to off
  pinMode(openPin, OUTPUT);           // Set open pin to output mode

  digitalWrite(closePin, RELAY_OFF);    // Force close relay to off
  pinMode(closePin, OUTPUT);          // Set close pin to output mode

  pinMode(chickenDetectionPin, INPUT); // Set detection pin to input mode

  motorSensor.begin();                // Start motor sensor
}

// Close chicken door
static void closeDoor() {
  signal(PSTR("Closing door"));
  doorState = doorClosing;                  // Set door state
  doorStartPercentage = doorOpenPercentage; // Save start percentage
  motorStartTime = millis();                //  Save start time
  digitalWrite(openPin, RELAY_OFF);         // Deactivate open relay
  digitalWrite(closePin, RELAY_ON);         // Activate close relay
  sendStatus();                             // Update status
}

// Open chicken door
static void openDoor() {
  signal(PSTR("Opening door"));
  doorState = doorOpening;                  // Set door state
  doorStartPercentage = doorOpenPercentage; // Save start percentage
  motorStartTime = millis();                // Save start time
  digitalWrite(closePin, RELAY_OFF);        // Deactivate close relay
  digitalWrite(openPin, RELAY_ON);          // Activate open relay
  sendStatus();                             // Update status
}

// Stop chicken door
static void stopDoor(alarmStates _stopReason) {
  signal(PSTR("Stopping door, reason %d!"), _stopReason);
  doorState = doorStopped;    // Set door state
  alarmState = _stopReason;   // Save stop reason
  motorStartTime = 0;         // Clear start time
  digitalWrite(closePin, RELAY_OFF);  // Deactivate close relay
  digitalWrite(openPin, RELAY_OFF);   // Deactivate open relay
  sendStatus();               // Update status
}

// Door management loop
void doorLoop() {
  // Is a chicken standing through door?
  bool previousChickenDetected = chickenDetected;
  chickenDetected = (digitalRead(chickenDetectionPin) == CHICKEN_DETECTED);
  if (previousChickenDetected != chickenDetected) {
    if (chickenDetected) {
      signal("Chicken detected!");
    } else {
      signal("Chicken gone");
    }
  }
  readCurrent();                                    // Read motor current and voltage

  // Are we closing?
  if (doorState == doorClosing) {
    if (!doorUncertainPosition) {                   // Except if door uncertain position
      // Update door position
      doorOpenPercentage = doorStartPercentage + (100 * (millis() - motorStartTime) / closeDuration);
      doorOpenPercentage = doorOpenPercentage > 100 ? 100 : doorOpenPercentage < 0 ? 0 : doorOpenPercentage;
    }
    // Is a chicken detected?
    if (chickenDetected) {                           // Yes
      signal(PSTR("Chicken detected!"));
      alarmState = alarmChickenDetected;            // Set alarm
      openDoor();                                   // Open door
      sendStatus();                                 // Update status
    } else {
      // Do we started more than 1,5 times close duration?
      if ((millis() - motorStartTime) > (closeDuration + (closeDuration / 2))) {
        stopDoor(alarmClosingTooLong);              // Stop door with reason
      } else if ((millis() - motorStartTime) > 500) { // Check motor's intensity only 500 ms after start, masking start overcurrent
        if (                                          // Check motor current giving endOfCourse current
                  (endOfCourseCurrent < 0 && motorIntensity < -endOfCourseCurrent) ||
                  (endOfCourseCurrent >= 0 && motorIntensity >= endOfCourseCurrent)
                ) {
        signal(PSTR("Door closed"));
        digitalWrite(closePin, RELAY_OFF);          // Deactivate close relay
        digitalWrite(openPin, RELAY_OFF);           // Deactivate open relay
        doorState = doorClosed;                     // Update state
        doorOpenPercentage = 0;                     // Force percentage
        doorUncertainPosition = false;              // Clear uncertain position
        alarmState = alarmNone;                     // Clear alarm
        motorStartTime = 0;                         // Clear start time
        sendStatus();                               // Update status
      } else {
        if (motorIntensity >= obstacleCurrent       // Motor currunt more than obstacle detected one
            && (doorOpenPercentage <= 95            //  and (not close to fully open
              || endOfCourseCurrent < 0)) {         //       or no end of course blocking current)
          signal(PSTR("Door blocked!"));
          alarmState = alarmDoorBlockedClosing;     // Set alarm
          openDoor();                               // Open door
          sendStatus();                             // Update status
        }
      }
    }
  }
  }
  // Are we opening?
  if (doorState == doorOpening) {
    if (!doorUncertainPosition) {                   // Except if door uncertain position
      // Update dor position
      doorOpenPercentage = doorStartPercentage + (100 * (millis() - motorStartTime) / openDuration);
      doorOpenPercentage = doorOpenPercentage > 100 ? 100 : doorOpenPercentage < 0 ? 0 : doorOpenPercentage;
    }
    // Do we started more than 1,5 times open duration?
    if ((millis() - motorStartTime) > (openDuration + (openDuration / 2))) {
      stopDoor(alarmOpeningTooLong);
    } else if (                                   // Check motor current giving endOfCourse current
                (endOfCourseCurrent < 0 && motorIntensity < -endOfCourseCurrent) ||
                (endOfCourseCurrent >= 0 && motorIntensity >= endOfCourseCurrent && doorOpenPercentage >= 95)
              ) {
      signal(PSTR("Door opened"));
      digitalWrite(closePin, RELAY_OFF);            // Deactivate close relay
      digitalWrite(openPin, RELAY_OFF);             // Deactivate open relay
      doorState = doorOpened;                       // Set status
      doorOpenPercentage = 100;                     // Force percentage
      doorUncertainPosition = false;                // Clear uncertain position
      motorStartTime = 0;                           // Clear start time
      sendStatus();                                 // Update status
    } else {
      if (motorIntensity >= obstacleCurrent         // Motor currunt more than obstacle detected one
          && (doorOpenPercentage <= 95              //  and (not close to fully open
            || endOfCourseCurrent < 0)) {           //       or no end of course blocking current)
        signal(PSTR("Door blocked!"));
        stopDoor(alarmDoorBlockedOpening);          // Stop door
        sendStatus();                               // Update status
      }
    }
  }
  // Door in mouvment
  if (doorState == doorClosing || doorState == doorOpening) {
    if ((millis() - statusSentTime) > 1000) {       // Every second
      sendStatus();                                 // Update status (statusSentTime will be set to current time into this routine)
    }
  }
}

// Current consumption
void readCurrent() {
  motorVoltage = motorSensor.getBusVoltage_V() + (motorSensor.getShuntVoltage_mV() / 1000); // Load battery voltage
  motorIntensity = motorSensor.getCurrent_mA();                                             // Load motor current
}

//  *** Illumination ***

// Illumination setup
void illuminationSetup() {
  for (uint8_t i = 0; i < ILLUMINATION_SIZE; i++) {                                 // For all illumination
    lastLuminosities[i] = 0;                                                        // Clear illumination (useless but clean ;-)
  }
}

// Illumination loop
void illuminationLoop() {
  if (openIllumination) {                                                           // If open illumination set
    if ((millis() - illuminationReadTime) > 1000) {                                 // Read illumination every second
      illuminationReadTime = millis();                                              // Save last read time
      lastLuminosities[illuminationPtr++] = analogRead(ldrPin);                     // Load illumination
      if (illuminationPtr >= ILLUMINATION_SIZE) {                                   // At end of table?
        illuminationPtr = 0;                                                        // Clear pointer
        float sum = 0;                                                          // Init sum
        for (uint8_t i = 0; i < ILLUMINATION_SIZE; i++) {                           // For all illumination
          sum += lastLuminosities[i]; // Add sum
        }
        illumination = sum / ILLUMINATION_SIZE;                                     // Compute average
        if (!manualMode) {                                                      // If door not in manual mode
          if (doorState == doorClosed && illumination <= closeIllumination) {       // Are we less than close illumination?
            signal(PSTR("Illumination %d, closing..."));
            closeDoor();                                                        // Close door
          } else if (doorState == doorOpened && illumination >= openIllumination) { // Are we over open illumination?
            signal(PSTR("Illumination %d, opening..."));
            openDoor();                                                         // Open door
          }
        }
      }
    }
  }
}

//  *** Settings ***

// Executed when a settings topic has been received
static void settingsReceived(char* msg) {
  JsonDocument settings;
	auto error = deserializeJson(settings, msg);  // Read settings
	if (error) {													        // Error reading settings
		signal(PSTR("Failed to parse settings >%s<"), msg);
		return;
  }
  // Save values before change
  int16_t sunOffsetMinutesBefore = sunOffsetMinutes;

  // Load all elements giving they types (with current value as default instead of null)
  openIllumination   = settings["openIllumination"].as<uint16_t>() | openIllumination;
  closeIllumination  = settings["closeIllumination"].as<uint16_t>() | closeIllumination;
  openDuration       = settings["openDuration"].as<uint16_t>() | openDuration;
  closeDuration      = settings["closeDuration"].as<uint16_t>() | closeDuration;
  endOfCourseCurrent = settings["endOfCourseCurrent"].as<int16_t>() | endOfCourseCurrent;
  obstacleCurrent    = settings["obstacleCurrent"].as<uint16_t>() | obstacleCurrent;
  sunOffsetMinutes   = settings["sunOffsetMinutes"].as<int16_t>() | sunOffsetMinutes;

  // Do we changed sun offset?
  if (sunOffsetMinutesBefore != sunOffsetMinutes) {
    lastComputeDay = 0;                         // Force ntpLoop to recompute right values
  }
}

// Sends current settings
static void sendSettings() {
  JsonDocument settings;                                  // Creates a JSON document
  // Loads all data
  settings["openIllumination"]   = openIllumination;
  settings["closeIllumination"]  = closeIllumination;
  settings["openDuration"]       = openDuration;
  settings["closeDuration"]      = closeDuration;
  settings["endOfCourseCurrent"] = endOfCourseCurrent;
  settings["obstacleCurrent"]    = obstacleCurrent;
  settings["sunOffsetMinutes"]   = sunOffsetMinutes;

  char buffer[512];                                       // Output buffer
  serializeJsonPretty(settings, buffer, sizeof(buffer));  // Convert document to string
  uint16_t result = mqttClient.publish(mqttSettingsTopic, 0, true, buffer); // Sends to settings topics
  #ifdef SERIAL_PORT
    SERIAL_PORT.printf(PSTR("Publish %s to %s returned %d\n"), buffer, mqttSettingsTopic, result);
  #endif
}

// Executed when a status topic has been received
static void statusReceived(char* msg) {
  if (!mqttStatusReceived) {                    // Load previous state only if not yet known
    mqttStatusReceived = true;                  // Set status received
    mqttClient.unsubscribe(mqttStatusTopic);    // Stop receiving updates (as we'll do it later)
    JsonDocument status;                        // Creates JSON document
    auto error = deserializeJson(status, msg);  // Read status
    if (error) {													      // Error reading status
      signal(PSTR("Failed to parse status >%s<"), msg);
      return;
    }
    // Load data
    doorState = status["doorState"].as<doorStates>();
    alarmState = status["alarmState"].as<alarmStates>();
    doorOpenPercentage = status["doorOpenPercentage"]. as<float>();
    doorUncertainPosition = (doorState == doorOpening || doorState == doorClosing || doorStartPercentage == doorUnknown);
    // Send status back (update of doorUncertainPosition)
    sendStatus();
  }
}

// Sends a status
static void sendStatus() {                              // Exit if no MQTT server
  if (!mqttServer[0]) {
    return;
  }
  JsonDocument status;                                  // Creates JSON document
  // Load all data
  char text[50];
  status["doorState"] = doorState;
  if (doorState < 0 || doorState >= sizeof(doorStatesTable)) {
    snprintf_P(text, sizeof(text), PSTR("??? %d ???"), doorState);
  } else {
    strncpy_P(text, (char *)pgm_read_ptr(&(doorStatesTable[doorState])), sizeof(text));
  }
  status["doorStateText"] = text;
  status["alarmState"] = alarmState;
  if (alarmState < 0 || alarmState >= sizeof(alarmStatesTable)) {
    snprintf_P(text, sizeof(text), PSTR("??? %d ???"), alarmState);
  } else {
    strncpy_P(text, (char *)pgm_read_ptr(&(alarmStatesTable[alarmState])), sizeof(text));
  }
  status["alarmStateText"] = text;
  status["sunState"] = sunState;
  if (sunState < 0 || sunState >= sizeof(sunStatesTable)) {
    snprintf_P(text, sizeof(text), PSTR("??? %d ???"), sunState);
  } else {
    strncpy_P(text, (char *)pgm_read_ptr(&(sunStatesTable[sunState])), sizeof(text));
  }
  status["sunStateText"] = text;
  status["manualMode"] = manualMode ? true : false;
  status["illumination"] = illumination;
  snprintf_P(text, sizeof(text), PSTR("%.2f"), motorVoltage);
  status["motorVoltage"] = text;
  status["motorIntensity"] = motorIntensity;
  status["motorDuration"] = motorStartTime ? (millis()-motorStartTime)/1000 : 0;
  status["lastStatusSent"] = statusSentTime ? (millis()-statusSentTime)/1000 : 0;
  status["chickenDetected"] = chickenDetected ? true : false;
  status["doorUncertainPosition"] = doorUncertainPosition ? true : false;
  status["doorOpenPercentage"] = doorOpenPercentage;
  snprintf_P(text, sizeof(text), PSTR("%02d:%02d"), nowInt/100, nowInt % 100);
  status["now"] = text;
  snprintf_P(text, sizeof(text), PSTR("%02d:%02d"), sunOpenInt/100, sunOpenInt % 100);
  status["sunOpen"] = text;
  snprintf_P(text, sizeof(text), PSTR("%02d:%02d"), sunCloseInt/100, sunCloseInt % 100);
  status["sunClose"] = text;

  char buffer[512];                                     // Output buffer
  serializeJsonPretty(status, buffer, sizeof(buffer));  // Load buffer with JSON data
  uint16_t result = mqttClient.publish(mqttStatusTopic, 0, true, buffer); // Send to status topic
  #ifdef SERIAL_PORT
    SERIAL_PORT.printf(PSTR("Publish %s to %s returned %d\n"), buffer, mqttStatusTopic, result);
  #endif
  statusSentTime = millis();                            // Save last time we sent an update (used in doorLoop)
}

// Executed when a command topic has been received
static void commandReceived(char* msg) {
  if (!strcmp(msg,"close")) {           // Is this a "close" command?
    manualMode = true;                  // We're in manual mode
    closeDoor();                        // Close door
   } else if (!strcmp(msg,"open")) {    // "Open" command?
    manualMode = true;                  // Manual mode
    openDoor();                         // Open door
  } else if (!strcmp(msg,"stop")) {     // "stop" command?
    manualMode = true;                  // Manual mode
    stopDoor(alarmStoppedbyUser);       // Stop door with right reason
  } else if (!strcmp(msg,"status")) {   // "status" command?
    sendStatus();                       // Send status
  } else if (!strcmp(msg,"auto")) {     // "auto" command?
    manualMode = false;                 // Clear manual mode
    sendStatus();                       // Update status
  } else if (!strcmp(msg,"settings")) { // "settings" command?
    sendSettings();                     // Send settings
  } else {
    // Don't signal acked or unknown messages
    if (!(strstr(msg, " done") || strstr(msg, " unknown"))) {
      // Here we have a invalid command not executed
      char buffer[50];                  // Allocated buffer for answer
      snprintf_P(buffer, sizeof(buffer), PSTR("%s unknown"), msg);
      uint16_t result = mqttClient.publish(mqttCommandTopic, 0, true, buffer);
      #ifdef SERIAL_PORT
        SERIAL_PORT.printf(PSTR("Publish %s to %s returned %d\n"), buffer, mqttCommandTopic, result);
      #endif
      signal(PSTR("Can't understand >%s< command!"), msg);
    }
    return;
  }
  // Here we have a valid command executed
  char buffer[50];                      // Allocated buffer for answer
  sniprintf(buffer, sizeof(buffer), "%s done", msg);
  uint16_t result = mqttClient.publish(mqttCommandTopic, 0, true, buffer);
  #ifdef SERIAL_PORT
    SERIAL_PORT.printf(PSTR("Publish %s to %s returned %d\n"), buffer, mqttCommandTopic, result);
  #endif
}

//    ### Executed at startup ###
void setup(){
  #ifdef SERIAL_PORT          // If SERIAL_PORT defined
    SERIAL_PORT.begin(74880); // Init Serial to right speed
    SERIAL_PORT.println("");
  #endif
  signal(PSTR(QUOTE(PROG_NAME) " V" CHICKEN_DOOR_VERSION " starting..."));
  mqttSetup();                // MQTT setup
  wiFiSetup();                // WiFi setup
  ntpSetup();                 // NTP setup
  illuminationSetup();        // Illumination setup
  doorSetup();                // Door stup
  signal(PSTR("Started"));
}

//    ### Main loop ###
void loop(){
  ntpLoop();        // NTP loop
  illuminationLoop(); // Illumination loop
  doorLoop();       // Door loop
}