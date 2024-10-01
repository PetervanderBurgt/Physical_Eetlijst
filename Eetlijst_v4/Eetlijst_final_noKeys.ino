#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//Display
#include <TM1637Display.h>

// WiFi parameters to be configured
const char* ssid = "Your_SSID"; // Hoofdlettergevoelig
const char* password = "Your_password"; // Hoofdlettergevoelig

//eetlijst parametes, in this example 7 people
int noOfPeople = 7;
String members[] = {"", "", "", "", "", "", ""};
String food[] = {"", "", "", "", "", "", ""};
int eten[] = { -5, -5, -5, -5, -5, -5, -5};
int red[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW};
int green[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW};
int blue = LOW;
int hoeveelMeeEters = -1;
int oldHoeveelMeeEters = -2;

//Display
// Module connection pins (Digital Pins)
#define CLK D2
#define DIO D3
TM1637Display display(CLK, DIO);

//ShiftRegister
int latchPin = D6;
int clockPin = D5;
int dataPin = D7;

// Data website to get and storing variables
String BASE_URL = "http://eetlijst.nl/";
String LOGIN = "http://eetlijst.nl/login.php?login=[EetlijstLoginName]&pass=[EetlijstPassword]";
String main_page = "";
HTTPClient http; //Declare an object of class HTTPClient


//ResetTimer
long previousMillis = 0;
long interval = 300 * 1000;           // interval after which to reset (milliseconds)


String locationRedirect = "";

void setup() {
  Serial.begin(9600);
  blue = true;
  //display
  display.setBrightness(0x0f);
  if (hoeveelMeeEters != oldHoeveelMeeEters) {
    oldHoeveelMeeEters = hoeveelMeeEters;
    setDisplay();
  }
  //Shift Register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  setShiftRegisters();

  Serial.print("Bezig met verbinden");
  WiFi.hostname("Eetlijst ESP");

  WiFi.begin(ssid, password); // Connect to WiFi

  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Verbonden.
  Serial.println("OK!");

  // Access Point (SSID).
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // IP adres.
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Signaalsterkte.
  long rssi = WiFi.RSSI();
  Serial.print("Signaal sterkte (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println("");
  // ESP.restart();      //RESET THE ENTIRE SYSTEM
}

void loop() {
  if (!(main_page.length() > 0)) {
    blue = HIGH;
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
      //    payload = {"login": self.username, "pass": self.password};
      //Serial.println(locationRedirect);
      if (locationRedirect == "") {
        Serial.println("Getting new Session ID");
        getSessionID();
      } else {
        Serial.println("Using new Session ID");
        getMainPage();
      }
    } else {
      Serial.println("Error in WiFi connection");
    }
  } else {
    blue = LOW;
    getAllNames();
    getFood();
    for (int i = 0; i < noOfPeople; i++) {
      eten[i] = findFood(food[i]);
      red[i] = setRedLight(eten[i]);
      green[i] = setGreenLight(eten[i]);
    }
    hoeveelMeeEters = telMeeEters();

    printEet();
    Serial.print("Zoveel mensen eten er mee: ");
    Serial.println(hoeveelMeeEters);

    //Set Values to Outputs
    if (hoeveelMeeEters != oldHoeveelMeeEters) {
      oldHoeveelMeeEters = hoeveelMeeEters;
      setDisplay();
    }
    setShiftRegisters();

    delay(interval / 10); //Send a request every 30 seconds
  }
  //Restart after interval to make sure that data displayed is up to date
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    Serial.println("Resetting ESP");
    ESP.restart();
  }
}

