/**
   @file Hologram_Tcp.ino
   @author rakwireless.com
   @brief BG77 tcp test with Hologram, send gps data to server
   @version 0.1
   @date 2020-12-28
   @copyright Copyright (c) 2020
**/
#include <Adafruit_TinyUSB.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//IMU libraries
#include "SparkFunLIS3DH.h" //http://librarymanager/All#SparkFun-LIS3DH
#include <Wire.h>

#define BG77_POWER_KEY WB_IO1
#define BG77_GPS_ENABLE WB_IO2

String bg77_rsp = "";
String gps_data = "";
bool hasSent = false;

float x,y,z = 0.00;


LIS3DH SensorTwo(I2C_MODE, 0x18);
void lis3dh_read_data()
{
  // read the sensor value
  uint8_t cnt = 0;

  Serial.print(" X(g) = ");
  Serial.println(SensorTwo.readFloatAccelX(), 4);
  Serial.print(" Y(g) = ");
  Serial.println(SensorTwo.readFloatAccelY(), 4);
  Serial.print(" Z(g)= ");
  Serial.println(SensorTwo.readFloatAccelZ(), 4);
}


void setup()
{
  time_t serial_timeout = millis();
  Serial.begin(115200);
  
  while (!Serial)
  {
    if ((millis() - serial_timeout) < 5000)
    {
        delay(100);
    }
    else
    {
        break;
    }
  }


  if (SensorTwo.begin() != 0)
  {
    Serial.println("Problem starting the sensor at 0x18.");
  }
  else
  {
    Serial.println("Sensor at 0x18 started.");
  }
  Serial.println("enter !");

  
  Serial.println("RAK11200 Cellular TEST With Hologram sim card!");


 
}

void lis3dh_write_data(float &x, float &y, float &z)
{
  // read the sensor value
  uint8_t cnt = 0;
  x = SensorTwo.readFloatAccelX();
  y = SensorTwo.readFloatAccelY();
  z = SensorTwo.readFloatAccelZ();
}

void setup_bg77(){
    //BG77 init , Check if the modem is already awake
  time_t timeout = millis();
  bool moduleSleeps = true;
   Serial1.begin(115200);
  delay(1000);
  pinMode(BG77_GPS_ENABLE, OUTPUT);
  digitalWrite(BG77_GPS_ENABLE, 1);
  Serial1.println("ATI");
  //BG77 init
  while ((millis() - timeout) < 6000)
  {
    if (Serial1.available())
    {
      String result = Serial1.readString();
      Serial.println("Modem response after start:");
      Serial.println(result);
      moduleSleeps = false;
    }
  }
  if (moduleSleeps)
  {
    // Module slept, wake it up
    pinMode(BG77_POWER_KEY, OUTPUT);
    digitalWrite(BG77_POWER_KEY, 0);
    delay(1000);
    digitalWrite(BG77_POWER_KEY, 1);
    delay(2000);
    digitalWrite(BG77_POWER_KEY, 0);
    delay(1000);
  }
  Serial.println("BG77 power up!");
  bg77_at("AT+QGMR", 2000);
  //active and join to the net, this part may depend on some information of your operator.
  bg77_at("AT+CFUN=1,0", 500);
  delay(2000);
  bg77_at("AT+CPIN?", 500);
  delay(2000);
  bg77_at("AT+QNWINFO", 500);
  delay(2000);
  bg77_at("AT+QCSQ", 500);
  delay(2000);
  bg77_at("AT+CSQ", 500);
  delay(2000);
  bg77_at("AT+QICSGP=1,1,\"hologram\",\"\",\"\",1", 2000);
  delay(2000);
  bg77_at("AT+QIACT=1", 3000);
  delay(2000);
  bg77_at("AT+QIDEACT=1", 3000);
  delay(4000);
  bg77_at("AT+QIACT=1", 3000);
  delay(2000);

  //open tcp link with Hologram server
  bg77_at("AT+QIOPEN=1,0,\"TCP\",\"cloudsocket.hologram.io\",9999,0,1", 5000);
  
  delay(2000);
  
}
void parse_gps() //not used yet, but will be.
{
   int index1 = gps_data.indexOf(',');

   if(strstr(gps_data.c_str(),"E") != NULL)
   {
      int index2 = gps_data.indexOf('E'); 
      gps_data = gps_data.substring(index1+1,index2+1); 
   }
   if(strstr(gps_data.c_str(),"W") != NULL)
   {
      int index3 = gps_data.indexOf('W'); 
      gps_data = gps_data.substring(index1+1,index3+1);       
   }   
   
}


