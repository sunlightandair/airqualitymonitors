/* Clean Air Council Air Quality Monitor

Clean Air Council Air Quality Monitor incorporates modified code from AirCasting, Copyright (C) 2012-2015 Michael Heimbinder (info@habitatmap.org)
More information about AirCasting can be found at <http://aircasting.org/> and the full source code is available at <https://github.com/HabitatMap/AirCastingAndroidClient>
AirCasting is distributed under the terms of the GNU GPL, version 3 


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

******

Each sensor reading should be written as one line to the serial output. Lines should end
with '\n' and should have the following format:
<Measurement value>;<Sensor package name>;<Sensor name>;<Type of measurement>;<Short type of measurement>;<Unit name>;<Unit symbol/abbreviation>;<T1>;<T2>;<T3>;<T4>;<T5>
The Sensor name should be different for each sensor.
T1..T5 are integer thresholds which guide how values should be displayed -
- lower than T1 - extremely low / won't be displayed
- between T1 and T2 - low / green
- between T2 and T3 - medium / yellow
- between T3 and T4 - high / orange
- between T4 and T5 - very high / red
- higher than T5 - extremely high / won't be displayed
*/

#include <SoftwareSerial.h> //Header for software serial communication


//**************************************
//User Defined Variables for Calibration
//All Times in milliseconds

#define calibrating  0 //Are you trying to calibrate the sensor? Bool value, 0 if no 1 if yes
#define caliSamples  50 //How many samples do you want to take for one reading?
#define caliInterval  5 //How long between each sample?

float methaneRo= 1.1 , HCHORo=1, HSulfideRo= 5.7; //calibrated values (user defined from zero air calibration)

//User Defined Variables for Normal Operation

#define readSamples  50 //How many samples do you want to take for one reading?
#define readInterval  5 //How long between each sample?

//Hardware Related Variables
#define sensorNumber  3 //How many metal oxide sensors are there? (Shinyei isn't one!)
#define HSulfidePin A1
#define MethanePin A3
#define PMpin 8
#define HCHOPin A0

#include "DHT.h" //grove temp library
#define DHTPIN A2 //what pin temp monitor is connected to
#define DHTTYPE DHT11 //grove temperature and humidity sensor
DHT dht(DHTPIN, DHTTYPE);
//**************************************

SoftwareSerial mySerial(6, 7); //Assign 2 as Rx and 3 as Tx (Be careful changing these)

float maxv, methane, methaneScale, HCHO, HCHOScale, HSulfide, HSulfideScale, ratio=0, concentration = 0, PM = 0, methaneV, HCHOV, HSulfideV, methaneRs, HCHORs, HSulfideRs;
int humi, kelv, cel, fah, circ = 5, heat = 6;
unsigned long duration, startTime, sampleTime_ms = 30000, lowPulseOccupancy = 0;
char buf[12];

