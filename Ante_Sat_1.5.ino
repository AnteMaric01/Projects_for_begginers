#include <Buzzer.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT11.h>

enum ButtonState {
  BUTTON_IDLE,
  BUTTON_PRESSED,
  BUTTON_LONG_PRESSED,
  BUTTON_RELEASED
};


RTC_DS1307 rtc;  // Use this line for DS1307
DHT11 dht11(2);   // Initialize DHT11 with pin 2

const int latchPin = 9;  // 74HC595 pin 9 STCP
const int clockPin = 10; // 74HC595 pin 10 SHCP
const int dataPin  = 8;  // 74HC595 pin 8 DS
const int digit0   = 46; // 7-Segment pin D4
const int digit1   = 48; // 7-Segment pin D3
const int digit2   = 50; // 7-Segment pin D2
const int digit3   = 52; // 7-Segment pin D1
const int infoPin = 4; // Button pin
const int decrementPin = 7; // Decrement button pin
const int incrementPin = 6; // Increment button pin
const int setPin = 5; // Set button pin
const int buzzerPin = 3;     // Buzzer pin
int brightness = 90;
unsigned long lastSensorReadTime = 0;
const int sensorReadInterval = 10000;
bool longPressDetected = false;
int displayMode = 0; // 0: Time, 1: Temperature, 2: Humidity
bool isSensorReadTriggered = false;
bool setMode = false;
bool hourSetMode = true; // true: set hours, false: set minutes
int alarmHour = 12; // Initial value for alarm hour
int alarmMinute = 0; // Initial value for alarm minute
bool alarmSetMode = false; // Flag for Alarm time setting mode

byte table[] =
    { 0b01011111,  // = 0
      0b01000100,  // = 1
      0b10011101,  // = 2
      0b11010101,  // = 3
      0b11000110,  // = 4
      0b11010011,  // = 5
      0b11011011,  // = 6
      0b01000101,  // = 7
      0b11011111,  // = 8
      0b11000111,  // = 9
      0b11001111,  // = A
      0b11011010,  // = b
      0b00011011,  // = C
      0b11011100,  // = d
      0b10011011,  // = E
      0b10001011,  // = F
      0b00000000,  // blank
      0b11001110   // = H
};

byte controlDigits[] = { digit0, digit1, digit2, digit3 };
byte displayDigits[] = { 0, 0, 0, 0 };





class TemperatureSensor {
public:
  int temperature;
  int humidity;

  void read() {
    temperature = dht11.readTemperature();
    humidity = dht11.readHumidity();
  }

  
};

TemperatureSensor sensor;
Buzzer buzzer(3);  // Specify the pin number (adjust as needed)




void setup() {
  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  for (int x = 0; x < 4; x++) {
    pinMode(controlDigits[x], OUTPUT);
    digitalWrite(controlDigits[x], LOW);
  }

  pinMode(infoPin, INPUT_PULLUP);
  pinMode(decrementPin, INPUT_PULLUP);
  pinMode(incrementPin, INPUT_PULLUP);
  pinMode(setPin, INPUT_PULLUP);
}

void DisplaySegments() {
  for (int x = 0; x < 4; x++) {
    // Enable the current digit
    for (int k = 0; k < 4; k++) {
      digitalWrite(controlDigits[k], LOW);
    }
    digitalWrite(latchPin, LOW);

    // Set the data for the current digit
    shiftOut(dataPin, clockPin, MSBFIRST, table[displayDigits[x]]);

    // Latch the data to the display
    digitalWrite(latchPin, HIGH);
    digitalWrite(controlDigits[x], HIGH);
    delayMicroseconds(2);
  }

  // Disable all digits
  for (int k = 0; k < 4; k++) {
    digitalWrite(controlDigits[k], LOW);
  }
}

void activateBuzzer() {
  tone(buzzerPin, 2000, 500); // Activate buzzer at 1000 Hz for 1 second
  delay(500); // Wait for 1 second to complete the tone
  noTone(buzzerPin); // Turn off the buzzer
}

void playMelody() {
  int melody[] = { NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4 };
  int noteDurations[] = { 400, 800, 800, 400, 400, 400, 400, 400 };

  unsigned long startTime = millis();

  while (millis() - startTime < 60000) {
    for (int i = 0; i < 8; i++) {
      tone(buzzerPin, melody[i], noteDurations[i]);
      delay(noteDurations[i] + 50);  // Add a small delay between notes
      noTone(buzzerPin);
    }
  }
}


