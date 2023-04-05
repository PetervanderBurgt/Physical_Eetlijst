//for general wifi and website connectivity
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
//for getting jsons
#include <ArduinoJson.h>
//for time
#include <WiFiUdp.h>
#include <NTPClient.h>


//Display
#include <TM1637Display.h>
#include "keys.h"

// WiFi parameters and eetlijst parameter to be configured to be configured
const char* ssid = DEFINED_SSID; // Hoofdlettergevoelig
const char* password = DEFINED_SSID_PASSWORD; // Hoofdlettergevoelig
const String Eetlijst_USERNAME = DEFINED_EETLIJST_USERNAME;
const String Eetlijst_PASSWORD = DEFINED_EETLIJST_PASSWORD;

//eetlijst parametes, in this example 7 people
const int noOfPeople = 7;
int red[noOfPeople];
int green[noOfPeople];
int blue = LOW;

//Display
// Module connection pins (Digital Pins)
#define CLK D2
#define DIO D3
TM1637Display display(CLK, DIO);

//ShiftRegister
#define latchPin D6
#define clockPin D5
#define dataPin D7

//variables for the token retrieval url and the api url
const char *token_url = "node.samenn.nl";
const char *api_url = "api.samenn.nl";
String retrieved_token = "";
//String which contains the number of people for query
const String Eetlijst_NUMBEROFPEOPLE = String(noOfPeople);

//should be in format yyyy-mm-dd (2023-03-21)
String current_date = "2023-03-20";
// string to be formatted to json
String returnedJsonString = "";

// arrays that will hold the order of the names, there eating statusses and their number of guests,
// and finally number of people that are eating
String actual_name_order[noOfPeople];
String actual_eating_status[noOfPeople];
int actual_number_of_guests[noOfPeople];
int hoeveelMeeEters = 0;

//variables for retriving the correct time
//offset in seconds from utc time
const long utcOffsetInSeconds = 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

//Wifi and httpclient variables
WiFiClientSecure wifiSecure;
HTTPClient http; //Declare an object of class HTTPClient

//ResetTimer variables
long previousMillis = 0;
const long interval = 1000 * 60 * 5;           // interval after which to reset (milliseconds)

void setup() {
  Serial.begin(9600);
  //set the values for coloring of lights
  blue = true;
  for (int i = 0; i < noOfPeople; i++) {
    red[i] = LOW;
    green[i] = LOW;
  }

  //display
  display.setBrightness(0x0f);
  //set Error on display
  if (retrieved_token == "") {
    setDisplay();
  }
  //Shift Register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  setShiftRegisters();

  //connect to the wifi
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
}

void loop() {
  //if this is the first time turning on, retrieve all information, otherwise wait for timeout
  if (retrieved_token == "") {
    // get the current date
    current_date = getDate();
    //check whether it has updated and it is not still unix 0 (1970)
    while (current_date[0] != '2') {
      current_date = getDate();
    }
    // get the bearer authorization token for api usage
    getToken();
    // use api to retrieve the information displayed by eetlijst
    returnedJsonString = getEters();
    // parese the information into the arrays used for display
    parseJsonString(returnedJsonString);

    //count how many people eat
    for (int i = 0; i < noOfPeople; i++) {
      if ((actual_eating_status[i] == "cook") || (actual_eating_status[i] == "eat_only") || (actual_eating_status[i] == "got_groceries")) {
                hoeveelMeeEters++;
      } else {
        //do not count to how many people are eating
      }
      hoeveelMeeEters = hoeveelMeeEters + actual_number_of_guests[i];
    }

    for (int i = 0; i < noOfPeople; i++) {
      //set lights red and green array
      red[i] = setRedLight(actual_eating_status[i]);
      green[i] = setGreenLight(actual_eating_status[i]);
    }
    //set blue light off
    blue = LOW;

    //print all information to the serial monitor
    for (int i = 0; i < 7; i++) Serial.print(actual_name_order[i] + ", ");
    Serial.println();
    for (int i = 0; i < 7; i++) Serial.print(actual_eating_status[i] + ", ");
    Serial.println();
    for (int i = 0; i < 7; i++) Serial.print(String(actual_number_of_guests[i]) + ", ");
    Serial.println();
    Serial.println("================================================");

    Serial.print("Number of people that are eating: ");
    Serial.println(hoeveelMeeEters);

    //set lights red and green array and (eten array)
    for (int i = 0; i < 7; i++) Serial.print(String(red[i]) + ", ");
    Serial.println();
    for (int i = 0; i < 7; i++) Serial.print(String(green[i]) + ", ");
    Serial.println();

    //set display
    setDisplay();

    //set shift register
    setShiftRegisters();


  } else {
    //Restart after interval to make sure that data displayed is up to date
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;
      Serial.println("Resetting ESP");
      ESP.restart();
    }
  }
}

