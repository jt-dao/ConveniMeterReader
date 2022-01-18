#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
#include "base64.hpp"
#include <MKRGSM.h>
#include "arduino_secrets.h" 
GSM gsmAccess;
GSM_SMS sms;
int ledPin1=6;  
int ledPinstop=1;  
int val1 = 0; 


#define BMPIMAGEOFFSET 66
const char bmp_header[BMPIMAGEOFFSET] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};
const char PINNUMBER[] = SECRET_PINNUMBER;

// set pin 7 as the slave select for the digital pot:
const int CS = 7;
bool is_header = false;
int mode = 0;
uint8_t start_capture = 0;
 ArduCAM myCAM( OV5642, CS );
uint8_t read_fifo_burst(ArduCAM myCAM);
void setup() {
// put your setup code here, to run once:
uint8_t vid, pid;
uint8_t temp;
pinMode(ledPin1, OUTPUT);
pinMode(ledPinstop, OUTPUT);
digitalWrite(ledPin1, LOW);
digitalWrite(ledPinstop, LOW);
  Wire.begin();
  Serial.begin(921600);
  // connection state
  bool connected = false;

  // Start GSM shield
  // If your SIM has PIN, pass it as a parameter of begin() in quotes
  while (!connected) {
    if (gsmAccess.begin(PINNUMBER) == GSM_READY) {
      connected = true;
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }
  Serial.println("GSM initialized, camera starting");
// set the CS as an output:
pinMode(CS, OUTPUT);
digitalWrite(CS, HIGH);
// initialize SPI:
SPI.begin();
 //Reset the CPLD
myCAM.write_reg(0x07, 0x80);
delay(100);
myCAM.write_reg(0x07, 0x00);
delay(100); 
while(1){
  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println(F("ACK CMD SPI interface Error! END"));
    delay(1000);continue;
  }else{
    Serial.println(F("ACK CMD SPI interface OK. END"));break;
  }
}
  while(1){
    //Check if the camera module type is OV5642
    myCAM.wrSensorReg16_8(0xff, 0x01);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if((vid != 0x56) || (pid != 0x42)){
      Serial.println(F("ACK CMD Can't find OV5642 module! END"));
      delay(1000);continue;
    }
    else{
      Serial.println(F("ACK CMD OV5642 detected. END"));break;
    } 
  }
//Change to JPEG capture mode and initialize the OV5642 module
myCAM.set_format(JPEG);
myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
myCAM.InitCAM();
delay(1000);
myCAM.clear_fifo_flag();
myCAM.write_reg(ARDUCHIP_FRAMES,0x00);

}

