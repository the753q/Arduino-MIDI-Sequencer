#include <EEPROM.h>
#include <TM1637Display.h>

//                  (CLK, DIO)
TM1637Display display(17, 19);

#define AnalogInFrequency A3
#define AnalogInTempo A2
#define AnalogInDuration A1
#define AnalogInPitch A0
#define DigitalOutSignal 14
#define PowerButton 15
#define PowerLED 5
#define ExtremeButton 16
#define ExtremeLED 24
#define RedLED 13
#define GreenLED 11
#define BlueLED 6
#define VibrationMotor 47
#define MenuColor 0, 30, 255
#define DualModeSwitch 46
#define RGBCommonCathode false
#define RGBMaxBrightness 200
#define RGBStandbyMaxBrightness 80

//     Segment dpGFEDCBA          7-Segment Alphabet
#define A     0b01110111  //  A
#define B     0b01111100  //  b
#define C     0b00111001  //  C
#define D     0b01011110  //  d
#define E     0b01111001  //  E
#define F     0b01110001  //  F
#define H     0b01110110  //  H
#define I     0b00110000  //  I
#define L     0b00111000  //  L
#define M     0b01010101  //  M
#define N     0b01010100  //  N
#define P     0b01110011  //  P
#define R     0b01010000  //  r
#define S     0b01101101  //  S
#define T     0b01111000  //  t
#define U     0b00011100  //  u
#define V     0b00111110  //  V
#define Y     0b01101110  //  y
#define blank 0b00000000  //  (blank)

const byte numberSegment[] = {
              0b00111111, // 0
              0b00000110, // 1
              0b01011011, // 2
              0b01001111, // 3
              0b01100110, // 4
              0b01101101, // 5
              0b01111101, // 6
              0b00000111, // 7
              0b01111111, // 8
              0b01101111, // 9
};

//  Segments to Display on Each 7-Segment Display:
const byte NoSelect[] = {SEG_G, SEG_G, SEG_G, SEG_G}; //  ----
const byte Select[] = {
  P,         //  P (piano)
  F,         //  F (frequency generator)
  R,         //  r (random generator)
  S          //  S (setup)
};
const byte PianoPlay[4] = {P, L, A, Y};
const byte SettingsText[29] = {M, I, D, I, blank, C, H, A, N, N, E, L, blank, blank, blank, blank, blank, V, I, B, R, A, T, E, blank, blank, blank, blank, blank};
bool settingsFirstRun = true;
bool channelSettingsFirstRun = false;
bool nextSegment = false;
bool channelButtonPrevious[] = {LOW, LOW};
byte onDisplayCharacter[] = {blank, blank, blank, blank};
byte segmentCharacter = 0;
byte MIDIchannelSettings = 0;
byte mainChannel = 0;
byte secondChannel = 1;;
int scrollingTime = 250;

//  MIDI messages
const byte NoteOFF = 0x80;
const byte NoteON  = 0x90;
const byte MIDIcc  = 0xB0;
//  Change number below to change the MIDI control change message device number for each potentiometer:
const byte DeviceNumberCC[] = {           17,               19,            20,                21};
const byte MIDIccPot[]      = {AnalogInPitch, AnalogInDuration, AnalogInTempo, AnalogInFrequency};

bool MIDIplaying = false;
byte velocity = 127;
byte note = 0;
byte previousNote = 1;