void setDisplay() {
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  display.clear();
  // Zet mee eters
  if (retrieved_token == "") {
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

int setRedLight(String eet) {
  //should be low if off and high if on
  if (eet == "cook" || eet == "not_attending") {
    return HIGH;
  } else {
    return LOW;
  }
}

int setGreenLight(String eet) {
  //should be low if off and high if on
  if (eet == "cook" || eet == "eat_only") {
    return HIGH;
  } else {
    return LOW;
  }
}

int getToken() {
  WiFiClientSecure client;
  client.setInsecure(); // this is the magical line that makes everything work

  Serial.print("HTTPS Connecting");
  int r = 0; //retry counter
  while ((!client.connect(token_url, 443)) && (r < 30)) {
    delay(100);
    Serial.print(".");
    r++;
  }
  if (r == 30) {
    Serial.println("Connection failed");
  }
  else {
    Serial.println("Connected to web");
  }
  const char *token_url_extra = "/api/v1/auth/eetlijst/";
  String login_token = "{\"login\":\"" + Eetlijst_USERNAME + "\", \"pass\":\"" + Eetlijst_PASSWORD + "\"}";
  const char *body = login_token.c_str();
  //  const char *body = "{\"login\":\"borstelhuis\", \"pass\":\"anno1990\"}"; // a valid jsonObject

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["login"] = "borstelhuis";
  root["pass"] = "anno1990";
  root.printTo(Serial);
  Serial.println();


  char postStr[40];

  sprintf(postStr, "POST %s HTTP/1.1", token_url_extra);  // put together the string for HTTP POST

  client.println(postStr);
  client.print("Host: "); client.println(token_url);
  client.println("Content-Type: application/json");
  client.print("Content-Length: "); client.println(root.measureLength());
  client.println();    // extra `\r\n` to separate the http header and http body
  root.printTo(client);
  Serial.println("request sent");


  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000)
    { Serial.println(">>> Client Timeout !");
      client.stop(); return -1;
    }
  } // Read all the lines of the reply from server and print them to Serial
  String line = "";
  while (client.available())
  { line = client.readStringUntil('\r');
  }
  Serial.println();
  //  Serial.println("line = " + line);
  retrieved_token = line.substring(11, line.length() - 2); //Add Json parsing properly
  //  Serial.println(retrieved_token);
  Serial.println("Token Retrieved");

  Serial.println("closing connection");
}

