#include <LiquidCrystal.h>
#include<SoftwareSerial.h>
#include "DHT.h"
#include<Servo.h>
#include <Adafruit_Sensor.h>

SoftwareSerial mySerial(9,10);

#define LIGHTBULB_PIN 30
#define MoistureSensorPin A0 
#define PH_PIN A3
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);
#define WATER_PUMP 38
#define WATER_SPRAY 40
#define EXHAUST_FAN_FORWARD 34
#define EXHAUST_FAN_REVERSE 35

// LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(26, 27, 22, 23, 24, 25);
Servo shadeMotor;
// initially shade is closed
int pos = 0; // variable to store the servo position
bool isShadeOpen = false;
// for sending message using GSM
bool isPhLowMessageSent = false;
bool isPhHighMessageSent = false;
bool isOverSaturatedMessageSent = false;
String crop;
void setup() {
  lcd.begin(16,4);
  mySerial.begin(9600); // Baud rate for GSM module
  Serial1.begin(9600); // Baud rate for Serial monitor
  dht.begin();
  shadeMotor.attach(4); // servo for Shade on pin 4
  shadeMotor.write(0);
  Serial1.println("Crop:");
  
  crop=Serial1.readString();
  pinMode(LIGHTBULB_PIN, OUTPUT); // setting output pin for light bulbs
  pinMode(A2,INPUT);
  pinMode(PH_PIN,INPUT);

  // initializing pinMode for Exhaust Fan
  pinMode(EXHAUST_FAN_FORWARD, OUTPUT);  
  pinMode(EXHAUST_FAN_REVERSE, OUTPUT); 

  // initializing pinMode for Water pump & sprayer
  pinMode(WATER_PUMP,OUTPUT);
  pinMode(WATER_SPRAY,OUTPUT);
  turn_off_waterPump();
  turn_off_waterSpray();

  mySerial.begin(9600);   // Setting the baud rate of GSM Module  
  Serial.begin(9600);    // Setting the baud rate of Serial Monitor (Arduino)
}

void loop() {
  delay(500);
  int moistureSensorValue = analogRead(MoistureSensorPin);
  int moistureValuePercent = map(moistureSensorValue,0,1023,0,100); // range 1-100%
  float moistureSensorVolts = analogRead(MoistureSensorPin)*0.0048828125; 

  // Checking moisture condition in %
  check_moisture(moistureValuePercent);

  float humidityValue = dht.readHumidity();
  float temperatureValue = dht.readTemperature();
  if (isnan(temperatureValue)) {
    Serial1.println("Failed to read from DHT sensor!");
  }
  float phValue = analogRead(PH_PIN);
  phValue = (phValue/1023)*14;

  bool adjusting_temperature = check_temperature(temperatureValue);
  check_humidity(humidityValue,adjusting_temperature);
  check_ph(phValue);
}

// Adding delay
void add_delay(){
  delay(200);
}
//For GSM message
void sms_ph_high()
{
  if(!isPhHighMessageSent){
    isPhHighMessageSent = true;
    mySerial.println("Warning:PH of too high");
    mySerial.println("Use sulfur, aluminum sulfate or sulfuric acid");
    mySerial.println(crop);
    delay(100);
    mySerial.println((char)26);
  }
}
void sms_ph_low()
{
  if(!isPhLowMessageSent){
    isPhLowMessageSent = true;
    mySerial.println("Warning:PH of too low");
    mySerial.println("Use dolomite lime, baking soda");
    delay(100);
    mySerial.println((char)26);
  }
}
void sms_oversaturation()
{
  if(!isOverSaturatedMessageSent){
    isOverSaturatedMessageSent = true;
    mySerial.println("Warning:Oversaturation- Please improve drainage system");
    delay(100);
    mySerial.println((char)26);
  }
}

void exhaust_fan_air_in(){
  digitalWrite(EXHAUST_FAN_FORWARD, HIGH);
  digitalWrite(EXHAUST_FAN_REVERSE, LOW); 
}
void exhaust_fan_air_out(){
  digitalWrite(EXHAUST_FAN_FORWARD, LOW);
  digitalWrite(EXHAUST_FAN_REVERSE, HIGH); 
}
void exhaust_fan_stop(){
  digitalWrite(EXHAUST_FAN_FORWARD, LOW);
  digitalWrite(EXHAUST_FAN_REVERSE, LOW); 
}
void light_bulb_on(){
  digitalWrite(LIGHTBULB_PIN, HIGH);
}
void light_bulb_off(){
  digitalWrite(LIGHTBULB_PIN, LOW);
}
void turn_off_waterPump() {
  digitalWrite(WATER_PUMP, LOW);
}
void turn_on_waterPump() {
  digitalWrite(WATER_PUMP, HIGH);
}
void turn_off_waterSpray() {
  digitalWrite(WATER_SPRAY, LOW);
}
void turn_on_waterSpray() {
  digitalWrite(WATER_SPRAY, HIGH);
}
void close_shade() {
  for (; pos >= 0; pos=pos-2) { 
    shadeMotor.write(pos);              
    delay(15);                       
  }
}
// Open roof shade, pos has to be 180
void open_shade() {
  for (; pos <= 180; pos=pos+2) { 
    shadeMotor.write(pos);              
    delay(15);                       
  }
}

