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
const unsigned int SENSOR_PIN = A0;
const unsigned int BAUD_RATE = 57600;

/*
  New one Wire bus
*/
#define ONE_WIRE_BUS 2

/* 
  RECV_PIN = IR reciever 
  FAN is Fan pin
  BUZZER is the buzzer pin
  number is testing of the 7 segment display

*/
const int RECV_PIN = 12;
int FAN_PIN = 11;
int BUZZER_PIN = 1;
int number = 0;
const byte numChars = 8;
char receivedChars[numChars];
int threashold = 20;
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
  //pinMode(SENSOR_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(FAN_PIN,LOW);
  sensors.begin();
}
/*
 * 
 * LOOP
 * first detect if the button has been pressed and wait for the state to be updated then depending on which state do smart fan or
 * memory game.
 * 
*/
void loop() {
  // put your main code here, to run repeatedly:
  SmartFan();
}
/*
 * SMART FAN
 * 
 * Simple smart fan device that uses a one wire device to access the temperatures of the outside then print to terminal.
 * Then checking whether the temperature is over 20C and turning the fan on.
*/
void SmartFan(){
  /* LEGACY CODE
    int temp = int(Thermister(analogRead(SENSOR_PIN)));
    Serial.print("T");
    Serial.println(temp);
  */
  //Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  //Serial.println("DONE");
  float tempC = sensors.getTempCByIndex(0);
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    //Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(round(tempC));
  }
  //recvWithEndMarker();
  //updateThreashold();
  //Serial.println(threashold);
  if(Serial.available() > 0)
  {
    byte bstatus = Serial.read();
    switch(bstatus){
      case '1':
        digitalWrite(FAN_PIN,HIGH);
        break;
      case '0':
        digitalWrite(FAN_PIN,LOW);
        break;
    }
    /*if(a = 0)
    {
      digitalWrite(FAN_PIN,HIGH);
    }
    else
    {
        digitalWrite(FAN_PIN,LOW);
    }*/
  }
}

void recvWithEndMarker(){
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
   
    while(Serial.available() != 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void updateThreashold(){
  if (newData == true){
      threashold = 0;
      threashold = atoi(receivedChars);
      newData = false;
    }
}

/*
 * MEMORY GAME
 * 
 * CURRENTLY NOT IMPLEMENTED
*/void MemoryGame(){
  // pinMode(buzzer, OUTPUT); UNCOMMENT
  int wait_time = 0.5;     //time to wait between beeps, in seconds.
  int length = 0.4;        //length of time of beeps, in seconds.
  int pitch_variation = 0; //how much the beeps change in tone.
  //TODO add a scoring system.
  int current_stage = generate_stage();
  for (int i = 0; i < 5; i++)
  {
      process_level(current_stage, wait_time, length, pitch_variation);
      increase_difficulty(i, wait_time, length, pitch_variation);
  }
  /* Reading from Terminal */
  if(irrecv.decode(&results))
  {
    
    Serial.print("I");
    Serial.println(results.value, HEX);
    if(results.value == "FF9867"){
      digitalWrite(BUZZER_PIN,HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN,LOW);
      delay(100);
      digitalWrite(BUZZER_PIN,HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN,LOW);
      delay(100);
     //NewTone(BUZZER,6000,100);
    }
    irrecv.resume();
  }
  sevseg.setNumber(number);
  sevseg.refreshDisplay();
  number++;
  if(number > 9)
  {
    number = 0;
  }
}
int generate_stage()
{
    return (rand() % (9)) + 1;
}

int generate_pitch(int pitch_variation)
{
    return 1000 * (rand() % pitch_variation);
}
//processes the ir input and returns a number representing the player's answer.
int process_input()
{
    int answer = -1; //TODO this is gross but it works
    //loop until an answer is found
    while (answer == -1)
    {
        //TODO get data
        printf("Your answer:\n > ");
        scanf("%d", &answer);
    }
    return answer;
}

//tell the arduino to beep x times (controlled by stage variable)
void play_level(int stage, int wait_time, int length, int pitch_variation)
{
    //conversion to milliseconds
    int _wait_time = wait_time * 1000;
    int _length = length * 1000;
    for (int i = 0; i < stage; i++)
    {
        // tone(buzzer, generate_pitch(pitch_variation)); // Send 1KHz sound signal... UNCOMMENT
        // delay(_length);                                // ...for 1 sec
        // noTone(buzzer);                                // Stop sound...
        // delay(_wait_time);                             // ...for 1sec
        //tell the thing to beep (with wait_time between beeps, and length of beep)
        printf("beep ");
    }
    printf("\n");
}
//print the answer of the level to the display.
void show_answer(int stage)
{
    //TODO
    printf("answer is %d\n", stage);
}

//run through the level
void process_level(int current_stage, int wait_time, int length, int pitch_variation)
{
    int stage = 0;
    //TODO add a cap for the number of attempts at a stage.
    while (stage < 3)
    { //checks for three consecutive correct answers
        play_level(current_stage, wait_time, length, pitch_variation);
        int answer = process_input();
        show_answer(current_stage);
        if (answer == current_stage)
        {
            printf("Correct!\n");
            //show them the answer, and iterate the stage by one to progress them.
            stage += 1;
        }
        //else, show the player the answer, generate a new stage and get them to try again
        current_stage = generate_stage();
    }
}

void increase_difficulty(int scalar, int wait_time, int length, int pitch_variation)
{
    //TODO improve
    wait_time -= scalar * 0.1;
    length -= scalar * 0.1;
    pitch_variation *= 1.5;
    printf("level difficulty increased\n");
}
