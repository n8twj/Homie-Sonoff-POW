#include <EEPROM.h>
#include <Homie.h>
#include "power.h"

#define BUTTON 0
#define RELAY 12
#define LED 13
#define INTERVAL 60

ESP8266PowerClass power_read;

int relayState = LOW; 
int lastRelayState;
bool stateChange = false; 

int buttonState;                     // the current reading from the input pin
int lastButtonState = LOW;           // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long lastSent = 0;
bool sentOnce = false;

HomieNode switchNode("switch", "switch");
HomieNode batteryLoadVoltageNode("voltage", "voltage");
HomieNode batteryCurrentNode("power", "watts");

bool switchHandler(HomieRange range, String value) {
  if (value == "true") {
    digitalWrite(RELAY, HIGH);
    digitalWrite(LED, LOW); // LED state is inverted on Sonoff TH
    switchNode.setProperty("on").send("true");
    Serial.println("Switch is on");
  } else if (value == "false") {
    digitalWrite(RELAY, LOW);
    digitalWrite(LED, HIGH); // LED state is inverted on Sonoff TH
    switchNode.setProperty("on").send("false");
    Serial.println("Switch is off");
  } else {
    return false;
  }
  return true;
}

void loopHandler() {
   if (millis() - lastSent >= INTERVAL * 1000UL || lastSent == 0) {
      double watts = power_read.getPower();
      double voltage = power_read.getVoltage();
      batteryLoadVoltageNode.setProperty("volts").send(String(voltage));
      batteryCurrentNode.setProperty("watts").send(String(watts));
      lastSent = millis();
   }
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON, INPUT);
  digitalWrite(LED, LOW);
  digitalWrite(RELAY, LOW);

  Serial.begin(115200);
  Serial.println();
  Serial.println();

  Homie_setFirmware("itead-sonoff-pow", "1.0.0");
  Homie.setLoopFunction(loopHandler);
  Homie.setLedPin(LED, LOW);
  Homie.setResetTrigger(BUTTON, LOW, 10000);
  switchNode.advertise("on").settable(switchHandler);
 
  Homie.setup();

  power_read.enableMeasurePower();
  power_read.selectMeasureCurrentOrVoltage(VOLTAGE);
  power_read.startMeasure();
}

void loop() {
  byte reading = digitalRead(BUTTON);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
       buttonState = reading;
       if (buttonState == LOW) {
        stateChange = true;
        relayState = !relayState;
      }
    }
  }
  lastButtonState = reading;
  if (stateChange) { 
    digitalWrite(RELAY, relayState);
    digitalWrite(LED, !relayState); // LED state is inverted on Sonoff
    switchNode.setProperty("on").send( (relayState == HIGH)? "true" : "false" );
    stateChange = false;
  }
  Homie.loop();
}