void setAlarmMode() {
  static unsigned long infoButtonPressTime = 0;
  static bool longPressDetected = false;
  int adjustmentSpeed = digitalRead(incrementPin) == LOW ? 1 : 2;  // Increase speed if increment button is held down

  if (digitalRead(infoPin) == HIGH) {
    // Info button is not pressed
    if (infoButtonPressTime > 0) {
      // Info button was released after a press
      if (millis() - infoButtonPressTime > 2000 && !longPressDetected) {
        // Long press: Enter/exit alarm setting mode
        alarmSetMode = !alarmSetMode;
        longPressDetected = true;
        // Buzzer activation for alarm setting mode
        activateBuzzer();
      } else if (!longPressDetected) {
        // Short press: Toggle between display modes
        displayMode = (displayMode + 1) % 3;
        activateBuzzer(); // Activate buzzer when switching display mode
      }
      infoButtonPressTime = 0; // Reset press time
      longPressDetected = false; // Reset long press flag
    }
  } else {
    // Info button is pressed
    if (infoButtonPressTime == 0) {
      // Info button is just pressed, record the press time
      infoButtonPressTime = millis();
    } else {
      // Info button is being held
      if (millis() - infoButtonPressTime > 2000) {
        // Long press detected
        longPressDetected = true;
      }
    }
  }

  if (alarmSetMode) {
    // Alarm setting mode logic
    // Add any specific actions for setting the alarm time
    // Use the hourSetMode flag to determine whether setting hours or minutes

    // Check if the Set button is pressed to toggle between hours and minutes adjustment
    if (digitalRead(setPin) == LOW) {
      hourSetMode = !hourSetMode;
      delay(500); // Debounce delay
    }

    if (digitalRead(decrementPin) == LOW) {
      // Decrement button is pressed, adjust hours or minutes
      if (hourSetMode) {
        alarmHour = (alarmHour + 23) % 24; // Subtract 1 hour, ensure result is positive
        delay(500);
      } else {
        alarmMinute = (alarmMinute + 59) % 60; // Subtract 1 minute, ensure result is positive
        delay(200);
      
      }
    }

    if (digitalRead(incrementPin) == LOW) {
      // Increment button is pressed, adjust hours or minutes
      if (hourSetMode) {
        alarmHour = (alarmHour + 1) % 24; // Add 1 hour, ensure result is within 0-23
        delay(500);
      } else {
        alarmMinute = (alarmMinute + 1) % 60; // Add 1 minute, ensure result is within 0-59
        delay(200);
      }
      
    }
    // Update display using existing array and function
    displayDigits[3] = alarmHour / 10;
    displayDigits[2] = alarmHour % 10;
    displayDigits[1] = alarmMinute / 10;
    displayDigits[0] = alarmMinute % 10;
    DisplaySegments();
  }
}



void setTimeMode() {
  static unsigned long setButtonPressTime = 0;
  static bool shortPressDetected = false;
  int adjustmentSpeed = digitalRead(incrementPin) == LOW ? 1 : 2;  // Increase speed if increment button is held down

  if (digitalRead(setPin) == HIGH) {
    // Set button is not pressed
    if (setButtonPressTime > 0) {
      // Set button was released after a press
      if (millis() - setButtonPressTime < 2000) {
        // Short press: Toggle between setting hours and minutes
        if (setMode && !shortPressDetected) {
          hourSetMode = !hourSetMode;
          shortPressDetected = true;
        }
      } else {
        // Long press: Enter/exit time setting mode
        setMode = !setMode;
        shortPressDetected = false;
        // Buzzer activation for time setting mode
        activateBuzzer();
      }
      setButtonPressTime = 0; // Reset press time
    }
  } else {
    // Set button is pressed
    if (setButtonPressTime == 0) {
      // Set button is just pressed, record the press time
      setButtonPressTime = millis();
    } else {
      // Set button is being held
      // Add additional logic here if needed
    }
  }

  if (setMode && digitalRead(setPin) == HIGH) {
    // Additional logic when in time setting mode and set button is not pressed
    // Add any specific actions if needed
    shortPressDetected = false;
  }

  if (setMode) {
    // Time setting mode logic
    DateTime now = rtc.now();
    if (digitalRead(decrementPin) == LOW) {
      // Decrement button is pressed, adjust hours or minutes
      if (hourSetMode) {
        rtc.adjust(DateTime(now.unixtime() - 3600)); // Subtract 1 hour
      } else {
        rtc.adjust(DateTime(now.unixtime() - (60 * adjustmentSpeed))); // Subtract minutes with adjustable speed
      }
      delay(500); // Debounce delay
    }

    if (digitalRead(incrementPin) == LOW) {
      // Increment button is pressed, adjust hours or minutes
      if (hourSetMode) {
        rtc.adjust(DateTime(now.unixtime() + 3600)); // Add 1 hour
      } else {
        rtc.adjust(DateTime(now.unixtime() + (60 * adjustmentSpeed))); // Subtract minutes with adjustable speed
      }
      delay(500); // Debounce delay
    }
  }
}


