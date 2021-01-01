/*

  Udp NTP Client

  Get the time from a Network Time Protocol (NTP) time server
  Demonstrates use of UDP sendPacket and ReceivePacket
  For more on NTP time servers and the messages needed to communicate with them,
  see http://en.wikipedia.org/wiki/Network_Time_Protocol

  created 4 Sep 2010
  by Michael Margolis
  modified 9 Apr 2012
  by Tom Igoe
  updated for the ESP8266 12 Apr 2015
  by Ivan Grokhotkov
  updated on 21 DEC 2020
  by Mateusz Kuliberda

  This code is in the public domain.

*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

#ifndef STASSID
#define STASSID "YOGAWIFI"
#define STAPSK  "123456789"
#endif

const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password


unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

int cnt = 360;
time_t calculated_time;

void setup() {
  Serial.begin(115200);
  Serial.println();

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  udp.begin(localPort);
}

void loop() {

  cnt++;
  calculated_time+=10;

  if (cnt > 360){ 
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);
  
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(500);  //TODO: check if this is enough or 1000 is better
  
    int cb = udp.parsePacket();
    if (!cb) {
      Serial.println("$Error:1");
    } else {
    
      // We've received a packet, read the data from it
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
  
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      // now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;

      time_t rawtime;
      rawtime = epoch + 3600;
      struct tm  ts;
      char       buf[80];

      // Format time, yy-mm-dd,ddd,hh-mm-ss 
      ts = *localtime(&rawtime);
      ts.tm_isdst = 1;
  
      calculated_time = rawtime;
      strftime(buf, sizeof(buf), "$%y-%m-%d,%a,%H-%M-%S", &ts); //TODO: remove for release
      Serial.println(buf);//TODO: remove for release
      cnt = 0;
    }
  }

  controlBlinds(calculated_time);
    // wait ten seconds before asking for the time again
  delay(10000);
  
}

void controlBlinds(time_t calc_time){
  struct tm ts = *localtime(&calc_time);
  ts.tm_isdst = 1;

  switch (ts.tm_mon){
    case 0:
    if (ts.tm_hour > 7 && ts.tm_hour < 16){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 1:
      if (ts.tm_hour > 7 && ts.tm_hour < 17){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 2:
      if (ts.tm_hour > 7 && ts.tm_hour < 18){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 3:
      if (ts.tm_hour > 7 && ts.tm_hour < 19){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 4:
      if (ts.tm_hour > 7 && ts.tm_hour < 20){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 5:
      if (ts.tm_hour > 7 && ts.tm_hour < 21){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 6:
      if (ts.tm_hour > 7 && ts.tm_hour < 21){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 7:
      if (ts.tm_hour > 7 && ts.tm_hour < 20){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 8:
      if (ts.tm_hour > 7 && ts.tm_hour < 19){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 9:
      if (ts.tm_hour > 7 && ts.tm_hour < 18){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 10:
      if (ts.tm_hour > 7 && ts.tm_hour < 17){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    case 11:
    if (ts.tm_hour > 7 && ts.tm_hour < 16){
      Serial.println("Blinds open");
      }
    else{
      Serial.println("Blinds closed");
      }
    break;
    default:
    break;
    }

}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
