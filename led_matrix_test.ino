#include <Twitter.h>
#include <LedControl.h>
#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>


Twitter twitter("2505156788-Wbc1QHUMjU1UpWZENXRnXByCIybIJvotqtZrbPV");

LiquidCrystal lcd(5, 6, 17, 16, 15, 14);

#define MaxHeaderLength 16    //maximum length of http header required
String HttpHeader = String(MaxHeaderLength);
String readData = String();
boolean attribute;

byte mac[] = {0xD8, 0x50, 0xE6, 0x89, 0x62, 0xB3};
IPAddress ip(192, 168, 1, 50);

EthernetServer server(80);

//GPIO pins used
int DIN = 9;
int CS =  8;
int CLK = 7;

//Enter starttime here
int seconds = 45;
int minutes = 30;
int hours = 03;

//Alarmtime
int alarmMin = 20;
int alarmHour = 23;
boolean alarmOn = true;

//Time to track time difference
long lastTime = 0;

//                  Hour        Hour     Space Minutes    Minutes   Space Seconds     Seconds
byte myTime[8] =     {0b00000010, 0b00000111, 0x00, 0b00000000, 0b00000000, 0x00, 0b00000000, 0b00000000};

//Control for LED-matrix, parameters for
LedControl lc = LedControl(DIN, CLK, CS, 1);

const int ALARMPIN = 2;
const int SOUNDPIN = 3;
const int BUTTONPIN = 18;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  server.begin();
  pinMode(ALARMPIN, OUTPUT);
  pinMode(BUTTONPIN, INPUT);

  lc.shutdown(0, false);      //The MAX72XX is in power-saving mode on startup
  lc.setIntensity(0, 15);     // Set the brightness to maximum value
  lc.clearDisplay(0);         // and clear the display

  //initialize variable
  HttpHeader = "";

  lcd.begin(16, 2);
  lcd.print("Welcome!");
/*
  if (twitter.post("Good night -Arduino smartbox")) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("OK.");
    } else {
      Serial.print("failed : code ");
      Serial.println(status);
    }
  } else {
    Serial.println("connection failed.");
  }*/
}

