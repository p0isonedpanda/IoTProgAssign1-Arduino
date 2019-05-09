#include <NewTone.h> // Needed as the original tone library conflicts with the IR remote libary
#include <SevSeg.h> //Drives the 7 segment display
#include <IRremote.h> //IR remote libray
#include <math.h> //Legacy operations
#include <OneWire.h> //Handling new temp sensor as it is a one wire sensor
#include <DallasTemperature.h> //prewritten libray to handle the one wire information to convert to temp

/*
  SENSOR_PIN legacy as it was the old sensor
  BAUD_RATE is the serial rate
*/
//const unsigned int SENSOR_PIN = A0;
const unsigned int BAUD_RATE = 57600;

/*
  New one Wire bus
*/
#define ONE_WIRE_BUS A0

/* 
  RECV_PIN = IR reciever 
  FAN is Fan pin
  BUZZER is the buzzer pin

*/
const int RECV_PIN = 12;
const int FAN_PIN = 11;
const int BUZZER_PIN = 2;
/*
 * BUTTON
 * BUTTON_PIN = button pin
 * the following variables are unsigned longs because the time, measured in
 * milliseconds, will quickly become a bigger number than can be stored in an int.
 * 
*/
const int BUTTON_PIN = 13;
int state = LOW;
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

/*
 *  IR REMOTE CODE STRING and Serial interpretation 
*/
String IRResult = "";
int DIFFICULTY;
int AmountOfBeeps;
bool GameStarted = false;
const byte numChars = 8;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;

/*
  irrecv is the the defintion for the ir remote
  results is decoding the binary sent from the ir recevier
  sevseg, 7 segment display
  OneWire, setting device up as a one wire device
  DallasTemp convers one wire data to Temperature
*/
IRrecv irrecv(RECV_PIN);
decode_results results;
SevSeg sevseg;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/*
 * LEGACY OPERATION
 * This was used as a way to calculate tempature based off a thermister, but due to the inconsistent nature and hard to properly test
 * moving the the DS18B20 as a One wire device cut down it the code and increased reliability
 * 
double Thermister (int RawADC){
 double Temp;
 Temp = log(((10240000/RawADC) - 9000));
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;            // Convert Kelvin to Celsius
 return Temp;
}
*/
/*
 * SETUP
 * Initialisation of the pins as well as setting the Serial Baud rate, preconfiguring the 7 segement display and setting up the 
 * ir reciever.
*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  byte numDigits = 1;
  byte digitPins{};
  byte segmentPins[] = {4, 3, 8, 9, 10, 5, 6, 7};
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_CATHODE;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);
  irrecv.enableIRIn();
  irrecv.blink13(true);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN,LOW);
  sensors.begin();
  noNewTone(BUZZER_PIN);
}
/*
 * 
 * LOOP
 * first detect if the button has been pressed and wait for the state to be updated then depending on which state do smart fan or
 * memory game.
 * Button code based off: https://www.arduino.cc/en/tutorial/switch and https://forum.arduino.cc/index.php?topic=506473.0
*/
void loop() {
  // put your main code here, to run repeatedly:
  int reading = digitalRead(BUTTON_PIN);

  if(reading != lastButtonState){
    lastDebounceTime = millis();
  }
  if((millis() - lastDebounceTime) > debounceDelay){
    if(reading != buttonState){
      buttonState = reading;

      if(buttonState == HIGH){
        state = !state;
      }
    }
  }
  lastButtonState = reading;
  switch(state){
  case HIGH:
    SmartFan();
    break;
  case LOW:
    MemoryGame();
    break;
  }
}
/*
 * SMART FAN
 * Simple smart fan device that uses a one wire device to access the temperatures of the outside then print to terminal.
 * Then checking whether the temperature is over 20C and turning the fan on.
*/
void SmartFan(){
  /* LEGACY CODE
    int temp = int(Thermister(analogRead(SENSOR_PIN)));
    Serial.print("T");
    Serial.println(temp);
  */
  GameStarted = false;
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0);
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    Serial.println(round(tempC));
  }
  if(Serial.available() > 0)
  {
    int bstatus = Serial.read() - '0';
    switch(bstatus){
      case 1:
        digitalWrite(FAN_PIN,HIGH);
        break;
      case 0:
        digitalWrite(FAN_PIN,LOW);
        break;
    }
  }
}
/*
 * MEMORY GAME
 * 
 * CURRENTLY IMPLMENTED
 * 
 * The users sends a code <DIFFICULT BEEBAMOUNT> then the arduino uses the first as how long is the delay
 * then the users is prompted to enter a number on the remote as to how many beeps they though it was
 * the actual amount is then displayed on then 7 segment display.
*/
void MemoryGame(){
  /* Recieving from Pi*/
  StartGame();
  if(GameStarted){
    noNewTone(BUZZER_PIN);
    recvWithStartEndMarkers();
    if(newData){
      /* 
       *  If the pi has issued a command it will take the first char as difficulty
       *  and the second as the amount of beeps
      */
      if(receivedChars[0] == 'a'){
        DIFFICULTY = 250;
      } else if(receivedChars[0] == 'b'){
        DIFFICULTY = 100;
      } else if(receivedChars[0] == 'c'){
        DIFFICULTY = 50;
      }
      //Deletes the '0' at the end of the conversion
      AmountOfBeeps = receivedChars[1] - '0';
      for(int i = 0; i < AmountOfBeeps; i++)
      {
        //Serial.println("BUZZ");
        //Serial.println(DIFFICULTY);
        NewTone(BUZZER_PIN,1000);
        delay(DIFFICULTY);
        noNewTone(BUZZER_PIN);
        delay(DIFFICULTY);
      }
      newData = false;
    }
    //Waits for user input and then displays the answer
    if(irrecv.decode(&results))
    {
      IRResult = String(results.value, HEX);
      //Serial.println(IRResult);
      if(IRResult == "ff30cf"){
        Serial.println("1");
      } 
      else if(IRResult == "ff18e7"){
        Serial.println("2");
      } 
      else if(IRResult == "ff7a85"){
        Serial.println("3");
      } 
      else if(IRResult == "ff10ef"){
        Serial.println("4");
      } 
      else if(IRResult == "ff38c7"){
        Serial.println("5");
      } 
      else if(IRResult == "ff5aa5"){
        Serial.println("6");
      } 
      else if(IRResult == "ff42bd"){
        Serial.println("7");
      } 
      else if(IRResult == "ff4ab5"){
        Serial.println("8");
      } 
      else if(IRResult == "ff52ad"){
        Serial.println("9");
      }
      sevseg.setNumber(AmountOfBeeps);
      sevseg.refreshDisplay();
      irrecv.resume();
    }
  }
}

/*
 * START GAME
 * Waits till a zero is sent to access the game features.
 * updates a bool accordingly
*/
bool StartGame(){
  if(Serial.available() > 0 && GameStarted == false)
  {
    //Serial.println("GAME STARTED");
    int bstatus = Serial.read() - '0';
    switch(bstatus){
      case 1:
        GameStarted = false;
        break;
      case 0:
        GameStarted = true;
        break;
    }
  }
}

/*
 * Terminal recieving the string of buzzer information 
 * code used form: https://forum.arduino.cc/index.php?topic=396450
*/

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}
