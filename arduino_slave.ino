// LIBRARY
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <SPI.h>
#include <MFRC522.h>

// KONFIGURASI PIN RST DAN SS KOMUNIKASI SPI RFID MODUL
#define RST_PIN         9          
#define SS_PIN          10         

// KOMUNIKASI VARIABEL RFID MODUL DAN BARCODE SCANNER
MFRC522         mfrc522 (SS_PIN, RST_PIN);
SoftwareSerial  Barcode (2,3);

// MODE OPERASI
boolean modepinjam      = false;
boolean modepinjamfull  = false;
boolean modekembali     = false;
boolean modekembalifull = false;

// VARIABEL MODE FULL
String kartu, bar;

// PICTURE BUTTON DAN BUTTON EVENT NEXTION
NexPicture p6 = NexPicture(8, 7, "p6");     // Overview MODE KEMBALI BUKU 
NexPicture p7 = NexPicture(4, 9, "p7");     // Overview MODE PINJAM BUKU 
NexButton b1  = NexButton(0, 6, "b1");      // Button added MODE KEMBALI BUKU
NexButton b0  = NexButton(0, 5, "b0");      // Button added MODE PINJAM BUKU

NexTouch *nex_listen_list[] = {
  &p6,&p7,&b1,&b0,NULL
};

// KIRIM DATA KONFIRMASI (MODE KEMBALI BUKU)
void p6PopCallback(void *ptr) {
  modepinjamfull = false;   modekembalifull = true;
}

// KIRIM DATA KONFIRMASI (MODE PINJAM BUKU)
void p7PopCallback(void *ptr) {
  modepinjamfull = true;    modekembalifull = false;
}

// IDENTIFIKASI MODE KEMBALI BUKU
void b1PopCallback(void *ptr) { 
  modepinjam = false;   modekembali = true;   Serial.println("MODE KEMBALI AKTIF");
}

// IDENTIFIKASI MODE PINJAM BUKU
void b0PopCallback(void *ptr) { 
  modepinjam = true;    modekembali = false;  Serial.println("MODE PINJAM AKTIF");
}

void setup() {
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Serial.begin(9600);              
  Barcode.begin(9600);
  
  while (!Serial);
  SPI.begin();         
  mfrc522.PCD_Init(); 
  delay(4);

  b1.attachPop(b1PopCallback);
  b0.attachPop(b0PopCallback);
  p7.attachPop(p7PopCallback);
  p6.attachPop(p6PopCallback);
}

void loop() {
  nexLoop(nex_listen_list);
}

// FUNGSI TERIMA DATA DARI MASTER
void receiveEvent(int howMany) {
  while (Wire.available() > 0) { responseDatabase(); }
}

// FUNGSI KIRIM DATA KE MASTER
void requestEvent() {
  kirimData();
}

