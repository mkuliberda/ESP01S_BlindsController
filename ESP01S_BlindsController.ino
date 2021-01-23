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
#define STASSID "Vectra-WiFi-84C84C"
#define STAPSK  "e63n6pb96ido2df7"
#endif

const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password

#define CMD_DELAY 30
byte Relay1_On[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xa0, 0x01, 0x01, 0xa2};
byte Relay1_Off[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xa0, 0x01, 0x00, 0xa1};
byte Relay2_On[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xA0, 0x02, 0x01, 0xA3};
byte Relay2_Off[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xA0, 0x02, 0x00, 0xA2};
byte Relay3_On[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xA0, 0x03, 0x01, 0xA4};
byte Relay3_Off[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xA0, 0x03, 0x00, 0xA3};
byte Relay4_On[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xA0, 0x04, 0x01, 0xA5};
byte Relay4_Off[] = {0x0d, 0x0a, 0x2b, 0x49, 0x50, 0x44, 0x2c, 0x30, 0x2c, 0x34, 0x3a, 0xA0, 0x04, 0x00, 0xA4};

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
WiFiServer server (80);

int cnt_ntp = 3600;
int cnt_ctrl = 10;
time_t calculated_time;
bool mode_auto = true;
int value = LOW;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("OK");
  delay(2000);
  Serial.println("WIFI CONNECTED WIFI GOT IP AT+CIPMUX=1 AT+CIPSERVER=1,8080 AT+CIPSTO=360");

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  delay(30000);
  Serial.println("0,CONNECTED");
  delay(1000);

  
  udp.begin(localPort);
  server.begin();
  delay(50);
  //Serial.println(WiFi.localIP());
}

void loop() { 
  
  cnt_ntp++;
  cnt_ctrl++;
  calculated_time+=1;

  if (cnt_ntp > 3600){
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);
  
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(500);  //TODO: check if this is enough or 1000 is better
  
    int cb = udp.parsePacket();
    if (!cb) {
      //Serial.println("$Error:1");
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
      //strftime(buf, sizeof(buf), "$%y-%m-%d,%a,%H-%M-%S", &ts); //TODO: remove for release
      //Serial.println(buf);//TODO: remove for release
      cnt_ntp = 0;
    }
  }

  handleServerRequests();

  if(cnt_ctrl > 10 && mode_auto == true){
    controlBlinds(calculated_time);
    cnt_ctrl = 0;
  }

  delay(1000);
  
}

void Blind1Up(void){
  Serial.write(Relay1_Off, sizeof(Relay1_Off));
  delay(CMD_DELAY);
  Serial.write(Relay2_On, sizeof(Relay2_On));
  delay(CMD_DELAY);
}

void Blind1Down(void){
  Serial.write(Relay2_Off, sizeof(Relay2_Off));
  delay(CMD_DELAY);
  Serial.write(Relay1_On, sizeof(Relay1_On));
  delay(CMD_DELAY);
}

void Blind2Up(void){
  Serial.write(Relay3_Off, sizeof(Relay3_Off));
  delay(CMD_DELAY);
  Serial.write(Relay4_On, sizeof(Relay4_On));
  delay(CMD_DELAY);
}

void Blind2Down(void){
  Serial.write(Relay4_Off, sizeof(Relay4_Off));
  delay(CMD_DELAY);
  Serial.write(Relay3_On, sizeof(Relay3_On));
  delay(CMD_DELAY);
}

void controlBlinds(time_t calc_time){
  struct tm ts = *localtime(&calc_time);
  ts.tm_isdst = 1;

  switch (ts.tm_mon){
    case 0:
    if (ts.tm_hour > 7 && ts.tm_hour < 16){
      Blind1Up();
      Blind2Up();
      value = HIGH;
      }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 1:
    if (ts.tm_hour > 7 && ts.tm_hour < 17){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 2:
    if (ts.tm_hour > 7 && ts.tm_hour < 18){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 3:
    if (ts.tm_hour > 7 && ts.tm_hour < 19){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 4:
    if (ts.tm_hour > 7 && ts.tm_hour < 20){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 5:
    if (ts.tm_hour > 7 && ts.tm_hour < 21){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 6:
    if (ts.tm_hour > 7 && ts.tm_hour < 21){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 7:
    if (ts.tm_hour > 7 && ts.tm_hour < 21){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 8:
    if (ts.tm_hour > 7 && ts.tm_hour < 20){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 9:
    if (ts.tm_hour > 7 && ts.tm_hour < 19){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    case 10:
    if (ts.tm_hour > 7 && ts.tm_hour < 17){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
      }
    break;
    case 11:
    if (ts.tm_hour > 7 && ts.tm_hour < 16){
      Blind1Up();
      Blind2Up();
      value = HIGH;
    }
    else{
      Blind1Down();
      Blind2Down();
      value = LOW;
    }
    break;
    default:
    break;
    }
}

void handleServerRequests(void){

  int timeout = 0;
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if ( !client ) {
    return;
  }
  
  // Wait until the client sends some data or timeout
  while ( !client.available() ){
    if(timeout++ > 500){
      return;
      }
    delay (10);
  }
    
  // Read the first line of the request
  String request = client.readStringUntil ('\r');
  client.flush ();

  value = LOW;
  if (request.indexOf("/BLINDS=UP") != -1) {
    mode_auto = false;
    Blind1Up();
    Blind2Up();
    value = HIGH;
  }
  if (request.indexOf("/BLINDS=DOWN") != -1){
    mode_auto = false;
    Blind1Down();
    Blind2Down();
  }
  if (request.indexOf("/BLINDS=AUTO") != -1){
    mode_auto = true;
  }
    
    // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
   
  client.print("<h1>Blinds are now: ");
  if(value == HIGH) {
    client.print("up");  
  } else {
    client.print("down");
  }
  client.println("<br>");
  client.print("Mode is now: ");
  if(mode_auto == true) {
    client.print("automatic");  
  } else {
    client.print("manual");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/BLINDS=UP\">here</a> to lift blinds<br>");
  client.println("Click <a href=\"/BLINDS=DOWN\">here</a> to lower blinds<br>");
  client.println("Click <a href=\"/BLINDS=AUTO\">here</a> set mode to automatic based on time</h1><br>");
  client.println("</html>");

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