void setDisplay() {
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  display.clear();
  // Zet mee eters
  if (hoeveelMeeEters < 0 || hoeveelMeeEters > 30 ) {
    data[0] = SEG_A | SEG_D | SEG_E | SEG_F | SEG_G; // letter E
    data[1] = SEG_E | SEG_G;                 // letter r
  } else if (hoeveelMeeEters > 20) {
    data[0] = display.encodeDigit(2);
    data[1] = display.encodeDigit(hoeveelMeeEters % 20);
  } else if (hoeveelMeeEters > 10) {
    data[0] = display.encodeDigit(1);
    data[1] = display.encodeDigit(hoeveelMeeEters % 10);
  } else {
    data[0] = 0x00;
    data[1] = display.encodeDigit(hoeveelMeeEters);
  }
  data[2] = 0x00;
  data[3] = SEG_A | SEG_B | SEG_E | SEG_F | SEG_G; //letter P
  display.setSegments(data);
}
void setShiftRegisters() {
  int shiftOutValue1 = 0;
  int shiftOutValue2 = 0;
  if (blue)     shiftOutValue1 = shiftOutValue1 + pow(2, 0); //0x00000001 /1

  if (red[0])   shiftOutValue1 = shiftOutValue1 + pow(2, 1); //0x00000010 /1
  if (green[0]) shiftOutValue1 = shiftOutValue1 + pow(2, 2); //0x00000100 /1
  if (red[1])   shiftOutValue1 = shiftOutValue1 + pow(2, 3); //0x00001000 /1
  if (green[1]) shiftOutValue1 = shiftOutValue1 + pow(2, 4); //0x00010000 /1
  if (red[2])   shiftOutValue1 = shiftOutValue1 + pow(2, 5); //0x00100000 /1
  if (green[2]) shiftOutValue1 = shiftOutValue1 + pow(2, 6); //0x01000000 /1
  if (red[3])   shiftOutValue1 = shiftOutValue1 + pow(2, 7); //0x10000000 /1
  if (green[3]) shiftOutValue2 = shiftOutValue2 + pow(2, 1); //0x00000010 /2
  if (red[4])   shiftOutValue2 = shiftOutValue2 + pow(2, 2); //0x00000100 /2
  if (green[4]) shiftOutValue2 = shiftOutValue2 + pow(2, 3); //0x00001000 /2
  if (red[5])   shiftOutValue2 = shiftOutValue2 + pow(2, 4); //0x00010000 /2
  if (green[5]) shiftOutValue2 = shiftOutValue2 + pow(2, 5); //0x00100000 /2
  if (red[6])   shiftOutValue2 = shiftOutValue2 + pow(2, 6); //0x01000000 /2
  if (green[6]) shiftOutValue2 = shiftOutValue2 + pow(2, 7); //0x10000000 /2

  Serial.println(shiftOutValue1, BIN);
  Serial.println(shiftOutValue2, BIN);
  // take the latchPin low so
  // the LEDs don't change while you're sending in bits:
  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, MSBFIRST, shiftOutValue2);
  shiftOut(dataPin, clockPin, MSBFIRST, shiftOutValue1);
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
  // pause before next value:
}

int setRedLight(int eet) {
  if (eet == -5 || eet > 0) {
    return HIGH;
  } else {
    return LOW;
  }
}

int setGreenLight(int eet) {
  if (eet == -5 || eet < 0) {
    return HIGH;
  } else {
    return LOW;
  }
}

int telMeeEters() {
  int meeEters = 0;
  for (int i = 0; i < noOfPeople; i++) {
    if (eten[i] != -5) {
      meeEters += abs(eten[i]);
    }
  }
  return meeEters;
}

void printEet() {
  if (members[0] != "" && food[0] != "") {
    Serial.println("-----------------------------------------");
    for (int i = 0; i < noOfPeople ; i++) {
      Serial.print(members[i]);
      Serial.print("\t");
      Serial.print(eten[i]);
      //      Serial.print("\t");
      //      Serial.print(red[i]);
      //      Serial.print("\t");
      //      Serial.print(green[i]);
      Serial.println();
    }
    Serial.println("-----------------------------------------");
  }
}

void getAllNames() {
  int beginIndex = main_page.indexOf("Meer informatie over ");
  int eindIndex = main_page.lastIndexOf("Meer informatie over ");

  members[0] = main_page.substring(beginIndex + sizeof("Meer informatie over") , main_page.indexOf('"', beginIndex + 1));
  members[1] = main_page.substring(main_page.indexOf("Meer informatie over ", main_page.indexOf(members[0])) + sizeof("Meer informatie over"), main_page.indexOf('"', main_page.indexOf("Meer informatie over ", main_page.indexOf(members[0])) + 1));
  members[2] = main_page.substring(main_page.indexOf("Meer informatie over ", main_page.indexOf(members[1])) + sizeof("Meer informatie over"), main_page.indexOf('"', main_page.indexOf("Meer informatie over ", main_page.indexOf(members[1])) + 1));
  members[3] = main_page.substring(main_page.indexOf("Meer informatie over ", main_page.indexOf(members[2])) + sizeof("Meer informatie over"), main_page.indexOf('"', main_page.indexOf("Meer informatie over ", main_page.indexOf(members[2])) + 1));
  members[4] = main_page.substring(main_page.indexOf("Meer informatie over ", main_page.indexOf(members[3])) + sizeof("Meer informatie over"), main_page.indexOf('"', main_page.indexOf("Meer informatie over ", main_page.indexOf(members[3])) + 1));
  members[5] = main_page.substring(main_page.indexOf("Meer informatie over ", main_page.indexOf(members[4])) + sizeof("Meer informatie over"), main_page.indexOf('"', main_page.indexOf("Meer informatie over ", main_page.indexOf(members[4])) + 1));
  members[6] = main_page.substring(main_page.indexOf("Meer informatie over ", main_page.indexOf(members[5])) + sizeof("Meer informatie over"), main_page.indexOf('"', main_page.indexOf("Meer informatie over ", main_page.indexOf(members[5])) + 1));
  //  Serial.println(eindIndex);
}
int findFood(String food) {
  String eten = "eet.gif";
  String kook = "kook.gif";
  String nope = "nop.gif";
  String leeg = "leeg.gif";

  //  Serial.println(food);
  if (food.indexOf(leeg) > 0) {
    return -5;
  }
  if (food.indexOf(nope) > 0) {
    return 0;
  }
  if (food.indexOf(kook) > 0) {
    int eentje = food.indexOf(kook);

    if (food.indexOf(eten, eentje + 1) > 0) {
      int tweetje = food.indexOf(eten, eentje + 1);

      if (food.indexOf(eten, tweetje + 1) > 0) {
        int drietje = food.indexOf(eten, tweetje + 1);

        if (food.indexOf(eten, drietje + 1) > 0) {
          return 4;
        }
        return 3;
      }
      return 2;
    }
    return 1;
  }  
  if (food.indexOf(eten) > 0) {
    int eentje = food.indexOf(eten);

    if (food.indexOf(eten, eentje + 1) > 0) {
      int tweetje = food.indexOf(eten, eentje + 1);

      if (food.indexOf(eten, tweetje + 1) > 0) {
        int drietje = food.indexOf(eten, tweetje + 1);

        if (food.indexOf(eten, drietje + 1) > 0) {
          return -4;
        }
        return -3;
      }
      return -2;
    }
    return -1;
  }


}