// FUNGSI PARSING DATA (VARIABEL, SEPARATOR, INDEX)
String getValue(String data, char separator, int index) {
  int found = 0;  int strIndex[] = {0, -1}; int maxIndex = data.length()-1;
 
  for(int i=0; i<=maxIndex && found<=index; i++) {
    if(data.charAt(i) == separator || i == maxIndex) {
        found++;  strIndex[0] = strIndex[1]+1;  strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  } 
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void endNextion() {
  Serial.print("\xFF\xFF\xFF");
}

// FUNGSI KIRIM DATA ID KARTU DAN BARCODE BUKU KE ESP
void kirimData() {
  String IDTAG, kodebarcode, varKirim;
  // BACA KODE BARCODE BUKU
  while (Barcode.available()>0) { kodebarcode += char (Barcode.read()); }
  
  // BACA ID KARTU
  if (mfrc522.PICC_IsNewCardPresent())
  if (mfrc522.PICC_ReadCardSerial())

  for(byte i=0; i<mfrc522.uid.size; i++) { IDTAG += mfrc522.uid.uidByte[i]; }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // PINDAHKAN KE GLOBAL SCOPE AGAR DATA TIDAK RESET
  if (IDTAG != "") kartu = IDTAG;  if (kodebarcode != "") bar = kodebarcode;

  // KIRIM DATA ID KARTU KE ESP
  if (IDTAG != "") {
    if (modepinjam == true && modekembali == false) {
      varKirim = "$" + IDTAG + "$%?";              // AKHIRI "%" ID KARTU MODE PINJAM BUKU
      Serial.println(varKirim);   Wire.print(varKirim); 
    }
    if (modepinjam == false && modekembali == true) {
      varKirim = "$" + IDTAG + "$&?";              // AKHIRI "&" ID KARTU MODE KEMBALI BUKU
      Serial.println(varKirim);     Wire.print(varKirim);
    }
  }

  // KIRIM DATA BARCODE BUKU KE ESP
  if (kodebarcode != "") {
    if (modepinjam == true && modekembali == false) {
      varKirim = "$" + kodebarcode + "$@?";        // AKHIRI "@" BARCODE MODE PINJAM BUKU
      Serial.println(varKirim);     Wire.print(varKirim); 
    }
    if (modepinjam == false && modekembali == true) {
      varKirim = "$" + kodebarcode + "$#?";        // AKHIRI "#" BARCODE MODE KEMBALI BUKU
      Serial.println(varKirim);     Wire.print(varKirim);     
    }
  }
 
  // KIRIM DATA ID KARTU DAN BARCODE BUKU KE ESP
  if (kartu != "" && bar != "") {
    varKirim = kartu + "*" + bar + "?";            // AKHIRI "*" MODE PINJAM BUKU
    if (modepinjamfull == true && modekembalifull == false) {
      Serial.println(varKirim);  Wire.print(varKirim); 
    }
    varKirim = kartu + "!" + bar + "?";            // AKHIRI "!" MODE KEMBALI BUKU
    if (modepinjamfull == false && modekembalifull == true) {
      Serial.println(varKirim);  Wire.print(varKirim); 
    }
  }
}

// FUNGSI TERIMA RESPON DATABASE
void responseDatabase() {
  String data, nama, nim, batas, buku, pengarang, denda;
  while (Wire.available() > 0) { char c = Wire.read(); data += c; }

  // IDENTIFIKASI DATA DARI CHAR AWAL DAN PARSING DATA INTI
  // DATA MODE PINJAM BUKU DAN MODE KEMBALI BUKU
  if (data.substring(0, 1) == "#")  nama      = getValue(data, '#', 1);
  if (data.substring(0, 1) == "$")  nim       = getValue(data, '$', 1);
  if (data.substring(0, 1) == "!")  buku      = getValue(data, '!', 1);
  if (data.substring(0, 1) == "@")  pengarang = getValue(data, '@', 1);
  if (data.substring(0, 1) == "%")  batas     = getValue(data, '%', 1);
  if (data.substring(0, 2) == "Rp") denda     = data;

  // JIKA TIDAK TERDAFTAR DI DATABASE
  if (data == "NO%") { nama = "."; nim = "."; }   // ID KARTU TIDAK TERDAFTAR DI TABLE USER
  if (data == "NO&") { nama = ","; nim = ","; }   // ID KARTU TIDAK TERDAFTAR DI TABLE PINJAMAN
  if (data == "NO@") { buku = "."; buku = "."; }  // BARCODE TIDAK TERDAFTAR DI TABLE BUKU
  if (data == "NO#") { buku = ","; buku = ","; }  // BARCODE TIDAK TERDAFTAR DI TABLE PINJAMAN

  // TAMPILKAN DATA PADA NEXTION
  for (int i=0; i<2; i++) {
    // DATA MODE PINJAM BUKU DAN MODE KEMBALI BUKU
    if (nama != "")     { Serial.print("t2.txt=\"" + nama + "\"");            endNextion(); }
    if (nim != "")      { Serial.print("t3.txt=\"" + nim + "\"");             endNextion(); }
    if (batas != "")    { Serial.print("overview.t8.txt=\"" + batas + "\"");  endNextion(); }
    if (buku != "")     { Serial.print("t0.txt=\"" + buku + "\"");            endNextion(); }
    if (pengarang != ""){ Serial.print("t1.txt=\"" + pengarang + "\"");       endNextion(); }
    
    // DATA MODE KONFIRMASI
    if (denda != "")    { Serial.print("end.t7.txt=\"""DENDA "+data+"\"");          endNextion(); }
    if (data == "NO")   { Serial.print("end.t7.txt=\"""GAGAL TERSIMPAN""\"");       endNextion(); }
    if (data == "YES")  { Serial.print("end.t7.txt=\"""PINJAM BUKU TERSIMPAN""\""); endNextion(); }
    
    // RESET DATA JIKA PROSES SELESAI
    if (denda != "" || data == "NO" || data == "YES") { 
      kartu = "";   bar = "";
      modepinjam = false; modepinjamfull = false; modekembali = false;  modekembalifull = false;
    }
  } 
}
