// LIBRARY
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

// SETUP WIFI NETWORK
const char* ssid      = "Brother_Land";
const char* password  = "RpShidqi,00";

// SETUP HOST
const String host     = "http://karawingspufy.000webhostapp.com";

// MODE SISTEM
boolean modepinjam    = false;
boolean modekembali   = false;

void setup() {
  Serial.begin(9600);   Wire.begin(D1, D2);

  // KONEKSI KE WIFI
  WiFi.hostname("NodeMCU");   WiFi.begin(ssid, password);

  // CEK KONEKSI
  while(WiFi.status() != WL_CONNECTED) { delay(500);  Serial.print("."); }
  Serial.println("Wifi Connected to IP Address : ");  Serial.print(WiFi.localIP());
}

void loop() {
  uploadData();
}

// FUNGSI PARSING DATA (VARIABEL, SEPARATOR, INDEX)
String getValue(String data, char separator, int index) {
  int found = 0;  int strIndex[] = {0, -1}; int maxIndex = data.length()-1;
 
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i) == separator || i == maxIndex){
        found++;  strIndex[0] = strIndex[1]+1;  strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  } 
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// FUNGSI UPLOAD DATA KE DATABASE
void uploadData() {
  String dataUpload, dataUpload2, data, payload, Link, nama, nim, batas, buku, pengarang;   
  Wire.requestFrom(8, 40);
  while(Wire.available()>0){
    data = Wire.readStringUntil('?');
    delay(20);
    if (data != "") {
      // % = KARTU PINJAM     ||  & = KARTU KEMBALI      || @ = BARCODE PINJAM 
      // # = BARCODE KEMBALI  ||  * = FULL MODE PINJAM   || ! = FULL MODE KEMBALI
      Serial.println(data);
      if (data.endsWith("%")) { dataUpload  = getValue(data, '$', 1);  Serial.println(dataUpload);  }
      else if (data.endsWith("&")) { dataUpload  = getValue(data, '$', 1);  Serial.println(dataUpload);  }
      else if (data.endsWith("@")) { dataUpload  = getValue(data, '$', 1);  Serial.println(dataUpload);  }     
      else if (data.endsWith("#")) { dataUpload  = getValue(data, '$', 1);  Serial.println(dataUpload);  }
      else {
        if (data.indexOf("*") > 0) { dataUpload  = getValue(data, '*', 0);  Serial.println(dataUpload);  
                                     dataUpload2 = getValue(data, '*', 1);  Serial.println(dataUpload2); } 
        if (data.indexOf("!") > 0) { dataUpload  = getValue(data, '!', 0);  Serial.println(dataUpload);  
                                     dataUpload2 = getValue(data, '!', 1);  Serial.println(dataUpload2); } 
      }
    }

    // KONFIGURASI KONEKSI KE HOST
    WiFiClient client;  const int httpPort = 80;
    while (!client.connect(host, httpPort)) { Serial.println("Connection failed"); return; }
    
    // UPLOAD ID KARTU PINJAM BUKU
    if (data.endsWith("%") && dataUpload != "") {
      // KONFIGURASI KONEKSI KE LINK 
      HTTPClient http;
      Link = host + "/pinjam.php?id=" + dataUpload;
      http.begin(client, Link); Serial.println(Link); Serial.println("UPLOAD ID KARTU MODE PINJAM BUKU");
  
      // RESPONSE DATABASE -> #shidqi#1803321034#19 Juli 2021#$
      int httpCode = http.GET();  payload = http.getString(); Serial.println(payload);
      http.end();
        
      if (payload == "NO%") { Wire.beginTransmission(8);  Wire.print(payload);  Wire.endTransmission(); }

      else {
        nama  = getValue(payload, '#', 1);  Serial.println("Nama: " +nama);      
        nim   = getValue(payload, '#', 2);  Serial.println("NIM: " + nim);
        batas = getValue(payload, '#', 3);  Serial.println("Batas Pinjam: " + batas);
 
        Wire.beginTransmission(8); Wire.print("#"+nama);  Wire.endTransmission();
        Wire.beginTransmission(8); Wire.print("$"+nim);   Wire.endTransmission();
        Wire.beginTransmission(8); Wire.print("%"+batas); Wire.endTransmission();
      }
      
      // FUNGSI END CONNECTION HOST
      unsigned long timeout = millis();
      while (client.available() == 0) { if (millis() - timeout > 500) { client.stop(); return; } }
      return;
    }

    // UPLOAD ID KARTU KEMBALI BUKU
    else if (data.endsWith("&") && dataUpload != "") {
      // KONFIGURASI KONEKSI KE LINK 
      HTTPClient http;
      Link = host + "/kembali.php?id=" + dataUpload;
      http.begin(client, Link); Serial.println(Link); Serial.println("UPLOAD ID KARTU MODE KEMBALI BUKU");

      // RESPONSE DATABASE -> #shidqi#1803321034#$
      int httpCode = http.GET();  payload = http.getString(); Serial.println(payload);  
      http.end();

      if (payload == "NO&") { Wire.beginTransmission(8);  Wire.print(payload);  Wire.endTransmission(); }

      else {
        nama  = getValue(payload, '#', 1);    Serial.println("Nama: " + nama);
        nim   = getValue(payload, '#', 2);    Serial.println("NIM: " + nim);

        Wire.beginTransmission(8); Wire.print("#" + nama); Wire.endTransmission();
        Wire.beginTransmission(8); Wire.print("$" + nim);  Wire.endTransmission();
      }
      
      // FUNGSI END CONNECTION HOST
      unsigned long timeout = millis();
      while (client.available() == 0) { if (millis() - timeout > 500) { client.stop(); return; } }
      return;
    }

    // UPLOAD BARCODE BUKU PINJAM BUKU
    else if (data.endsWith("@") && dataUpload != "") {
      dataUpload = dataUpload.toDouble();   dataUpload = getValue(dataUpload, '.', 0);
    
      // KONFIGURASI KONEKSI KE LINK 
      HTTPClient http;
      Link = host + "/pinjam.php?barcode=" + dataUpload;
      http.begin(client, Link); Serial.println(Link); Serial.println("UPLOAD BARCODE BUKU MODE PINJAM BUKU");
  
      // RESPONSE DATABASE -> %elektronika digital%wicaksono%13 Maret 2021%$
      int httpCode = http.GET();  payload = http.getString(); Serial.println(payload);
      http.end();

      if (payload == "NO@") { Wire.beginTransmission(8);  Wire.print(payload);  Wire.endTransmission(); }

      else {
        buku      = getValue(payload, '%', 1);    Serial.println("Buku: " + buku);
        pengarang = getValue(payload, '%', 2);    Serial.println("Pengarang: " + pengarang);
  
        Wire.beginTransmission(8);  Wire.print("!" + buku);       Wire.endTransmission();
        Wire.beginTransmission(8);  Wire.print("@" + pengarang);  Wire.endTransmission();
      }
      
      // FUNGSI END CONNECTION HOST
      unsigned long timeout = millis();
      while (client.available() == 0) { if (millis() - timeout > 500) { client.stop(); return; } }
      return;
    }

    // UPLOAD BARCODE BUKU KEMBALI BUKU
    else if (data.endsWith("#") && dataUpload != "") {
      dataUpload = dataUpload.toDouble();    dataUpload = getValue(dataUpload, '.', 0);
    
      // KONFIGURASI KONEKSI KE LINK
      HTTPClient http;
      Link = host + "/kembali.php?barcode=" + dataUpload;
      http.begin(client, Link); Serial.println(Link); Serial.println("UPLOAD BARCODE BUKU MODE KEMBALI BUKU");

      // RESPONSE DATABASE -> %elektronika digital%wicaksono%&   
      int httpCode = http.GET();  payload = http.getString();  Serial.println(payload); 
      http.end();

      if (payload == "NO#") { Wire.beginTransmission(8);  Wire.print(payload);  Wire.endTransmission(); }
    
      else {
        buku      = getValue(payload, '%', 1);    Serial.println("Buku: " + buku);
        pengarang = getValue(payload, '%', 2);    Serial.println("Pengarang: " + pengarang);
  
        Wire.beginTransmission(8);  Wire.print("!" + buku);       Wire.endTransmission();
        Wire.beginTransmission(8);  Wire.print("@" + pengarang);  Wire.endTransmission();
      } 
      
      // FUNGSI END CONNECTION HOST
      unsigned long timeout = millis();
      while (client.available() == 0) { if (millis() - timeout > 500) { client.stop(); return; } }
      return;
    }

    // UPLOAD FULL MODE PINJAM BUKU
    else {
      if (data.indexOf("*") > 0 && dataUpload != "" && dataUpload2 != "") {
        dataUpload2 = dataUpload2.toDouble();  dataUpload2 = getValue(dataUpload2, '.', 0);
      
        // KONFIGURASI KONEKSI KE LINK
        HTTPClient http;
        Link = host + "/pinjam.php?id=" + dataUpload + "&barcode=" + dataUpload2;
        http.begin(client, Link); Serial.println(Link); Serial.println("UPLOAD FULL MODE PINJAM BUKU");
  
        // RESPONSE DATABASE -> YES / NO
        int httpCode = http.GET();  payload = http.getString();  Serial.println(payload); 
        http.end();
  
        Wire.beginTransmission(8);  Wire.print(payload);  Wire.endTransmission(); 
  
        // FUNGSI END CONNECTION HOST
        unsigned long timeout = millis();
        while (client.available() == 0) { if (millis() - timeout > 500) { client.stop(); return; } }
        return;
      }
  
      // UPLOAD FULL MODE KEMBALI BUKU
      if (data.indexOf("!") > 0 && dataUpload != "" && dataUpload2 != "") {
        dataUpload2 = dataUpload2.toDouble();  dataUpload2 = getValue(dataUpload2, '.', 0);
      
        // KONFIGURASI KONEKSI KE LINK
        HTTPClient http;
        Link = host + "/kembali.php?id=" + dataUpload + "&barcode=" + dataUpload2;
        http.begin(client, Link); Serial.println(Link); Serial.println("UPLOAD FULL MODE KEMBALI BUKU");
  
        // RESPONSE DATABASE -> denda / NO
        int httpCode = http.GET();  payload = http.getString();  Serial.println(payload); 
        http.end();
  
        Wire.beginTransmission(8);  Wire.print(payload);  Wire.endTransmission(); 
  
        // FUNGSI END CONNECTION HOST
        unsigned long timeout = millis();
        while (client.available() == 0) { if (millis() - timeout > 500) { client.stop(); return; } }
        return;
      }
    }
  }
}