void setup(){
  
  Serial.begin(115200); //Serial communication for Arduino Serial Monitor
  mySerial.begin(115200); //Serial communcation for Aircasting Application
  pinMode(circ, OUTPUT);
  pinMode(heat, OUTPUT);
  
  /* All #S|LOGFILE|# commands log serial output to a data file using Gobetwino <http://www.mikmo.dk/gobetwinodownload.html>
   This data file is used for calibration and debugging, timestamping readings, and saves you from having to copy and paste serial monitor data into excel.
   Make sure to set the correct baud rate (115200) and set the correct COM port in Gobetwino settings
   Make sure when using Gobetwino create a command called LOGFILE (select the lgfil command type). 
   Make sure to change which file this command outputs to after each run. 
  */
  
  
  
  if(calibrating){
    Serial.print("#S|LOGFILE|[");
    Serial.print("***************************************************************************************");
    Serial.println("]#");
    Serial.print("#S|LOGFILE|[");
    Serial.print("Calibration Session Begins.");
    Serial.println("]#");
    Serial.print("#S|LOGFILE|[");
    Serial.print("Waiting for sensors to warm up.");    
    Serial.println("]#");
    delay(30000); //wait a few minutes (180000 ms) for everything to warm up
    Serial.print("#S|LOGFILE|[");
    Serial.print("Sensors are warm, ready to calibrate.");
    Serial.println("]#");
    
    //Scale Tempereature
    
    GetTempHum();
    Serial.print("#S|LOGFILE|[");
    Serial.print("The Temperature in Celcius is:");
    Serial.print(itoa(cel, buf, 10));
    Serial.println("]#");
    Serial.print("#S|LOGFILE|[");
    Serial.print("The Humidity % is:");
    Serial.print(itoa(humi, buf, 10));
    Serial.println("]#");
    methaneScale = ScaleSensor(cel,humi,MethanePin); 
    Serial.print("#S|LOGFILE|[");
    Serial.print("The scaling factor for Methane is:");
    Serial.print(itoa(methaneScale, buf, 10));
    Serial.println("]#");
  //HCHOScale = ScaleSensor(cel,humi,MethanePin);
  //HSulfideScale= ScaleSensor(cel,humi,MethanePin);
  
    //Calibrate Sensors
  
    int sensor = 1;
    
    for(sensor=1;sensor<=sensorNumber;sensor++){
      calibrate(sensor);    
  } 
  Serial.print("#S|LOGFILE|[");
  Serial.print("All sensors are now calibrated.");
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("***************************************************************************************");
  Serial.println("]#");
  
} else {
  Serial.print("#S|LOGFILE|[");
  Serial.print("***************************************************************************************");
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("Pre-Calibrated Session Begins.");
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("Make sure you have calibrated these values in zero air before using this air monitor.");
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("Calibrated Ro Values:");
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("Methane: ");
  Serial.print(itoa(methaneRo, buf, 10));
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("Hydrogen Sulfide: ");
  Serial.print(itoa(HSulfideRo, buf, 10));
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("HCHO: ");
  Serial.print(itoa(HCHORo, buf, 10));
  Serial.println("]#");
  Serial.print("#S|LOGFILE|[");
  Serial.print("***************************************************************************************");
  Serial.println("]#");
  }
}
  

