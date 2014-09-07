#include <EtherCard.h>


// Define the pins
#define P1 4
#define N1 3
#define P2 2
#define N2 9

#define DEBUG 0

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
static byte myip[] = { 192,168,114,195 };

byte Ethernet::buffer[500];
BufferFiller bfill;


// Make string buffer for commands
const int buffersize = 32;
char cbuffer[buffersize];
int count = 0;
boolean go = false;
boolean controller_active = false;
int controller_target = 155;

void setup() {
  pinMode(P1, OUTPUT);
  pinMode(N1, OUTPUT);
  pinMode(P2, OUTPUT);
  pinMode(N2, OUTPUT);
  pinMode(3, INPUT);
  turn_off();
  // initialize serial:
  //Serial.begin(115200); 
  if (ether.begin(sizeof Ethernet::buffer, mymac, 8) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  ether.staticSetup(myip);
  Serial.println("Arduino up and running");
  Serial.println("Enter command please");
}

static word homePage(char* data, word len) {
  Serial.print("START, length:");
  Serial.println(len);
  for(int i=0; i<len; i++){
    Serial.print(data[i]);
  }
  Serial.println("END");
  long t = millis() / 1000;
  word h = t / 3600;
  byte m = (t / 60) % 60;
  byte s = t % 60;
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
  "HTTP/1.0 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Pragma: no-cache\r\n"
  "\r\n"
  "<title>RBBB server</title>" 
  "<h1>$D$D:$D$D:$D$D</h1>"),
    h/10, h%10, m/10, m%10, s/10, s%10);

  if(controller_active){
    bfill.emit_p(PSTR("Controller enabled."));
  }
  else{
    bfill.emit_p(PSTR("Controller not active"));
  }
  return bfill.position();
}

void loop() {  
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  if (pos){  // check if valid tcp data is received
    char* data = (char *) Ethernet::buffer + pos;
    //Serial.println(data[5]);
    if(data[5] == 'C'){
      controller_active = true;
      char cval[3];
      for(int i=0; i<3;i++){
        cval[i] = data[i+7];
      }
      controller_target = constrain(atoi(cval), 10, 300); 
      Serial.println("Processing controller command");
      Serial.println(controller_target);
    }
    ether.httpServerReply(homePage(data, len)); // send web page data
  }
  
  
  // if there's any serial available, read it as a string:
  while (Serial.available() > 0) {

    char c = Serial.read();
    if(c == '\n'){
      // end of command, quit loop
      go = true;
    }
    else{
      cbuffer[count] = c;
      count++;
    }
    if(count == buffersize){
      go = true;
    }
  }
    
  if(go){ // process command
    go = false;
    // do stuff with the string
    Serial.println(cbuffer);
    if(strcmp(cbuffer, "test") == 0){
      Serial.println("haha je zei test");
    }
    
    if(strcmp(cbuffer, "off") == 0){
      controller_active = false;
      turn_off();
    }
    
    if(cbuffer[0] == 'L'){
      controller_active = false;
      int val = get_val();
      go_left(val);
    }
    
    if(cbuffer[0] == 'R'){
      controller_active = false;
      int val = get_val();
      go_right(val);
    }    
    if(cbuffer[0] == 'C'){
      controller_active = true;
      char cval[3];
      for(int i=0; i<3;i++){
        cval[i] = cbuffer[i+1];
      }
      controller_target = constrain(atoi(cval), 10, 300); 
    }    
    
    // clear the string
    for(int i = 0; i<buffersize; i++){
      cbuffer[i] = 0;
    }
    count = 0;
  }
  
  if(controller_active){
    // read sensor data
    int anal;
    anal = analogRead(3);
    //Serial.println(anal);
    if(controller_target < anal){
      go_left(55);
    }
    if(controller_target > anal){
      go_right(55);
    }
    if(abs(controller_target-anal) < 5){
      turn_off();
      controller_active = false;
    }
  }
  
}

int get_val() {
  char cval[3];
  for(int i=0; i<3;i++){
    cval[i] = cbuffer[i+1];
  }
  int ival = constrain(atoi(cval), 0, 255); 
  return ival;
}

void turn_off() {
  digitalWrite(P1, LOW);
  digitalWrite(N1, LOW);
  digitalWrite(P2, LOW);
  digitalWrite(N2, LOW);
}

void go_right(int val) {
  turn_off();
  digitalWrite(P1, HIGH);
  analogWrite(N1, val);
}

void go_left(int val) {
  turn_off();
  digitalWrite(P2, HIGH);
  analogWrite(N2, val);
}