void loop() {

  if (millis() - lastTime >= 990) {
    lastTime = millis();
    tick();
    checkAlarm();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Current time");
    lcd.setCursor(0, 1);
    if (hours < 10) {
      lcd.print("0");
    }
    lcd.print(hours);

    lcd.print(":");
    if (minutes < 10) {
      lcd.print("0");
    }
    lcd.print(minutes);

    lcd.print(":");
    if (seconds < 10) {
      lcd.print("0");
    }
    lcd.print(seconds);

    if (digitalRead(BUTTONPIN) == HIGH) {
      if (alarmOn) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Alarm canceled");
        cancelAlarm();
      }
    }
    
  }

  printByte(myTime);
  delay(1);
  EthernetClient client = server.available();
  if (client) {
    attribute = false;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //read MaxHeaderLength number of characters in the HTTP header
        //discard the rest until \n
        if (HttpHeader.length() < MaxHeaderLength)
        {
          //store characters to string
          HttpHeader = HttpHeader + c;
        }
        if (c == '?') {
          attribute = true;
        }
        if (attribute) {
          readData += c;

          if (c == ' ') {
            String head = readData.substring(1, 4);
            Serial.print("head ");
            Serial.println(head);
            if (head == "led") {
              String result = readData.substring(5, 6);
              Serial.println("LED ");
              Serial.println(result);
              Serial.println(readData);
              if (result == "1") {
                digitalWrite(ALARMPIN, HIGH);
              } else if (result == "2") {
                digitalWrite(ALARMPIN, LOW);
              }
              attribute = false;
            } else if (head == "ala") {
              Serial.println("Alarm ");
              String inHour = readData.substring(7, 9);
              String inMin = readData.substring(12, 14);
              Serial.println(readData);
              Serial.print(inHour);
              Serial.print(":");
              Serial.println(inMin);

              alarmHour = inHour.toInt();
              alarmMin = inMin.toInt();
              alarmOn = true;
              attribute = false;

            } else if (head == "swi") {
              Serial.println("Abort ");
              cancelAlarm();
              attribute = false;
            }
            else if (head == "tim") {
              Serial.println("Setting time ");
              String inHour = readData.substring(5, 7);
              String inMin = readData.substring(10, 12);
              Serial.println(readData);
              Serial.print(inHour);
              Serial.print(":");
              Serial.println(inMin);

              hours = inHour.toInt();
              minutes = inMin.toInt();
              attribute = false;

            }
            readData = "";
          }
        }
        //if HTTP request has ended
        if (c == '\n') {
          // show the string on the monitor
          Serial.println(HttpHeader);
          // start of web page
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("<html><head></head><body>");
          client.println();

          client.print("<h1>Led control</h1>");

          client.print("<form method=get>");
          client.print("<input type='hidden' name=led value='1'><br>");
          client.print("<input type=submit value=On></form>");

          client.print("<form method=get>");
          client.print("<input type='hidden' name=led value='2'><br>");
          client.print("<input type=submit value=Off></form>");

          client.print("<h1>Set Alarm</h1>");
          if (alarmOn) {
            client.print("<h3>Next alarm ");
            client.print(alarmHour);
            client.print(":");
            client.print(alarmMin);
            client.print("</h3>");
          }
          client.print("<form method=get>");
          client.print("<input type='time' name=alarm value='set'><br>");
          client.print("<input type=submit value=set></form>");

          client.print("<h3>Cancel alarm</h3>");
          client.print("<form method=get>");
          client.print("<input type='hidden' name=swi value='can'><br>");
          client.print("<input type=submit value=abort></form>");

          client.print("<h1>Set Time</h1>");
          client.print("<form method=get>");
          client.print("<input type='time' name=tim value='set'><br>");
          client.print("<input type=submit value=set></form>");
          client.print("</body></html>");
          //clearing string for next read
          HttpHeader = "";
          //stopping client
          client.stop();
        }
      }
    }
  }
}

/**
 * Prints a byte to a column on the LED-matrix
 */
void printByte(byte character [])
{
  int i = 0;
  for (i = 0; i < 8; i++)
  {
    lc.setColumn(0, i, character[i]);
  }
}

/**
 * Executes every second to track time
 */
void tick() {
  seconds++;
  if (seconds == 60) {
    seconds = 0;
    minutes++;
    if (minutes == 60) {
      minutes = 0;
      hours++;
      if (hours == 24) {
        hours = 0;
      }
    }
  }
  updateTime();
}

/**
 * Updates byte array containing the time
 */
void updateTime() {
  int h1 = (hours % 100) / 10;
  int h2 = hours % 10;

  int m1 = (minutes % 100) / 10;
  int m2 = minutes % 10;

  int s1 = (seconds % 100) / 10;
  int s2 = (seconds % 10);

  myTime[0] = getByte(h1);
  myTime[1] = getByte(h2);

  myTime[3] = getByte(m1);
  myTime[4] = getByte(m2);


  myTime[6] = getByte(s1);
  myTime[7] = getByte(s2);


}

/**
 * Returns a rightmost four bits of an integer
 */
byte getByte(int num) {

  byte ret;
  for (int i = 0; i < 4; i++) {
    int b = bitRead(num, i);
    bitWrite(ret, i, b);

  }
  return ret;
}

/**
 * Execute alarm
 */
void alarm() {
  digitalWrite(ALARMPIN, HIGH);
  alarmSound();
}
void alarmSound() {
  tone(SOUNDPIN, 350, 100);
  // delay(100);
}

/**
 * Check alarm
 */
void checkAlarm() {
  if (alarmOn) {
    if (minutes == alarmMin && hours == alarmHour) {
      alarm();
    }
  }
}
/**
 * Cancels
 */
void cancelAlarm() {
  alarmOn = false;
  noTone(SOUNDPIN);
  digitalWrite(ALARMPIN, LOW);
}