void loop(){  
  
    
  
  //Find temperature and humidity
  GetTempHum();
  //Figure out temperature and humidity scaling
  methaneScale = ScaleSensor(cel,humi,MethanePin); 
  //HCHOScale = ScaleSensor(cel,humi,HCHOPin);
  //HSulfideScale= ScaleSensor(cel,humi,HSulfidePin);
  
  //Call up calculation functions
  GetMethane();
  GetHCHO();
  GetH2S();
  GetPM();
  
  //Display to AirCasting app and debug to file via USB and Gobetwino
  
  
  
  Serial.print("#S|LOGFILE|[");
  Serial.print("Temperature: ");
  // Serial.print(kelv);
  // Serial.print("K ");
  Serial.print(itoa(cel, buf, 10));
  Serial.print("C ,");
  Serial.print(itoa(fah,buf,10));
  Serial.print("F ");
  Serial.println("]#");  
  
  Serial.print("#S|LOGFILE|[");
  Serial.print("Humidity: ");
  Serial.print(itoa(humi, buf, 10));
  Serial.print("% ");
  Serial.println("]#");
   

  //Display of Methane gas sensor voltage and resistance
  Serial.print("#S|LOGFILE|[");
  Serial.print("Methane Gas: ");
  Serial.print(itoa(methane, buf, 10));
  Serial.print(" ppm ");
  Serial.print("Raw CH4 Sensor Voltage and Resistance: ");
  Serial.print(itoa(methaneV, buf, 10));
  Serial.print(", ");
  Serial.print(itoa(methaneRs, buf, 10));  
  Serial.print(", ");
  Serial.print("Temp/Hum Scaling: ");
  Serial.print(itoa(methaneScale, buf, 10));
  Serial.println("]#");
  
  /* 
  
  Scrapping HSulfide, not generally found around wells and stations in PA.
  May be useful for Western shale plays.
  
  Serial.print("#S|LOGFILE|[");
  Serial.print("Hydrogen Sulfide: ");
  Serial.print(itoa(HSulfide, buf, 10));
  Serial.print(" ppm ");
  Serial.print("Raw Hydrogen Sulfide Sensor Voltage and Resistance: ");
  Serial.print(itoa(HSulfideV, buf, 10));
  Serial.print(", ");
  Serial.print(itoa(HSulfideRs, buf, 10));  
  Serial.print(", ");
  Serial.println("]#");
  
  */
  
  Serial.print("#S|LOGFILE|[");
  Serial.print("Particulate Matter: ");
  Serial.print(itoa(concentration, buf, 10));
  Serial.print(" hppcf ,");
  Serial.print(itoa(PM, buf, 10));
  Serial.print(" ug/m^3 ");
  Serial.println("]#");
  
 
  
  //Display of temperature in K, C, and F
  /*
  mySerial.print(kelv);
  mySerial.print(";InsertSensorPackageName;TMP36;Temperature;K;kelvin;K;273;300;400;500;600");
  mySerial.print("\n");
  mySerial.print(cel);
  mySerial.print(";InsertSensorPackageName;TMP36;Temperature;C;degrees Celsius;C;0;10;15;20;25");
  mySerial.print("\n");
  */
  
  /*Display of humidity
  Commenting this so it does not display (too many sensors for app currently)
  mySerial.print(humi);
  mySerial.print(";groveTempandHum;DHT11;Humidity;RH;percent;%;0;25;50;75;100");
  mySerial.print("\n");
  */
  
  mySerial.print(fah);
  mySerial.print(";groveTempandHum;DHT11;Temperature;F;degrees Fahrenheit;F;0;30;60;90;120");
  mySerial.print("\n");
 
 if(calibrating){   
  mySerial.print(methaneV);
  mySerial.print(";GroveMQ9 V;MQ9 V;CH4 Gas Volt;CH4 V;Sensor Voltage;V;0;1;2;3;5");
  mySerial.print("\n");
  
  mySerial.print(methaneRs); 
  mySerial.print(";GroveMQ9 KOhm;MQ9 KOhm;CH4 Gas KOhm;CH4 KOhm;Sensor Resistance;KOhm;0;2;5;10;20"); 
  mySerial.print("\n");
  
  /*
  mySerial.print(HSulfideV);
  mySerial.print(";SainsmartMQ136 V;MQ136 V;Hydrogen Sulfide Gas Volt;H2S V;Sensor Voltage;V;0;1;2;3;5");
  mySerial.print("\n");
  
  mySerial.print(HSulfideRs);  
  mySerial.print(";SainsmartMQ136 KOhm;MQ136 KOhm;Hydrogen Sulfide Gas KOhm;H2S;Sensor Resistance;kOhm;0;30;100;150;200");
  mySerial.print("\n");
  */ 
 } else {
   
  mySerial.print(methane);
  mySerial.print(";GroveMQ9;MQ9;CH4 Gas;CH4;ppm;ppm;0;500;1000;5000;9999");
  mySerial.print("\n");
  
  mySerial.print(HCHO);
  mySerial.print(";GroveHCHO;HCHO;Formaldehyde Gas;HCHO;ppm;ppm;0;500;1000;5000;9999");
  mySerial.print("\n");
  
 /* 
 `mySerial.print(HSulfide);
  mySerial.print(";SainsmartMQ136;MQ136;Hydrogen Sulfide Gas;H2S;Parts Per Million;ppm;0;10;25;50;100");
  mySerial.print("\n");
  */
  
  mySerial.print(PM);
  mySerial.print(";GroveDustSensor;Shinyei;Particulate Matter;PM;Micrograms Per Meter Cubed;ug/m^3;0;1;5;10;17");
  mySerial.print("\n");
  
  
  
 }
      
  //Display in app Hydrogen Sulfide Concentration, Voltage, and Resistance for Calibration
  
 
   
  
  
  
}

void GetTempHum(){
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  humi = h;
  cel = t;
  kelv = t + 273.15;
  fah = ((t * 9)/5.0) + 32; //convert celsius read to fahrenheit

}

  

void GetMethane(){ 
  
  //insert methane sensor code here
  //using the sensor plugged into A3
  //may require more current & RL adjustment
  
  float methaneRead, ratio;
  methaneRead = sensorRead(MethanePin); //average the multiple read values
  methaneV = calcVolts(methaneRead); //Analog read to Volts
  methaneRs = calcRs(methaneV);//Volts to R_s using the equation R_s = ((V_c-V_rl)/V_rl)* R_l where V_c=5 V, and R_l=10 Kohms and V_rl is the sensor voltage
  /*Commented out for Calibration
  
  methane = 4660*pow((methaneRs/4455.653),(-2.673));  //modified with specsheet curve, see SensorCurveApproximations.xlsx, you can also use a lookup table array for values on the spec sheet
 
  */
  ratio=methaneRs/methaneRo; //In ambient air, the Rs/Ro is about 10, the Rs is getting measurments of 11 so Ro is probably 1.1
  methane = 616.47*1/(pow((1*ratio),1.965)); //1 is a placeholder for methaneScale
  
/* About calibration: May be built into AirCasting app eventually, until then you can use the provided guide (will be a PDF, work in progress) and some zero air, ethane, or known CO to calibrate most sensors.
The calibration would modify the R_o value, which, in this case, is the "0.0723" in the denominator of the pow() function. This denominator was found in ambient air 
(again, see SensorCurveApproximations.xslx) and will need to be modified.
*/
}