String getEters() {
  WiFiClientSecure client;
  client.setInsecure(); // this is the magical line that makes everything work

  Serial.print("HTTPS Connecting");
  int r = 0; //retry counter
  while ((!client.connect(api_url, 443)) && (r < 30)) {
    delay(100);
    Serial.print(".");
    r++;
  }
  if (r == 30) {
    Serial.println("Connection failed");
  }
  else {
    Serial.println("Connected to web");
  }
  const char *token_url_extra = "/v1/graphql";
  //  const String query = "\nquery\x20MyQuery {\n    eetschema_event(\n      where: {start_date: {_gte: \"2023-03-21T00:00:00+00:00\", _lt: \"2023-03-21T23:00: 00 + 00: 00\"}}\n    ) {\n      group_id\n      event_attendees {\n        attending_user {\n          name\n        }\n        status\n        number_guests\n      }\n    }\n    eetschema_users_in_group(where: {order: {_lte: \"6\"}}) {\n      order\n      user {\n        name\n      }\n    }\n  }";

  const char *jsonquery = "{\"query\":\" query MyQuery ($time_in: timestamptz!,$time_out: timestamptz!,$no_of_people: Int!) {    eetschema_event(      where: {start_date: {_gte: $time_in, _lt:$time_out}}    ) {      event_attendees {        attending_user {          name        }        status        number_guests      }    }    eetschema_users_in_group(where: {order: {_lt: $no_of_people}}) {      order      user {        name      }    }  }\",";

  String variables_retrieving = "\"variables\":{\"time_in\":\"" + current_date + "T00:00:00+00:00\",\"time_out\":\"" + current_date + "T23:00:00+00:00\",\"no_of_people\":\"" + Eetlijst_NUMBEROFPEOPLE + "\"}}";
  const char *jsonvariables = variables_retrieving.c_str();
  //  const char* jsonvariables = "\"variables\":{\"time_in\":\"2023-03-21T00:00:00+00:00\",\"time_out\":\"2023-03-21T23:00:00+00:00\",\"no_of_people\":\"7\"}}";

  int newSize = strlen(jsonquery)  + strlen(jsonvariables) + 1;
  // Allocate new buffer
  char *body = (char *)malloc(newSize);

  // do the copy and concat
  strcpy(body, jsonquery);
  strcat(body, jsonvariables); // or strncat

  //  const char *body = "{\"query\":\"\\n          query EventSingleEventQuery($event_id: uuid!) {\\n            eetschema_event_by_pk(id: $event_id) {\\n              start_date\\n              name\\n              id\\n              description\\n              end_date\\n              open\\n              updated_at\\n              created_at\\n              closed_by\\n              linked_expenses_aggregate(where: { deleted: { _eq: false } }) {\\n                aggregate {\\n                  sum {\\n                    payed_amount\\n                  }\\n                }\\n              }\\n            }\\n            eetschema_event_attendees(where: { event_id: { _eq: $event_id } }) {\\n              status\\n              user_id\\n              number_guests\\n              comment\\n            }\\n          }\\n        \",\"variables\":{\"event_id\":\"a91be5d8-2143-4829-8c00-79b6fda00f3d\"},\"operationName\":\"EventSingleEventQuery\"}";
  //  const char *body = "{\"query\":\" query MyQuery ($time_in: timestamptz!,$time_out: timestamptz!,$no_of_people: Int!) {    eetschema_event(      where: {start_date: {_gte: $time_in, _lt:$time_out}}    ) {      event_attendees {        attending_user {          name        }        status        number_guests      }    }    eetschema_users_in_group(where: {order: {_lte: $no_of_people}}) {      order      user {        name      }    }  }\",\"variables\":{\"time_in\":\"2023-03-21T00:00:00+00:00\",\"time_out\":\"2023-03-21T23:00:00+00:00\",\"no_of_people\":\"6\"}}";

  char postStr[40];

  sprintf(postStr, "POST %s HTTP/1.1", token_url_extra);  // put together the string for HTTP POST

  client.println(postStr);
  client.print("Host: "); client.println(api_url);
  client.println("Content-Type: application/json");
  client.print("authorization: Bearer "); client.println(retrieved_token);
  client.print("Content-Length: "); client.println(strlen(body));
  client.println();    // extra `\r\n` to separate the http header and http body
  client.println(body);
  Serial.println("request sent");


  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000)
    { Serial.println(">>> Client Timeout !");
      client.stop();
    }
  } // Read all the lines of the reply from server and print them to Serial
  const int lines_max = 20;  // array capacity
  String lines[lines_max];
  int line_counter = 0;
  while (client.available())
  {
    lines[line_counter] = client.readStringUntil('\r');
    line_counter++;
  }
  String jsonReturned = lines[11];
  //  Serial.println(jsonReturned);
  Serial.println("Json Retrieved");

  Serial.println("closing connection");
  return jsonReturned;
}

String getDate() {
  timeClient.begin();

  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  char formattedDatechar[(4 + 1 + 2 + 1 + 2)];
  sprintf(formattedDatechar, "%04d-%02d-%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);

  String formattedDate = String(formattedDatechar);
  Serial.println(formattedDate);
  return formattedDate;
}

void parseJsonString(String returnedJson) {
  const size_t capacity = JSON_ARRAY_SIZE(1) + 2 * JSON_ARRAY_SIZE(7) + 16 * JSON_OBJECT_SIZE(1) + 8 * JSON_OBJECT_SIZE(2) + 7 * JSON_OBJECT_SIZE(3) + 750;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(returnedJson);

  String eating_status_names[noOfPeople];
  String eating_status[noOfPeople];
  int number_of_guests[noOfPeople];

  String name_order[noOfPeople];
  int int_order[noOfPeople];

  JsonArray& event_attendees = root["data"]["eetschema_event"][0]["event_attendees"];
  JsonArray& eetschema_order = root["data"]["eetschema_users_in_group"];

  for (int i = 0; i < noOfPeople; i++) {
    eating_status_names[i] = (event_attendees[i]["attending_user"]["name"]).as<String>();
    eating_status[i] = (event_attendees[i]["status"]).as<String>();
    number_of_guests[i] = event_attendees[i]["number_guests"];

    name_order[i] = eetschema_order[i]["user"]["name"].as<String>();
    int_order[i] = eetschema_order[i]["order"];
  }



  //initalize to doesnt know and zero eaters
  for (int i = 0; i < noOfPeople; i++) {
    actual_eating_status[i] = "dont_know_yet";
    actual_number_of_guests[i] = 0;
  }

  for (int i = 0; i < noOfPeople; i++) {
    actual_name_order[int_order[i]] = name_order[i];
  }

  for (int i = 0; i < noOfPeople; i++) {
    for (int j = 0; j < noOfPeople; j++) {
      if (actual_name_order[i] == eating_status_names[j]) {
        actual_eating_status[i] = eating_status[j];
        actual_number_of_guests[i] = number_of_guests[j];
      }
    }
  }
}