void print_moistureValue(int value) {
  lcd.clear();
  lcd.print("Moisture is: ");
  lcd.print(value);
  lcd.print("%");
}
void check_moisture(int value) {
  if(value < 20) {
    isOverSaturatedMessageSent = false;
    print_moistureValue(value);
    lcd.setCursor(0,1);
    lcd.print("Need irrigation!");
    lcd.setCursor(0,2);
    lcd.print("Turning on");
    lcd.setCursor(0,3);
    lcd.print("Water pump");
    turn_on_waterPump();
  } else if(value >=20 && value <= 60) {
    isOverSaturatedMessageSent = false;
    print_moistureValue(value);
    lcd.setCursor(0,1);
    lcd.print("Turning off");
    lcd.setCursor(0,2);
    lcd.print("Water pump");
    turn_off_waterPump();
  } else {
    // Notify user about Over saturation using GSM
    print_moistureValue(value);
    sms_oversaturation();
    turn_off_waterPump();
  }
  delay(100);
}
 
void check_ph(float phValue){
  //5.5-6.5 for optimal plant growth 
  lcd.clear(); 
  lcd.setCursor(0,1);
  lcd.print(String("PH = ") + String(phValue));
  lcd.setCursor(0,2);
  if(phValue > 8){
     isPhLowMessageSent = false;
     sms_ph_high();

     
  }
  else if(phValue > 7.5){
    if(crop.equals("asparagus") || crop.equals("garlic") || crop.equals("beans")){
      isPhHighMessageSent = false;
      isPhLowMessageSent = false;
      mySerial.println("Stable ph");
      mySerial.println(crop);
    }
    else{
      isPhLowMessageSent = false;
      sms_ph_high();
    }
  }
  else if(phValue < 6.5 && phValue>=5.5){
    if(crop.equals("rice") || crop.equals("mango") || crop.equals("cashew")){
      isPhHighMessageSent = false;
      isPhLowMessageSent = false;
      mySerial.println("Stable ph");
    }
    else{
      isPhHighMessageSent = false;
      sms_ph_low();
    }
  }
  else if(phValue < 5.5){
     isPhHighMessageSent = false;
     sms_ph_low();

  }
  else {
    // pH is normal
    isPhHighMessageSent = false;
    isPhLowMessageSent = false;
    mySerial.println("Stable ph");
  }
  add_delay();
}
 
void check_humidity(float humidityValue, bool adjusting_temperature){
  //60-80% humidity for optimal plant growth
  if(adjusting_temperature){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Adjust Temp");
    add_delay();
    add_delay();
    return;
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(String("Hum = ") + String(humidityValue));
  if(humidityValue < 50){
     turn_on_waterSpray();
     exhaust_fan_stop(); 
  }else if(humidityValue > 80){
      exhaust_fan_air_out();
      turn_off_waterSpray();
  }else{
      exhaust_fan_stop();
      turn_off_waterSpray();
  }
  add_delay();
}

bool check_temperature(float temperatureValue){
    if(temperatureValue < 20){
      //shade open
      if(!isShadeOpen) {
        Serial1.print("Opening...");
        open_shade();
        isShadeOpen = true;
      }
      turn_off_waterSpray();
      light_bulb_on();
      exhaust_fan_stop();
      return true;
    }else if(temperatureValue > 25){
      turn_on_waterSpray();
      //shade close
      if(isShadeOpen) {
        Serial1.print("Closing...");
        close_shade();
        isShadeOpen = false;
      }
      exhaust_fan_air_in();
      light_bulb_off();
      return true;
    }else{
      //shade open
      if(!isShadeOpen) {
        Serial1.print("Opening...");
        open_shade();
        isShadeOpen = true;
      }
      turn_off_waterSpray();
      exhaust_fan_stop(); 
      light_bulb_off();
      return false;
    }
    add_delay();
}
