/*
 * Really rudimentary environmental sensor / fire control.
 * There is nothing special required for wiring hardware-
 * All of the sensors are bargain-basement kookye sensors.
 * 
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

#define OLED_RESET 4
#define dht_dpin A0         // Analog pin for temp/hum sensor
#define mq5_apin A1         // Analog pin for MQ5 gas detector
#define ok_pin 3            // green LED
#define ohshit_pin 2        // red LED
#define buzz_pin 4          // piezo "sensor" - may need more components if bare piezo
#define fire_pin A6         // Fire sensor
#define extinguisher_pin 21 // "extinguisher relay" - This won't actually fire unless you have decent relays and enough power going through them.
#define fire_threshold 70   // Voltage beneath this means we are on fire
/* #define barom_i2c 0x77   // */  /* we can use defaults for the basic barometer */
#define oled_i2c 0x3C       // This is the address of the DIY mall 128x96 OLED and many others

#define calcR0 8.65         // This is pulled from get_mq5_r0 - This is meant as a "baseline" that is calcluated against.
                            // This requires 24h initial burn in of the sensor plus min 15 min run time to stabilize
                            // R0 is the reading with "clean" air.
                            // This is ultimately used for a ratio that is well beyond my understanding, but apparently accurate
                            // enough for basic testing.


byte bGlobalErr;                       // Lifted from mq5 example code
byte dht_dat[5];                       // Also lifted
Adafruit_SSD1306 display(OLED_RESET);  // Lifted from adafruit examples
float tempF;
float mq5aout;
float myR0;
float myratio;
char globalstatus;                     // We buzz/go red if this is > 0
float curpressure;
bool sensor_works;                     // Skip pressure sensor if it doesn't work or can't be IDed
float fireval;                         // Analog reading for fire sensor.  Lower numbers are presumably a brighter or bigger fire.
bool on_fire;                          // If a fire has been detected or not.

// Pressure sensor, lifted from Adafruit examples.
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);



void setup() {
  InitDHT();
  pinMode(ohshit_pin,OUTPUT);
  pinMode(ok_pin,OUTPUT);
  pinMode(buzz_pin,OUTPUT);
  pinMode(extinguisher_pin,OUTPUT);
  digitalWrite(extinguisher_pin,LOW);
  globalstatus = 0;
  on_fire = false;
  buzz_alert();
  /* ALMOST ALL OF THIS IS LIFTED CODE */
  display.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, oled_i2c);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done

  // Clear the buffer.
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display();
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    display.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    display.display();
    delay(5000);
    sensor_works = false;
  }
  else {
    displaySensorDetails();
    sensor_works = true;
  }

  

}

void loop() {
  // put your main code here, to run repeatedly:
  globalstatus = 0;
  ReadDHT();
  if (sensor_works) {
    curpressure = get_pressure();
  }
  else {
    curpressure = -1;
  }
  
  mq5aout = analogRead(mq5_apin);
  myR0 = calcR0;
  //Uncomment the below to show the actual R0 in real time
  //myR0 = get_mq5_r0();
  myratio = get_mq5_ratio(myR0);

  switch (bGlobalErr){
    case 0:
  // Everything is working, no need to handle errors
  // The display routine should really be functionized.
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Temp/Hum/Presure plus");
  display.print("chance of exploding");
  display.setCursor(0,16);
  //display.print("TEMP: ");
  //display.print(dht_dat[2], DEC);
  //display.print(".");
  //display.print(dht_dat[3], DEC);
  //display.println("C  ");
  //display.print("      ");
  // tempC = (dht_dat[2] + (dht_dat[3] * .01));
  tempF = (((dht_dat[2] + (dht_dat[3] * .01)) * 1.8)+ 32);
  display.print(tempF,1);
  display.print("F");

  display.print("/");
  display.print(dht_dat[0], DEC);
  // display.print(".");
  // display.print(dht_dat[1], DEC);
  // ^ don't really need this much resolution for humidity
  display.print("%/");
  display.print(curpressure,2);
  display.println("hPA");
  display.print("Flammability: 1:");
  display.println(myratio, 1);
  // display.print("RAW: ");
  // display.print(analogRead(mq5_apin));
  // display.println("    ");
  // display.print("R0:" );
  // display.print(myR0);
  display.println("");

  if (myratio < 1) {
    globalstatus++;   
  }

  fireval = get_fire();
  if (fireval < fire_threshold) {
    on_fire = true;
    globalstatus++;
  }
  else {
    on_fire = false;
  }

  
  if (on_fire) {
    display.println("We are on fire.");
    digitalWrite(extinguisher_pin, LOW);   
  }
  else {
    display.println("We are NOT on fire.");
    digitalWrite(extinguisher_pin, HIGH);
  }

  // display.print("Fire val:");
  // display.println(fireval);

  
  if (globalstatus > 0) {
    digitalWrite(ok_pin,LOW);
    digitalWrite(ohshit_pin,HIGH);
    buzz_alert();
  }
  else {
    digitalWrite(ok_pin,HIGH);
    digitalWrite(ohshit_pin,LOW);
  }
  
         break;
    case 1:
      display.println("Error 1: DHT start condition 1 not met.");
      break;
    case 2:
       display.println("Error 2: DHT start condition 2 not met.");
       break;
    case 3:
      display.println("Error 3: DHT checksum error.");
      break;
    default:
      display.println("Error: Unrecognized code encountered.");
      break;
    }
  display.display();
  delay(800);
}