void getFood() {
  int tabelIndex = main_page.indexOf("<td valign=\"top\">");
  int datumIndex = main_page.indexOf("<td", tabelIndex + 1);
  int tijdIndex = main_page.indexOf("<td", datumIndex + 1);
  int beginIndex = main_page.indexOf("<td", tijdIndex + 1);
  int tweeIndex = main_page.indexOf("<td", beginIndex + 1);
  int drieIndex = main_page.indexOf("<td", tweeIndex + 1);
  int vierIndex = main_page.indexOf("<td", drieIndex + 1);
  int vijfIndex = main_page.indexOf("<td", vierIndex + 1);
  int zesIndex = main_page.indexOf("<td", vijfIndex + 1);
  int zevenIndex = main_page.indexOf("<td", zesIndex + 1);

  food[0] = main_page.substring(beginIndex + sizeof("img src=") , main_page.indexOf("</td>", beginIndex + sizeof("img src=") + 1));
  food[1] = main_page.substring(tweeIndex + sizeof("img src=") , main_page.indexOf("</td>", tweeIndex + sizeof("img src=") + 1));
  food[2] = main_page.substring(drieIndex + sizeof("img src=") , main_page.indexOf("</td>", drieIndex + sizeof("img src=") + 1));
  food[3] = main_page.substring(vierIndex + sizeof("img src=") , main_page.indexOf("</td>", vierIndex + sizeof("img src=") + 1));
  food[4] = main_page.substring(vijfIndex + sizeof("img src=") , main_page.indexOf("</td>", vijfIndex + sizeof("img src=") + 1));
  food[5] = main_page.substring(zesIndex + sizeof("img src=") , main_page.indexOf("</td>", zesIndex + sizeof("img src=") + 1));
  food[6] = main_page.substring(zevenIndex + sizeof("img src=") , main_page.indexOf("</td>", zevenIndex + sizeof("img src=") + 1));

  //Serial.println(beginIndex);
}

int getSessionID() {
  http.begin((LOGIN).c_str()); //Specify request destination

  int httpCode = http.GET(); //Send the request
  if (httpCode > 0) { //Check the returning code
    String payload = http.header("Date"); //Get the request response payload
    locationRedirect = http.getLocation();
    Serial.println(httpCode); //Print the response payload
    Serial.println(locationRedirect); //Print the response payload
  }
  http.end(); //Close connection
  return httpCode;
}

int getMainPage() {
  http.begin((BASE_URL + locationRedirect).c_str()); //Specify request destination

  int httpCode = http.GET(); //Send the request
  if (httpCode == 200) { //Check the returning code
    if (http.getString().length() > main_page.length()) {
      main_page = http.getString(); //Get the request response payload
    }
    Serial.println(httpCode); //Print the response payload

  } else {
    Serial.print("Unexpected error code: ");
    Serial.println(httpCode);
  }
  http.end();
  main_page = main_page.substring(main_page.indexOf("<!-- Hoofdscherm - Eetlijst -->"), main_page.indexOf("<tr><th colspan=\"4\">Eetlijst aanpassen</th></tr>"));

  return httpCode;

}
