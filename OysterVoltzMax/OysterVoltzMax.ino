//INA219 Voltage measurement
#include <Wire.h>
#include <Adafruit_INA219.h>

 //Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include <RTClib.h>

//serial peripheral interface and library for SD card reader
#include <SPI.h> //serial peripheral interface for SD card reader
#include <SD.h> //library for SD card reader

const int chipSelect = 4; //sets chip select pin for SD card reader
char datalogFileName[12]; //sets data logger file name 

Adafruit_INA219 ina219; //define INA219 chip

RTC_DS3231 rtc; //define real-time clock

void setup(void) {

  Serial.begin(9600);
    
  Serial.print("Initializing SD card...");

  while (!SD.begin(chipSelect)) { //halts program if no SD card detected

    Serial.println("Card failed, or not present");
    delay(1000);

  }

  Serial.println("card initialized.");

  delay(1000);

  if (! rtc.begin()) { //looks for RTC

  Serial.println("Couldn't find RTC");
  while (1);

  }

  // This function allows the sd-library to set the correct file created & modified dates for all sd card files.
  // (See the SDCardDateTimeCallback function defined at the end of this file)
  SdFile::dateTimeCallback(SDCardDateTimeCallback);
  get_numbered_filename(datalogFileName, "LOG", "CSV");

  Serial.print("Writing to datalog: ");
  Serial.println(datalogFileName);

  File dataFile = SD.open(datalogFileName, FILE_WRITE);

  if (dataFile) {
    Serial.println("====================================================");
    Serial.println("Date,Time,Bus Voltage,Shunt Voltage,Load Voltage,Current(mA),Power(mW)");
    dataFile.println("Date,Time,Bus Voltage,Shunt Voltage,Load Voltage,Current(mA),Power(mW)");
    dataFile.close();

  } else {
    Serial.println("Err: Can't open datalog!");
  }
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Couldn't find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();

  //Initialize real-time clock
  if (rtc.lostPower()) {

    //reset RTC with time when code was compiled if RTC loses power
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  }

  delay(250);   // Wait a quarter second to continue.

}

void loop(void) {

  DateTime now = rtc.now(); //check RTC
  char dateTimeString[40];
  get_date_time_string(dateTimeString, now);

  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  Serial.println("");

  delay(2000);

//output readings to data file.
  File dataFile = SD.open(datalogFileName, FILE_WRITE);
  if (dataFile) {

      dataFile.print(dateTimeString);
      dataFile.print(",");
      dataFile.print(busvoltage);
      dataFile.print(",");
      dataFile.print(shuntvoltage);      
      dataFile.print(",");
      dataFile.print(loadvoltage);
      dataFile.print(",");
      dataFile.print(current_mA);
      dataFile.print(",");
      dataFile.println(power_mW);  

      dataFile.close();          
      }  
}

void get_numbered_filename(char* outStr, const char* filePrefix, const char* fileExtension) {

  // Make base filename
  sprintf(outStr, "%s000.%s", filePrefix, fileExtension);
  int namelength = strlen(outStr);
  if (namelength > 12) Serial.println("Error: filename too long. Shorten your filename to < 5 characters (12 chars total w number & file extension) !");

  // Keep incrementing the number part of filename until we reach an unused filename
  int i = 1;
  while (SD.exists(outStr)) {  // keep looping if filename already exists on card. [If the filename doesn't exist, the loop exits, so we found our first unused filename!]

    int hundreds = i / 100;
    outStr[namelength - 7] = '0' + hundreds;
    outStr[namelength - 6] = '0' + (i / 10) - (hundreds * 10);
    outStr[namelength - 5] = '0' + i % 10;
    i++;

  }

} 

void get_date_time_string(char* outStr, DateTime date) {
  // outputs the date as a date time string,
  sprintf(outStr, "%02d/%02d/%02d,%02d:%02d:%02d", date.month(), date.day(), date.year(), date.hour(), date.minute(), date.second());
  // Note: If you would like the date & time to be seperate columns chabge the space in the formatting string to a comma - this works because the file type is CSV (Comma Seperated Values)
}

void SDCardDateTimeCallback(uint16_t* date, uint16_t* time) // This funny function allows the sd-library to set the correct file created & modified dates for all sd card files (As would show up in the file explorer on your computer)
{
  DateTime now = rtc.now();
  *date = FAT_DATE(now.year(), now.month(), now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