void InitDHT(){
   pinMode(dht_dpin,OUTPUT);
        digitalWrite(dht_dpin,HIGH);
}

void ReadDHT(){
bGlobalErr=0;
byte dht_in;
byte i;
digitalWrite(dht_dpin,LOW);
delay(20);

digitalWrite(dht_dpin,HIGH);
delayMicroseconds(40);
pinMode(dht_dpin,INPUT);
//delayMicroseconds(40);
dht_in=digitalRead(dht_dpin);

if(dht_in){
   bGlobalErr=1;
   return;
   }
delayMicroseconds(80);
dht_in=digitalRead(dht_dpin);

if(!dht_in){
   bGlobalErr=2;
   return;
   }
delayMicroseconds(80);
for (i=0; i<5; i++)
   dht_dat[i] = read_dht_dat();
pinMode(dht_dpin,OUTPUT);
digitalWrite(dht_dpin,HIGH);
byte dht_check_sum =
       dht_dat[0]+dht_dat[1]+dht_dat[2]+dht_dat[3];
if(dht_dat[4]!= dht_check_sum)
   {bGlobalErr=3;}
};

byte read_dht_dat(){
  byte i = 0;
  byte result=0;
  for(i=0; i< 8; i++){
      while(digitalRead(dht_dpin)==LOW);
      delayMicroseconds(30);
      if (digitalRead(dht_dpin)==HIGH)
     result |=(1<<(7-i));
    while (digitalRead(dht_dpin)==HIGH);
    }
  return result;
}

float get_mq5_r0() {
   float sensor_volt;
   float RS_air; //  Get the value of RS via in a clear air
   float R0;  // Get the value of R0 via in H2
   float sensorValue;
   sensorValue = 0.0;
   for(int x = 0 ; x < 100 ; x++)
   {
     sensorValue = sensorValue + analogRead(mq5_apin);
   }
   sensorValue = sensorValue/100.0;
   sensor_volt=(float)sensorValue/1024*5.0;
   RS_air = (5.0-sensor_volt)/sensor_volt; // omit *RL
   R0 = RS_air/6.5; // The ratio of RS/R0 is 6.5 in a clear air from Graph (Found using WebPlotDigitizer)
   return R0;
}

float get_mq5_ratio(float R0) {
    float sensor_volt;
    float RS_gas; // Get value of RS in a GAS
    float ratio; // Get ratio RS_GAS/RS_air
    int sensorValue = analogRead(mq5_apin);
    sensor_volt=(float)sensorValue/1024*5.0;
    RS_gas = (5.0-sensor_volt)/sensor_volt; // omit *RL
    ratio = RS_gas/R0;  // ratio = RS/R0
    return ratio;
}

float get_fire(void) {
  return(analogRead(fire_pin));  
}

void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  display.print  ("Sens:"); display.println(sensor.name);
  display.print  ("Ver :"); display.println(sensor.version);
  display.print  ("ID  :"); display.println(sensor.sensor_id);
  display.print  ("Max :"); display.print(sensor.max_value); display.println(" hPa");
  display.print  ("Min :"); display.print(sensor.min_value); display.println(" hPa");
  display.print  ("Res :"); display.print(sensor.resolution); display.println(" hPa");  
  display.display();
  delay(1000);
}

void get_barometer(void) {
  sensors_event_t event;
  bmp.getEvent(&event);
 
  /* Display the results (barometric pressure is measure in hPa) */
  if (event.pressure)
  {
    /* Display atmospheric pressue in hPa */
    display.print(event.pressure);
    display.print(" hPa/");
    
    /* Calculating altitude with reasonable accuracy requires pressure    *
     * sea level pressure for your position at the moment the data is     *
     * converted, as well as the ambient temperature in degress           *
     * celcius.  If you don't have these values, a 'generic' value of     *
     * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
     * in sensors.h), but this isn't ideal and will give variable         *
     * results from one day to the next.                                  *
     *                                                                    *
     * You can usually find the current SLP value by looking at weather   *
     * websites or from environmental information centers near any major  *
     * airport.                                                           *
     *                                                                    *
     * For example, for Paris, France you can check the current mean      *
     * pressure and sea level at: http://bit.ly/16Au8ol                   */
    
    /* Then convert the atmospheric pressure, and SLP to altitude         */
    /* Update this next line with the current SLP for better results      */
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    display.print(bmp.pressureToAltitude(seaLevelPressure,
                                        event.pressure)); 
    display.println("m");
    display.println("");
  }
}

float get_pressure(void) {
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure)
  {
    return(event.pressure);
  }
  else {
    return(-1);
  }
}

void buzz_alert(void) {
  {
    unsigned char i;
    for(i=0;i<80;i++)
    {
      digitalWrite(buzz_pin,HIGH);
      delay(1);
      digitalWrite(buzz_pin,LOW);
      delay(1);
    }
    for(i=0;i<100;i++)
    {
      digitalWrite(buzz_pin,HIGH);
      delay(2);
      digitalWrite(buzz_pin,LOW);
      delay(2);
    }
  }
}
  