//----------------------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
// put your main code here, to run repeatedly:
uint8_t temp = 0xff, temp_last = 0;
bool is_header = false;
    myCAM.OV5642_set_JPEG_size(OV5642_320x240);delay(1000);
    myCAM.set_format(BMP);
    myCAM.InitCAM();     
    myCAM.clear_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    myCAM.wrSensorReg16_8(0x3818, 0x81);
    myCAM.wrSensorReg16_8(0x3621, 0xA7);

    //Flush the FIFO
    myCAM.flush_fifo();
    myCAM.clear_fifo_flag();
    //Start capture
    digitalWrite(ledPin1, HIGH);
    delay(100); //delays for 5 milliseconds
    myCAM.start_capture();
    start_capture = 0;
    while (!(myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)))
    {
      delay(100);
    }
    Serial.println(F("ACK CMD CAM Capture Done. END"));delay(50);
    digitalWrite(ledPin1, LOW);
    uint32_t length = 0;
    length = myCAM.read_fifo_length();
    if (length >= MAX_FIFO_SIZE ) 
    {
      Serial.println(F("ACK CMD Over size. END"));
      myCAM.clear_fifo_flag();
      return;
    }
    if (length == 0 ) //0 kb
    {
      Serial.println(F("ACK CMD Size is 0. END"));
      myCAM.clear_fifo_flag();
      return;
    }
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();//Set fifo burst mode

    //Serial.print(0xFF, HEX);
    //Serial.print(0xAA, HEX);
    for (temp = 0; temp < BMPIMAGEOFFSET; temp++)
    {
      //Serial.print(pgm_read_byte(&bmp_header[temp]), HEX);
    }
    //SPI.transfer(0x00);
    char VH, VL, rrr, ggg1, ggg2, ggg, bbb, grscale, chartemp, charmask;
    int i = 0, j = 0, bitcounter = 0, arrayindex = 0;
    unsigned char base64[12800];
    unsigned char chararray[9600];
    for (i = 0; i < 240; i++)
    {
      for (j = 0; j < 320; j++)
      {
        VH = SPI.transfer(0x00);;
        VL = SPI.transfer(0x00);;
        rrr = VH & 248;
        rrr = rrr >> 3;
        ggg1 = VH & 7;
        ggg1 = ggg1 << 3;
        ggg2 = VL & 224;
        ggg2 = ggg2 >> 5;
        ggg = ggg1 + ggg2;
        bbb = VL & 31;
        grscale = (rrr/3) + (ggg/3) + (bbb/3);
        if (grscale > 20)
          {
            VH = 255;
            VL = 255;
            charmask = 1;
          }
          else
          {
            VH = 0;
            VL = 0;
            charmask = 0;
          }
          if (j>120 && j<310 && i > 77 && i < 109)
          {
            Serial.print(VH>>7);
            chartemp = chartemp << 1;
            chartemp = chartemp + charmask;
            bitcounter = bitcounter + 1;
            if (bitcounter%8 == 0)
            {
              chararray[arrayindex] = chartemp;
              arrayindex = arrayindex + 1;
            }
           }
        //Serial.print(VL, HEX);
        delayMicroseconds(12);
        //Serial.print(VH, HEX);
        delayMicroseconds(12);
      }
    }
    Serial.println("");
   // Serial.print(0xBB, HEX);
    //Serial.print(0xCC, HEX);
    myCAM.CS_HIGH();
    if (bitcounter%8 != 0)
            {
              chararray[arrayindex] = chartemp;
              arrayindex = arrayindex + 1;
            }
    chararray[arrayindex] = '\0';
    //Clear the capture done flag
    myCAM.clear_fifo_flag();
    
 //   encode_base64(chararray, arrayindex, base64);
 //smsloop
 int smsloop;
   for (smsloop = 0; smsloop < arrayindex; smsloop = smsloop + 120)
   {
    encode_base64(&chararray[smsloop], 120, base64);
    Serial.println((String)"CMD Array printing now");
    Serial.println((char *)base64);
  // use for soracom 901011
  //char remoteNum[20] = "901011";
  //use for tracfone
  char remoteNum[20] = "4087188689";
  Serial.println(remoteNum);

  // sms text
  Serial.print("Now, enter SMS content: ");
 // String txtMsg(base64);
  //txtMsg = ((String)base64);

  Serial.println("SENDING");
  Serial.println();
  Serial.println("Message:");
  // send the message *UNCOMMENT TO SEND SMS*--------------------------------------------
  sms.beginSMS(remoteNum);
  sms.print((char*)base64);
  sms.endSMS();
  Serial.println("\nCOMPLETE!\n"); 
  }
  digitalWrite(ledPinstop, HIGH);
  delay(100);
  digitalWrite(ledPinstop, LOW);
  while (1)
  {
 
  }
}
uint8_t read_fifo_burst(ArduCAM myCAM)
{
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  length = myCAM.read_fifo_length();
  Serial.println(length, DEC);
  if (length >= MAX_FIFO_SIZE) //512 kb
  {
    Serial.println(F("ACK CMD Over size. END"));
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("ACK CMD Size is 0. END"));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  temp =  SPI.transfer(0x00);
  length --;
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    if (is_header == true)
    {
      //Serial.print(temp, HEX);
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      Serial.println(F("ACK IMG END"));
      //Serial.print(temp_last, HEX);
      //Serial.print(temp, HEX);
    }
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    break;
    delayMicroseconds(15);
  }
  myCAM.CS_HIGH();
  is_header = false;
  return 1;
}
