
# AJ-SR04M Transmition only


## Automatic Serial Mode
Use Resistance value 120KΩ, to enter the Automatic Serial port mode. The trigger signal is not used in this mode. In this mode, the distance calculation happen on the sensor and it outputs the distance directly over the Echo line every 120ms.

AJ-SR04m transmits bytes per measurement, which is shown below.

Byte1	Byte2	Byte3	Byte4
Start Byte	Upper Byte	Lower Byte	Checksum
Showing 1 to 1 of 1 entries
40KHZ pulse generated internally for every 120ms and gives the output distance in echo line. distance in (mm). The checksum is the output,  and it is the sum of the Upperbyte and LowerByte. Checksum is used to verify the packet loss during transmission.

#### Sample Code to test Automatic Serial Mode:

```
#include <SoftwareSerial.h>
#define rxPin 10
#define txPin 11
 
SoftwareSerial jsnSerial(rxPin, txPin);
 
void setup() {
  jsnSerial.begin(9600);
  Serial.begin(9600);
}
 
void loop() {
  /*jsnSerial.write(0x01);
   delay(10);*/
  if(jsnSerial.available()){
    getDistance();
   //
  
  }
  
}
void getDistance(){
  unsigned int distance;
  byte startByte, h_data, l_data, sum = 0;
  byte buf[3];
  
  startByte = (byte)jsnSerial.read();
  if(startByte ==255){
    jsnSerial.readBytes(buf, 3);
    h_data = buf[0];
    l_data = buf[1];
    sum = buf[2];
    distance = (h_data<<8) + l_data;
    if((( h_data + l_data)) != sum){
      Serial.println("Invalid result");
    }
    else{
      Serial.print("Distance [mm]: "); 
      Serial.println(distance);
      
    } 
  } 
  
  else return;
  delay(100);
}
```