const byte StepButton[] = {32, 33, 34, 35, 36, 37, 38, 39};
const byte StepLED[] = {2, 3, 4, 45, 44, 7, 8, 12};
byte brightness[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte fadeTime = 3;
byte fadeValue = 1;
byte RGBcalculatedBrightness = 255;

bool LEDsReset = false;
bool standbyDimming = true;
byte standbyFadeValue = 254;
byte standbyFadeNumber = 1;
int standbyFadeTime = 10;

bool preciseUp = LOW;
bool preciseDown = LOW;
bool MIDIsent[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool keyON[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte keyOFF = 0;

byte previousPushedStep = -1;
byte previousKnownStep = -1;
byte playMode = 0;
byte stepNumber = 0;
byte previousKey[] = {0, 0, 0, 0, 0, 0, 0, 0};
int steps[] = {200, 250, 200, 250, 200, 255, 250, 252};
int keyTone[] = {33, 37, 41, 45, 49, 55, 62, 66};
int pitch = 0;
int tempo = 10;
int duration = 50;
int frequency = 50;

byte dualNote = 0;
byte dualPreviousNote = 0;
int dualPitch = 0;
int dualTempo = 10;
int dualDuration = 50;

bool displayingPotValue = false;
bool rotatingPot[] = {false, false, false, false};
byte mapCC[] = {0, 0, 0, 0};
byte previousCC[] = {0, 0, 0, 0};
byte potSmoothValue = 2;
int potValue[] = {0, 0, 0, 0};
int previousPotValue[] = {0, 0, 0, 0};

bool reading = LOW;
bool previous = LOW;
bool extremeModeSelected = false;
byte extremeMode = 0;
byte debouncePeriod = 255;
unsigned int modeSelection = -1;

unsigned long currentMillis = 0;
unsigned long debounceMillis = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long textScrollingMillis = 0;
unsigned long previousMillisVibration = 0;
unsigned long potRotatingMillis[] = {0, 0, 0, 0};

bool allowVibration = true;
bool previousVibrateButton = LOW;
bool vibration = false;
int vibrateDuration = 300;

void setup() {
  pinMode(DigitalOutSignal, OUTPUT);
  pinMode(PowerLED, OUTPUT);
  pinMode(ExtremeLED, OUTPUT);
  if (RGBCommonCathode == true) {
    RGBcalculatedBrightness = RGBMaxBrightness;
  }
  else {
    RGBcalculatedBrightness = map(RGBMaxBrightness, 0, 255, 255, 0);
  }
  display.setBrightness(0xF);
  display.clear();
  allowVibration = EEPROM.read(0);
  mainChannel = EEPROM.read(1);
  secondChannel = EEPROM.read(2);
  vibrate(400);
  Serial.begin(31250);
}

void loop() {
  currentMillis = millis() + modeSelection;
  if (currentMillis - previousMillis3 > 1010) {
    previousMillis3 = currentMillis;
    displayingPotValue = false;
  }
  if (vibration == true) {
    digitalWrite(VibrationMotor, HIGH);
    if (currentMillis - previousMillisVibration > vibrateDuration) {
      previousMillisVibration = currentMillis;
      vibration = false;
    }
  }
  else {
    digitalWrite(VibrationMotor, LOW);
  }
  reading = digitalRead(PowerButton);
  if (reading == HIGH && previous == LOW) {
    if (currentMillis - debounceMillis >= debouncePeriod) {
      debounceMillis = currentMillis;
      if (playMode == 0) {
        playMode = 1;
        digitalWrite(PowerLED, HIGH);
      }
      else {
        playMode = 0;
        extremeModeSelected = false;
        digitalWrite(PowerLED, LOW);
        digitalWrite(ExtremeLED, LOW);
        noTone(DigitalOutSignal);
        resetLEDs(0, 7);
        MIDImessage(NoteOFF, true, note, velocity);
        MIDImessage(NoteOFF, false, dualNote, velocity);
        display.clear();
        RGBcolor(0, 0, 0);
        standbyDimming = true;
        standbyFadeValue = 0;
        settingsFirstRun = true;
      }
      if (extremeMode == 1 && playMode != 0) {
        display.setSegments(PianoPlay);
      }
    }
  }
  previous = reading;
  if (playMode == 1 || playMode == 2) {
    if (digitalRead(ExtremeButton) == HIGH) {
      digitalWrite(ExtremeLED, HIGH);
      if (extremeModeSelected == false) {
        if (extremeMode == 0) {
          previousMillis2 = currentMillis;
        }
        extremeModeSelected = true;
      }
    }
    else {
      digitalWrite(ExtremeLED, LOW);
      extremeModeSelected = false;
      extremeMode = 0;
      fading();
      RGBcolor(map(pitch, 1, 8, 0, 255), map(pitch, 1, 8, 255, 0), 0);
      readSwitches();
      readPots();
    }
  }
  else {
    standby();
  }
  if (extremeModeSelected == false) {
    if (dualNote == dualPreviousNote) {
      MIDImessage(NoteOFF, false, dualPreviousNote, velocity);
      dualPreviousNote = -1;
    }
    if (playMode == 1 && currentMillis - previousMillis >= tempo) {
      previousMillis = currentMillis;
      playMode = 2;
    }
    LEDsReset = false;
    if (playMode == 2) {
      if (currentMillis - previousMillis >= duration) {
        previousMillis = currentMillis;
        playMode = 1;
        noTone(DigitalOutSignal);
        MIDImessage(NoteOFF, true, previousNote, velocity);
        previousNote = -1;
        stepNumber++;
        if (stepNumber >= 8) {
          stepNumber = 0;
        }
      }
      else {
        tone(DigitalOutSignal, steps[stepNumber] * pitch);
        note = map(steps[stepNumber] * pitch, 32, 8440, 0, 127);
        if (displayingPotValue == false) {
          display.showNumberDec(steps[stepNumber] * pitch, false);
        }
        if (note > 127) {
          note = 127;
        }
        if (note != previousNote) {
          MIDImessage(NoteOFF, true, previousNote, velocity);
          MIDImessage(NoteON,true, note, velocity);
          previousNote = note;
          vibrate(75);
        }
        MIDIplaying = true;
        brightness[stepNumber] = 255;
      }
    }
    settingsFirstRun = true;
  }
  else {
    if (currentMillis - previousMillis2 < modeSelection) {
      noTone(DigitalOutSignal);
      if (MIDIplaying == true) {
        MIDImessage(NoteOFF, true, previousNote, velocity);
        MIDIplaying = false;
        previousNote = -1;
      }
      if (LEDsReset == false) {
        resetLEDs(0, 7);
      }
      readSwitches();
      if (digitalRead(StepButton[0]) == HIGH) {
        extremeMode = 1;
        previousMillis2 = 1;
      }
      else if (digitalRead(StepButton[1]) == HIGH) {
        extremeMode = 2;
        previousMillis2 = 1;
      }
      else if (digitalRead(StepButton[2]) == HIGH) {
        extremeMode = 3;
        previousMillis2 = 1;
        randomSeed(analogRead(A15));
      }
      else if (digitalRead(StepButton[3]) == HIGH) {
        extremeMode = 4;
        previousMillis2 = 1;
      }
      display.setSegments(Select);
      RGBcolor(MenuColor);
      analogWrite(StepLED[0], 70);
      analogWrite(StepLED[1], 70);
      analogWrite(StepLED[2], 70);
      analogWrite(StepLED[3], 70);
    }
    else {
      if (extremeMode == 1) {
        piano();
      }
      else if (extremeMode == 2) {
        frequencyGenerator();
      }
      else if (extremeMode == 3) {
        randomGenerator();
      }
      else if (extremeMode == 4) {
        settings();
      }
      else if (extremeMode == 0) {
        display.setSegments(NoSelect);
        RGBcolor(map(analogRead(AnalogInPitch), 0, 1023, 0, 255), map(analogRead(AnalogInDuration), 0, 1023, 0, 255), map(analogRead(AnalogInTempo), 0, 1023, 0, 255));
        for (byte x = 0; x < 8; x++) {
          analogWrite(StepLED[x], map(analogRead(AnalogInFrequency), 0, 1023, 0, 255));
        }
      }
    }
  }
}

void readPots() {
  tempo = map(analogRead(AnalogInTempo), 0, 1023, 0, 1000);
  duration = map(analogRead(AnalogInDuration), 0, 1023, 1, 1000);
  if (analogRead(AnalogInPitch) > 900) {
    pitch = 900;
  }
  else {
    pitch = analogRead(AnalogInPitch);
  }
  pitch = map(pitch, 0, 900, 1, 8);
  if (extremeModeSelected == false) {
    dualTempo = tempo;
    dualDuration = duration;
    dualPitch = pitch;
  }
  for (byte q = 0; q < 4; q++) {
    mapCC[q] = map(analogRead(MIDIccPot[q]), 0, 1023, 0, 127);
    if (abs(mapCC[q] - previousCC[q]) > potSmoothValue) {
      rotatingPot[q] = true;
      potRotatingMillis[q] = currentMillis;
    }
    if (previousPushedStep < 10) {
      displayingPotValue = false;
    }
    if (rotatingPot[q] == true && currentMillis - potRotatingMillis[q] < 600) {
      if (mapCC[q] != previousCC[q]) {
        MIDImessage(MIDIcc, true, DeviceNumberCC[q], mapCC[q]);
        previousCC[q] = mapCC[q];
        if (extremeMode == 1 && previousPushedStep > 10) {
          displayingPotValue = true;
          display.showNumberDec(previousCC[q], false);
        }
      }
    }
    else {
      rotatingPot[q] = false;
    }
  }
}

void readSwitches() {
  if (extremeModeSelected == false) {
    for (byte btn; btn < 8; btn++) {
      if (digitalRead(StepButton[btn]) == HIGH) {
        steps[btn] = analogRead(AnalogInFrequency) + 32;
      }
    }
  }
}

void fading() {
  fadeTime = map(tempo, 0, 1000, 1, 60);
  fadeValue = map(tempo + duration, 1, 2001, 8, 1);
  if (currentMillis - previousMillis1 >= fadeTime) {
    previousMillis1 = currentMillis;
    for (byte led = 0; led < 8; led++) {
      if (brightness[led] - fadeValue < 0) {
        brightness[led] = 0;
      }
      else {
        brightness[led] = brightness[led] - fadeValue;
      }
      analogWrite(StepLED[led], brightness[led]);
    }
  }
}

void dualSequencer() {
  if (playMode == 1 && currentMillis - previousMillis >= dualTempo) {
    previousMillis = currentMillis;
    playMode = 2;
  }
  if (playMode == 2) {
    if (currentMillis - previousMillis >= dualDuration) {
      previousMillis = currentMillis;
      playMode = 1;
      MIDImessage(NoteOFF, true, dualPreviousNote, velocity);
      dualPreviousNote = -1;
      stepNumber++;
      if (stepNumber >= 8) {
        stepNumber = 0;
      }
    }
    else {
      dualNote = map(steps[stepNumber] * dualPitch, 32, 8440, 0, 127);
      if (dualNote > 127) {
        dualNote = 127;
      }
      if (dualNote != dualPreviousNote) {
        MIDImessage(NoteOFF, true, dualPreviousNote, velocity);
        MIDImessage(NoteON, true, dualNote, velocity);
        dualPreviousNote = dualNote;
      }
    }
  }
}

void piano() {
  if (digitalRead(DualModeSwitch) == LOW) {
    dualSequencer();
  }
  else if (dualNote == dualPreviousNote) {
    MIDImessage(NoteOFF, true, dualPreviousNote, velocity);
    dualPreviousNote = -1;
  }
  readPots();
  RGBcolor(map(pitch, 1, 8, 0, 255), map(pitch, 1, 8, 255, 0), 0);
  previousPushedStep = -1;
  for (byte x = 0; x < 8; x++) {
    if (digitalRead(StepButton[x]) == HIGH) {
      previousPushedStep = x;
    }
  }
  if (previousPushedStep < 8) {
    tone(DigitalOutSignal, keyTone[previousPushedStep] * (pitch * 2));
    if (displayingPotValue == false) {
      display.showNumberDec(keyTone[previousPushedStep] * (pitch * 2), false);
    }
  }
  for (byte key; key < 8; key++) {
    if (digitalRead(StepButton[key]) == HIGH) {
      keyON[key] = 1;
    }
    if (keyON[key] == 1 && digitalRead(StepButton[key]) == LOW) {
      keyON[key] = 0;
      analogWrite(StepLED[key], 0);
      if (MIDIsent[key] == true) {
        MIDImessage(NoteOFF, false, previousKey[key], velocity);
        MIDIsent[key] = false;
      }
    }
    if (keyON[key] == 1 && digitalRead(StepButton[key]) == HIGH) {
      analogWrite(StepLED[key], 255);
      note = map(keyTone[key] * pitch * 2, 32, 1056, 0, 127);
      if (MIDIsent[key] == false) {
        MIDImessage(NoteON, false, note, velocity);
        previousKey[key] = note;
        MIDIsent[key] = true;
        vibrate(120);
      }
    }
    keyOFF = keyOFF + keyON[key];
  }
  if (keyOFF == 0) {
    resetLEDs(0, 7);
    noTone(DigitalOutSignal);
  }
  keyOFF = 0;
}

void frequencyGenerator() {
  for (byte key = 2; key < 6; key++) {
    if (digitalRead(StepButton[key]) == HIGH) {
      keyON[key] = 1;
    }
    if (keyON[key] == 1 && digitalRead(StepButton[key]) == LOW) {
      keyON[key] = 0;
    }
    if (keyON[key] == 1 && digitalRead(StepButton[key]) == HIGH) {
      frequency = analogRead(AnalogInFrequency) + 32;
    }
    keyOFF = keyOFF + keyON[key];
  }
  if (keyOFF == 0) {
    resetLEDs(2, 5);
    analogWrite(StepLED[0], 3);
    analogWrite(StepLED[7], 3);
    analogWrite(StepLED[1], 80);
    analogWrite(StepLED[6], 80);
  }
  else {
    for (byte leds = 2; leds < 6; leds++) {
      analogWrite(StepLED[leds], 120);
    }
    readPots();
    analogWrite(StepLED[0], 1);
    analogWrite(StepLED[7], 1);
    analogWrite(StepLED[1], 8);
    analogWrite(StepLED[6], 8);
  }
  keyOFF = 0;
  if (digitalRead(StepButton[1]) == HIGH) {
    frequency = frequency - 1;
    if (frequency < 32) {
      frequency = 32;
    }
    vibrate(50);
    readPots();
  }
  else if (digitalRead(StepButton[6]) == HIGH) {
    frequency = frequency + 1;
    if (frequency > 9999) {
      frequency = 9999;
    }
    vibrate(50);
    readPots();
  }
  if (digitalRead(StepButton[0]) == HIGH && preciseDown == LOW) {
    frequency = frequency - 1;
    if (frequency < 32) {
      frequency = 32;
    }
    preciseDown = HIGH;
    vibrate(255);
  }
  else if (digitalRead(StepButton[7]) == HIGH && preciseUp == LOW) {
    frequency = frequency + 1;
    if (frequency > 9999) {
      frequency = 9999;
    }
    preciseUp = HIGH;
    vibrate(255);
  }
  if (digitalRead(StepButton[0]) == LOW) {
    preciseDown = LOW;
  }
  if (digitalRead(StepButton[7]) == LOW) {
    preciseUp = LOW;
  }
  while (frequency * pitch > 9999) {
    frequency = frequency - ((frequency * pitch) - 9999);
  }
  RGBcolor(map(pitch, 1, 8, 0, 255), map(pitch, 1, 8, 255, 0), 0);
  if (displayingPotValue == false) {
    display.showNumberDec(frequency * pitch, false);
  }
  tone(DigitalOutSignal, frequency * pitch);
}

void randomGenerator() {
  readPots();
  if (currentMillis - previousMillis1 > duration) {
    previousMillis1 = currentMillis;
    frequency = random(32, 1055);
    tone(DigitalOutSignal, frequency);
    if (displayingPotValue == false) {
      display.showNumberDec(frequency, false);
    }
    RGBcolor(random(0, 255), random(0, 255), random(0, 255));
    for (byte e = 0; e < 8; e++) {
      analogWrite(StepLED[e], random(1, 3));
    }
  }
}

void settings() {
  if (settingsFirstRun == true) {
    resetLEDs(1, 6);
    analogWrite(StepLED[0], 255);
    segmentCharacter = 0;
    textScrollingMillis = currentMillis;
    settingsFirstRun = false;
    MIDIchannelSettings = 0;
    channelSettingsFirstRun = false;
  }
  if (channelSettingsFirstRun == true) {
    analogWrite(StepLED[0], 8);
    analogWrite(StepLED[7], 8);
    analogWrite(StepLED[3], 255);
    analogWrite(StepLED[4], 255);
    RGBcolor(255,255,0);
  }
  if (MIDIchannelSettings == 0) {
    if (digitalRead(StepButton[0]) == HIGH) {
      MIDIchannelSettings = 1;
      channelSettingsFirstRun = true;
    }
    for (byte x = 0; x < 4; x++) {
      if (x + segmentCharacter > 28) {
        onDisplayCharacter[x] = SettingsText[x + (segmentCharacter - 29)];
      }
      else {
        onDisplayCharacter[x] = SettingsText[x + segmentCharacter];
      }
    }
    if (segmentCharacter > 29) {
      segmentCharacter = 0;
    }
    if (segmentCharacter == 0) {
      scrollingTime = 2000;
      nextSegment = false;
    }
    if (nextSegment == false && onDisplayCharacter[0] == M) {
      display.setSegments(onDisplayCharacter);
    }
    if (currentMillis - textScrollingMillis > scrollingTime) {
      textScrollingMillis = currentMillis;
      if (scrollingTime == 2000) {
        nextSegment = true;
        scrollingTime = 300;
      }
      display.setSegments(onDisplayCharacter);
      if (nextSegment == true) {
        segmentCharacter = segmentCharacter + 1;
      }
    }
    if (digitalRead(StepButton[7]) == HIGH && previousVibrateButton == LOW) {
      if (currentMillis - debounceMillis >= 500) {
        debounceMillis = currentMillis;
        previousVibrateButton = HIGH;
        if (allowVibration == false) {
          allowVibration = true;
          vibrate(600);
          EEPROM.write(0, true);
        }
        else {
          allowVibration = false;
          EEPROM.write(0, false);
        }
      }
    }
    else if (digitalRead(StepButton[7]) == LOW) {
      previousVibrateButton = LOW;
    }
    if (allowVibration == true) {
      analogWrite(StepLED[7], 255);
      RGBcolor(10, 255, 0);
    }
    else {
      analogWrite(StepLED[7], 8);
      RGBcolor(255, 0, 0);
    }
  }
  else if (MIDIchannelSettings == 1) {
    if (digitalRead(StepButton[0]) == LOW && channelSettingsFirstRun == true) {
      channelSettingsFirstRun = false;
    }
    if (channelSettingsFirstRun == false) {
      if (digitalRead(StepButton[0]) == HIGH && channelButtonPrevious[0] == LOW) {
        channelButtonPrevious[0] = HIGH;
        if ((mainChannel - 1) < 0) {
          mainChannel = 0;
          vibrate(800);
        }
        else {
          mainChannel = mainChannel - 1;
          vibrate(200);
        }
        analogWrite(StepLED[0], 255);
      }
      else if (digitalRead(StepButton[0]) == LOW) {
        channelButtonPrevious[0] = LOW;
        analogWrite(StepLED[0], 8);
      }
      if (digitalRead(StepButton[7]) == HIGH && channelButtonPrevious[1] == LOW) {
        channelButtonPrevious[1] = HIGH;
        if ((mainChannel + 1) > 15) {
          vibrate(800);
        }
        else {
          mainChannel = mainChannel + 1;
          vibrate(200);
        }
        analogWrite(StepLED[7], 255);
      }
      else if (digitalRead(StepButton[7]) == LOW) {
        channelButtonPrevious[1] = LOW;
        analogWrite(StepLED[7], 8);
      }
    }
    onDisplayCharacter[0] = 0b10000110;
    onDisplayCharacter[1] = 0b01000000;
    if ((mainChannel + 1) < 10) {
      onDisplayCharacter[2] = blank;
      onDisplayCharacter[3] = numberSegment[mainChannel + 1];
    }
    else {
      onDisplayCharacter[2] = numberSegment[1];
      onDisplayCharacter[3] = numberSegment[(mainChannel + 1) % 10];
    }
    display.setSegments(onDisplayCharacter);
    if (digitalRead(StepButton[3]) == HIGH || digitalRead(StepButton[4]) == HIGH) {
      MIDIchannelSettings = 2;
      channelSettingsFirstRun = true;
      EEPROM.write(1, mainChannel);
      vibrate(800);
    }
  }
  else if (MIDIchannelSettings == 2) {
    if (digitalRead(StepButton[3]) == LOW && digitalRead(StepButton[4]) == LOW && channelSettingsFirstRun == true) {
      channelSettingsFirstRun = false;
    }
    if (channelSettingsFirstRun == false) {
      if (digitalRead(StepButton[0]) == HIGH && channelButtonPrevious[0] == LOW) {
        channelButtonPrevious[0] = HIGH;
        if ((secondChannel - 1) < 0) {
          secondChannel = 0;
          vibrate(800);
        }
        else {
          secondChannel = secondChannel - 1;
          vibrate(200);
        }
        analogWrite(StepLED[0], 255);
      }
      else if (digitalRead(StepButton[0]) == LOW) {
        channelButtonPrevious[0] = LOW;
        analogWrite(StepLED[0], 8);
      }
      if (digitalRead(StepButton[7]) == HIGH && channelButtonPrevious[1] == LOW) {
        channelButtonPrevious[1] = HIGH;
        if ((secondChannel + 1) > 15) {
          vibrate(800);
        }
        else {
          secondChannel = secondChannel + 1;
          vibrate(200);
        }
        analogWrite(StepLED[7], 255);
      }
      else if (digitalRead(StepButton[7]) == LOW) {
        channelButtonPrevious[1] = LOW;
        analogWrite(StepLED[7], 8);
      }
    }
    onDisplayCharacter[0] = 0b11011011;
    onDisplayCharacter[1] = 0b01000000;
    if ((secondChannel + 1) < 10) {
      onDisplayCharacter[2] = blank;
      onDisplayCharacter[3] = numberSegment[secondChannel + 1];
    }
    else {
      onDisplayCharacter[2] = numberSegment[1];
      onDisplayCharacter[3] = numberSegment[(secondChannel + 1) % 10];
    }
    display.setSegments(onDisplayCharacter);
    if ((digitalRead(StepButton[3]) == HIGH || digitalRead(StepButton[4]) == HIGH) && channelSettingsFirstRun == false) {
      settingsFirstRun = true;
      EEPROM.write(2, secondChannel);
      vibrate(800);
    }
  }
}

void MIDImessage(byte MIDIstatus, bool onMainChannel, byte data1, byte data2) {
  if (onMainChannel == true) {
    Serial.write(MIDIstatus | mainChannel);
  }
  else {
    Serial.write(MIDIstatus | secondChannel);
  }
  Serial.write(data1);
  Serial.write(data2);
}

void resetLEDs(byte from, byte to) {
  for (byte led = from; led < to + 1; led++) {
    analogWrite(StepLED[led], 0);
  }
  LEDsReset = true;
}

void RGBcolor(byte red_light_value, byte green_light_value, byte blue_light_value) {
  if (RGBCommonCathode == true) {
    analogWrite(RedLED, map(red_light_value, 0, 255, 0, RGBcalculatedBrightness));
    analogWrite(GreenLED, map(green_light_value, 0, 255, 0, RGBcalculatedBrightness));
    analogWrite(BlueLED, map(blue_light_value, 0, 255, 0, RGBcalculatedBrightness));
  }
  else {
    analogWrite(RedLED, map(red_light_value, 0, 255, 255, RGBcalculatedBrightness));
    analogWrite(GreenLED, map(green_light_value, 0, 255, 255, RGBcalculatedBrightness));
    analogWrite(BlueLED, map(blue_light_value, 0, 255, 255, RGBcalculatedBrightness));
  }
}

void vibrate(int duration) {
  if (allowVibration == true) {
    previousMillisVibration = millis() + modeSelection;
    vibrateDuration = duration;
    vibration = true;
  }
}

void standby() {
  if (currentMillis - previousMillis1 >= standbyFadeTime) {
    previousMillis1 = currentMillis;
    standbyFadeTime = 10;
    if (standbyDimming == true) {
      if (standbyFadeValue - standbyFadeNumber < 0) {
        standbyFadeValue = 0;
      }
      else {
        standbyFadeValue = standbyFadeValue - standbyFadeNumber;
      }
      if (standbyFadeValue == 0) {
        standbyFadeValue = 0;
        standbyDimming = false;
        standbyFadeTime = 2500;
      }
    }
    else {
      if (standbyFadeValue + standbyFadeNumber > 255) {
        standbyFadeValue = 255;
      }
      else {
        standbyFadeValue = standbyFadeValue + standbyFadeNumber;
      }
      if (standbyFadeValue == 255) {
        standbyFadeValue = 255;
        standbyDimming = true;
      }
    }
    analogWrite(PowerLED, standbyFadeValue);
    RGBcolor(map(standbyFadeValue, 0, 255, 0, RGBStandbyMaxBrightness), map(standbyFadeValue, 0, 255, 0, RGBStandbyMaxBrightness), map(standbyFadeValue, 0, 255, 0, RGBStandbyMaxBrightness));
  }
}