void get_gps() //not used yet, but will be.
{
  int gps_count = 300;
  int timeout = 1000;
  while(gps_count--)
  {
    Serial1.write("AT+QGPSLOC?\r");
    timeout = 1000;
    while (timeout--)
    {
      if (Serial1.available())
      {
         gps_data += char(Serial1.read());
      }
      delay(1);
    }
    if(strstr(gps_data.c_str(),"CME ERROR") != NULL)
    {
      gps_data = "";
      continue;
    }      
    if(strstr(gps_data.c_str(),"E") != NULL || strstr(gps_data.c_str(),"W") != NULL)
    {
      Serial.println(gps_data);
      parse_gps();
      break;
    }      
  }
}


//this function is suitable for most AT commands of bg96. e.g. bg96_at("ATI")
void bg77_at(char *at, uint16_t timeout)
{
  char tmp[256] = {0};
  int len = strlen(at);
  strncpy(tmp, at, len);
  uint16_t t = timeout;
  tmp[len] = '\r';
  Serial1.write(tmp);
  delay(10);
  while (t--)
  {
    if (Serial1.available())
    {
      bg77_rsp += char(Serial1.read());
    }
    delay(1);
  }
  Serial.println(bg77_rsp);
  bg77_rsp = "";
}


void send_HTTP_POST(char *URL, char *message){
//can 100% be optimized, especially regarding these 4 char arrays. Look at how sample functions pass data to the bg77. Time delays may also likely be decreased.
  char urlAT[256] = "";
  char postAT[256] = "";
  char urlIN[100] = "";
  char msgIN[100] =  "";
  sprintf(urlIN, "%s", URL);
  sprintf(msgIN, "%s", message);
  sprintf(urlAT, "AT+QHTTPURL=%d,80", strlen(URL));
  sprintf(postAT, "AT+QHTTPPOST=%d,80,80", strlen(message));

    bg77_at(urlAT, 2000);
    delay(3000);
    
    bg77_at(urlIN, 2000);
    delay(3000);
    
    bg77_at(postAT, 2000);
    delay(3000);
    
    bg77_at(msgIN, 2000);    
    delay(3000);
    
    bg77_at("AT+QHTTPREAD=200", 2000);
    delay(3000);
  
  }
  void send_HTTP_PUT(char *URL, char *message){
//Only works with firmware version BG77LAR02A04_01.008.01.008 or newer. use AT+QGMR to find firmware version and use Qflash to update.
//can 100% be optimized, especially regarding these 4 char arrays. Look at how sample functions pass data to the bg77. Time delays may also likely be decreased.
  char urlAT[256] = "";
  char postAT[256] = "";
  char urlIN[100] = "";
  char msgIN[100] =  "";
  sprintf(urlIN, "%s", URL);
  sprintf(msgIN, "%s", message);
  sprintf(urlAT, "AT+QHTTPURL=%d,80", strlen(URL));
  sprintf(postAT, "AT+QHTTPPUT=%d,80,80", strlen(message));

    bg77_at(urlAT, 2000);
    delay(3000);
    
    bg77_at(urlIN, 2000);
    delay(3000);
    
    bg77_at(postAT, 2000);
    delay(3000);
    
    bg77_at(msgIN, 2000);
    delay(3000);
    
    //bg77_at("AT+QHTTPREAD=200", 2000);
    delay(3000);
  
  }


void powerdown_bg77(){
        delay(5000);
    bg77_at("AT+QPOWD",2000);
    delay(5000);
    Serial1.end();
}
void loop()
{
  
//  if(!hasSent){
//    Serial.println("Send test data to Firebase via HTTP PUT.");
//    //bg77_at("AT+QGMR", 2000);  //gets firmware version.
//    
//    //Uncomment this bad boy \/ \/ \/ to send an http post
//    send_HTTP_PUT("https://wrecks-24e46-default-rtdb.firebaseio.com/posts.json", "{\"Message\":\"puttest2\"}");
//    hasSent = true;
//
//
//  }
//  else{
//    Serial.println("Message already sent! restart program to send another. (we only have 1MB of data, don't want to burn through it all by leaving this running)");
//  }
    lis3dh_write_data(x, y, z);
  //Serial.printf("%f %f %f\n", x,y,z);
  if(sqrt(sq(x) + sq(y) + sq(z)) > 1.5){
    Serial.printf("Oh no! Our Table! It's Broken!\n");
    setup_bg77();
    send_HTTP_PUT("https://wrecks-24e46-default-rtdb.firebaseio.com/posts.json", "{\"Message\":\"puttest2\"}");
    powerdown_bg77();
  }


  delay(100);

  
}