void GetHCHO(){ 
  
  // insert formaldehyde sensor code here 
  // Using the sensor plugged into A0
  //may require more current & RL adjustment
  
  float HCHORead,ratio;
  HCHORead = sensorRead(HCHOPin);
  HCHOV = calcVolts(HCHORead);
  HCHORs = calcRs(HCHOV);
  /*Commented out for Calibration
  HCHO = 1.1507*pow((HCHORs/237.86),(-2.272));
  */
  ratio=HCHORs/HCHORo;
  HCHO = 0.2132*pow((ratio),(-2.212));
}

void GetH2S() {
 //insert H2S sensor code here
 //Using A1 (sensor and two breakout boards)
 //may require more current & RL adjustment
 
 float HSulfideRead, ratio;
 HSulfideRead = sensorRead(HSulfidePin);
 HSulfideV = calcVolts(HSulfideRead);
 HSulfideRs = calcRs(HSulfideV);
 /*Commented out for Calibration
 HSulfide = 42.129*pow((HsulfideRs/2737.75),(-3.061));
 */
 ratio=(HSulfideRs/HSulfideRo)*methaneScale; //in ambient air, Rs was around 20, Rs/Ro = 3.5 in ambient air, so Ro is about 5.7
 HSulfide = 945.68*pow(ratio,(-4.426));
 
}

void GetPM(){
    //using PM sensor plugged into pin 8 for digital input
    //adapted from <http://www.howmuchsnow.com/arduino/airquality/>
    //Chris Nafis (c) 2012
  duration = pulseIn(PMpin, LOW);
  lowPulseOccupancy = lowPulseOccupancy+duration;

  if ((millis()-startTime) > sampleTime_ms)
  {
    ratio = lowPulseOccupancy/(sampleTime_ms*10.0);  // Integer percentage 0=>100
    concentration = (1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62)/100; //Using spec sheet curve, cubic regression
    PM= concentration*3531.5*0.000000588; //converting 100pcs/cf to ug/m^3 (more info at <http://wireless.ece.drexel.edu/air_quality/project%20files/Data%20Validation.pdf>
    Serial.print("#S|LOGFILE|[");
    Serial.print("LPO:");
    Serial.print(lowPulseOccupancy);
    Serial.print(", ");
    Serial.print("Ratio:");    
    Serial.print(ratio);
    Serial.print(", ");
    Serial.println("]#");
    lowPulseOccupancy = 0;
    startTime = millis();         
    
}


}

void GetBenzene(){
 //insert Benzene code here
 //This will most likely be the same code as HCHO, but with a different sensor adjustment
 //This sensor will be the same HCHO sensor but with a different calibration
 //Will probably be scrapping this concept for total VOCs due to cross-sensitivity issues
}

