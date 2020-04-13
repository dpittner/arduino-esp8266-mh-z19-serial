#define MH_Z19_RX D7
#define MH_Z19_TX D6

#include "MHZ19.h"       
#include <SoftwareSerial.h>

MHZ19 myMHZ19;        

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19

int readCo2()
{
    int ppm;

    ppm = myMHZ19.getCO2(false);                             // Request CO2 (as ppm)

    int8_t Temp;
    Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
    Serial.print("Temperature (C): ");                  
    Serial.println(Temp);            
    return ppm;

}

void setupCo2() {
  co2Serial.begin(9600,SWSERIAL_8N1, MH_Z19_RX, MH_Z19_TX); //Init sensor MH-Z19(14)
  myMHZ19.begin(co2Serial);                                // *Serial(Stream) refence must be passed to library begin(). 
  myMHZ19.autoCalibration(false);                              // Turn auto calibration ON (OFF autoCalibration(false))

  char myVersion[4];          
  myMHZ19.getVersion(myVersion);

  Serial.print("\nFirmware Version: ");
  for(byte i = 0; i < 4; i++)
  {
    Serial.print(myVersion[i]);
    if(i == 1)
      Serial.print(".");    
  }
   Serial.println("");
   Serial.print("Range: ");
   Serial.println(myMHZ19.getRange());   
   Serial.print("Background CO2: ");
   Serial.println(myMHZ19.getBackgroundCO2());
   Serial.print("Temperature Cal: ");
   Serial.println(myMHZ19.getTempAdjustment());
}