ButtonState getButtonState(int pin) {
  static bool buttonWasPressed = false;
  static unsigned long buttonPressStartTime = 0;
  int buttonState = digitalRead(pin);

  if (buttonState == HIGH) {
    // Button is not pressed
    if (buttonWasPressed) {
      // Button was released after a press
      buttonWasPressed = false;
      if (millis() - buttonPressStartTime < 2000) {
        // Short press
        return BUTTON_RELEASED;
      } else {
        // Long press
        return BUTTON_LONG_PRESSED;
      }
    }
  } else {
    // Button is pressed
    if (!buttonWasPressed) {
      // Button was just pressed, record the press start time
      buttonWasPressed = true;
      buttonPressStartTime = millis();
      return BUTTON_PRESSED;
    }
  }

  return BUTTON_IDLE;
}


void loop() {
  DateTime now = rtc.now();
  static bool infoButtonPressed = false;
  static unsigned long infoButtonPressTime = 0;

  ButtonState infoButtonState = getButtonState(infoPin);

  switch (infoButtonState) {
    case BUTTON_PRESSED:
      infoButtonPressed = true;
      infoButtonPressTime = millis();
      break;

    case BUTTON_LONG_PRESSED:
      if (infoButtonPressed && millis() - infoButtonPressTime > 2000) {
        alarmSetMode = !alarmSetMode;
        activateBuzzer();
        infoButtonPressed = false;
      }
      break;

    case BUTTON_RELEASED:
      if (infoButtonPressed) {
        if (millis() - infoButtonPressTime < 2000) {
          // Short press: Switch display mode
          displayMode = (displayMode + 1) % 3;
          activateBuzzer();
        }
        infoButtonPressed = false;
      }
      break;

    default:
      // Check if info button is being held
      if (infoButtonPressed && millis() - infoButtonPressTime > 2000) {
        alarmSetMode = !alarmSetMode;
        activateBuzzer();
        infoButtonPressed = false;
      }
      break;
  }

  if (alarmSetMode) {
    // Call setAlarmMode function if in alarm setting mode
    setAlarmMode();
  } else {
    // Display modes
    if (displayMode == 0) {
      // Display time mode
      setTimeMode();
      displayDigits[3] = now.hour() / 10;
      displayDigits[2] = now.hour() % 10;
      displayDigits[1] = now.minute() / 10;
      displayDigits[0] = now.minute() % 10;
      DisplaySegments();
       // Check if current time matches alarm time
      if (now.hour() == alarmHour && now.minute() == alarmMinute) {
        playMelody();  // Call a function to play the melody
        delay(60000);  // Delay for 60 seconds
        noTone(buzzerPin);
      }

    } else if (displayMode == 1) {
      // Display temperature mode
      if (!isSensorReadTriggered) {
        sensor.read();
        isSensorReadTriggered = true;
      }
      displayDigits[2] = sensor.temperature / 10;
      displayDigits[1] = sensor.temperature % 10;
      displayDigits[3] = 16;  // No segments
      displayDigits[0] = 12;  // C
      DisplaySegments();
    } else if (displayMode == 2) {
      // Display humidity mode
      if (!isSensorReadTriggered) {
        sensor.read();
        isSensorReadTriggered = true;
      }
      displayDigits[0] = 17;  // H
      displayDigits[3] = 16;
      displayDigits[2] = sensor.humidity / 10;
      displayDigits[1] = sensor.humidity % 10;
      DisplaySegments();
    }
  }

  delayMicroseconds(1638 * ((100 - brightness) / 10));
}