float ScaleSensor(int temperature, float humidity, char pin){
  //Temperature and humidity curves based on 9 average curves generated from the MQ 9 Datasheet
  //For more on spec curves, see Sensor Curve Approximations.xlsx
  //Going to come up with a new surface when calibrating each sensor
  
  
 
   if((temperature<50 && temperature>-10) && (humidity<=85 && humidity>=33)){
  switch(pin){
    case MethanePin:
         int i;
         float j;
         j = map(humidity, 33, 85, 1, 9);
         i = round(j);
        switch (i){
          case 9: //humi<=85 && humi>82
            return .000000004*pow(cel,5)-.0000004*pow(cel,4)+.000004*pow(cel,3)+.0005*pow(cel,2)-0.0194*cel+1.1218;          
          break;
          case 8: //humi<=82 && humi>75
            return .000000004*pow(cel,5)-.0000004*pow(cel,4)+.000004*pow(cel,3)+.0004*pow(cel,2)-0.0186*cel+1.1945;
          break;
          case 7: //humi<=75 && humi>69
            return .000000004*pow(cel,5)-.0000003*pow(cel,4)+.000004*pow(cel,3)+.0004*pow(cel,2)-0.0177*cel+1.171;
          break;
          case 6: //humi<=69 && humi>62
            return .000000004*pow(cel,5)-.0000003*pow(cel,4)+.000004*pow(cel,3)+.0004*pow(cel,2)-0.0169*cel+1.1475;
          break;
          case 5: //humi<=62 && humi>56
            return .000000004*pow(cel,5)-.0000003*pow(cel,4)+.000003*pow(cel,3)+.0004*pow(cel,2)-0.016*cel+1.124;
          break;
          case 4: //humi<=56 && humi>49
            return .000000003*pow(cel,5)-.0000003*pow(cel,4)+.000004*pow(cel,3)+.0003*pow(cel,2)-0.052*cel+1.1004;
          break;
          case 3: //humi<=49 && humi>43
            return .000000003*pow(cel,5)-.0000003*pow(cel,4)+.000004*pow(cel,3)+.0003*pow(cel,2)-0.0143*cel+1.0769;
          break;
          case 2: //humi<=43 && humi>36
          return .000000003*pow(cel,5)-.0000003*pow(cel,4)+.000004*pow(cel,3)+.0002*pow(cel,2)-0.0135*cel+1.0534;
          break;
          case 1: //humi<=36 && humi>=33
          return .000000003*pow(cel,5)-.0000002*pow(cel,4)+.000004*pow(cel,3)+.0002*pow(cel,2)-0.0127*cel+1.0299;
          break;
          default:
            Serial.print("#S|LOGFILE|[");
            Serial.print("Something went wrong when mapping humidity.");
            Serial.println("]#");
            return 0;          
        }
    break;
    case HSulfidePin:
    return 0;
    Serial.print("#S|LOGFILE|[");
    Serial.print("No scaling yet for Hydrogen Sulfide");
    Serial.println("]#");
   break;
    case HCHOPin:
    return 0;
    Serial.print("#S|LOGFILE|[");
    Serial.print("No scaling yet for Formaldehyde");
    Serial.println("]#");
   break;
   default:
   Serial.print("#S|LOGFILE|[");
   Serial.print("No pin identified");
   Serial.println("]#");
   return 0;
    }
    
    } else {
        Serial.print("#S|LOGFILE|[");
        Serial.print("Out of temperature or humidity range");
        Serial.println("]#");
        return 0;
        
      }
   
}

void calibrate(int sensor){
  
  float sig, v, rs;
  
  switch (sensor){
  case 1: sig = sensorRead(MethanePin);
          v = calcVolts(sig);
          rs = calcRs(v);
          methaneRo= rs/10;
          Serial.print("#S|LOGFILE|[");
          Serial.print("The calibrated Methane Ro is: ");
          Serial.print(itoa(methaneRo, buf, 10));
          Serial.println("]#");
          break;          
  case 2: sig = sensorRead(HCHOPin);
          v = calcVolts(sig);
          rs = calcRs(v);
          HCHORo = rs;
          Serial.print("#S|LOGFILE|[");
          Serial.print("The calibrated HCHO Ro is ");
          Serial.print(itoa(HCHORo, buf, 10));
          Serial.println("]#");
          break;
  case 3: sig = sensorRead(HSulfidePin);
          v = calcVolts(sig);
          rs = calcRs(v);
          HSulfideRo = rs/3.5;
          Serial.print("#S|LOGFILE|[");
          Serial.print("The calibrated Hydrogen Sulfide Ro is ");
          Serial.print(itoa(HSulfideRo, buf, 10));
          Serial.println("]#");
          break;
}
}

float sensorRead(char pin){
  int i;
  float sample = 0;
  
  if (calibrating){
  for (i=0;i<caliSamples;i++) 
  {
     sample += analogRead(pin);
     delay(caliInterval);
  }
  sample = sample/caliSamples;
  
     
  } else{
    
    for(i=0;i<readSamples;i++){
      sample += analogRead(pin);
      delay(readInterval);
    }
    sample = sample/readSamples;
  }
  
  return sample;
}

float calcVolts(float sensorRead){
  return map(sensorRead, 0, 1023, 0, 5);
}

float calcRs(float volts) {
  return((5-volts)/volts)*10; //Volts to R_s using the equation R_s = ((V_c-V_rl)/V_rl)* R_l where V_c=5 V, and R_l=10 Kohms and V_rl is the sensor voltage
}

float map(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
