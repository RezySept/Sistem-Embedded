#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Sesuaikan alamat I2C jika perlu
const int trigPin = 6;
const int echoPin = 5;
Servo myServo; // Deklarasi objek servo
const int servoPin = 3; // Pin servo

#define SensorPin A0      // pH meter Analog output to Arduino Analog Input 0
#define RedLED 11         // Pin untuk LED Merah
#define YellowLED 9     // Pin untuk LED Kuning
#define BlueLED 10         // Pin untuk LED Biru

// Kalibrasi pH
float PH4 = 3.594;
float PH7 = 3.271;
float PH_step;
float Po; // Nilai pH yang dihitung
int nilai_analog_PH;
double TeganganPh;

unsigned long int avgValue;  // Menyimpan nilai rata-rata dari sensor feedback
int buf[10], temp;

const int relayPin = 7;  // Pin untuk Relay Pompa

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Cek Ketinggian");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  myServo.attach(servoPin); // Hubungkan servo ke pin servo
  myServo.write(0); // Posisi awal servo di 0 derajat (keran tertutup)

  pinMode(SensorPin, INPUT);
  
  // Inisialisasi LED pin sebagai output
  pinMode(RedLED, OUTPUT);
  pinMode(YellowLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(relayPin, OUTPUT);  // Inisialisasi relay pompa

  delay(2000); // Tunggu beberapa detik untuk Serial Monitor terbuka
}

void loop() {
  // Kirim sinyal ultrasonik
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Baca durasi pantulan
  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout 30 ms
  float distance = (duration == 0) ? -1 : duration * 0.034 / 2;

  // Tampilkan hasil di Serial Monitor
  Serial.print("Jarak Sensor Ultrasonik: ");
  if (distance < 0) {
    Serial.println("Out of Range");
  } else {
    Serial.print(distance);
    Serial.println(" cm");
  }

  // Tampilkan hasil di LCD
  lcd.setCursor(0, 0);
  lcd.print("Jarak: ");
  if (distance < 0 || distance < 1 || distance > 30) { // Hanya tampilkan jarak antara 1 dan 30 cm
    lcd.print("Out of Range   ");
  } else {
    lcd.print(distance);
    lcd.print(" cm    ");
  }

  // Kontrol servo berdasarkan ketinggian air
  if (distance <= 13.80) { // Jika jarak 10 cm atau kurang, buka keran (servo 90 derajat)
    lcd.setCursor(0, 1);
    lcd.print("Keran Terbuka   ");
    myServo.write(90); // Buka keran
  } else if (distance >= 18.50) { // Jika jarak 18.50 cm, tutup keran (servo 0 derajat)
    lcd.setCursor(0, 1);
    lcd.print("Keran Tertutup  ");
    myServo.write(0); // Tutup keran
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Proses...       ");
  }

  // Membaca nilai pH
  for (int i = 0; i < 10; i++) {
    buf[i] = analogRead(SensorPin);
    delay(10);
  }

  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  avgValue = 0;
  for (int i = 2; i < 8; i++) avgValue += buf[i];

  // Kalkulasi tegangan dan pH dengan kalibrasi
  nilai_analog_PH = avgValue / 6; // Ambil nilai rata-rata
  TeganganPh = 5.0 / 1024.0 * nilai_analog_PH;
  
  // Kalkulasi pH berdasarkan kalibrasi dua titik
  PH_step = (PH4 - PH7) / 3;
  Po = 7.00 + ((PH7 - TeganganPh) / PH_step);

  // Tampilkan nilai pH di Serial Monitor
  Serial.print("Nilai PH Air: ");
  Serial.print(Po);
  Serial.println("  ");

  // Tampilkan Nilai pH di LCD
  lcd.setCursor(0, 1);
  lcd.print("pH: ");
  lcd.print(Po, 2); // Menampilkan pH dengan dua angka desimal

  // Menentukan Status pH
  String statusPH;
  if (Po < 4) {
    statusPH = "Very Acidic";
    digitalWrite(YellowLED, HIGH); // LED Kuning hidup
    digitalWrite(relayPin, LOW); //Pompa mati (relay tidak aktif)
  } else if (Po >= 4 && Po < 6.5) {
    statusPH = "Acidic";
    digitalWrite(YellowLED, HIGH); // LED Kuning hidup
    digitalWrite(relayPin, LOW); // Pompa mati (relay tidak aktif)
  } else if (Po >= 6.5 && Po <= 8.5) {
    statusPH = "Neutral";  
    digitalWrite(BlueLED, HIGH); // LED Biru hidup
    digitalWrite(relayPin, HIGH); // Pompa aktif (relay aktif)
  } else if (Po > 8.5) {
    statusPH = "Alkaline";
    digitalWrite(RedLED, HIGH); // LED Merah hidup
    digitalWrite(relayPin, LOW); // Pompa mati (relay tidak aktif)
  }

  // Menampilkan Status PH di Serial Monitor
  Serial.print("Status PH Air: ");
  Serial.println(statusPH);

  // Menampilkan Status PH di LCD
  lcd.setCursor(0, 1);
  lcd.print("Status: ");
  lcd.print(statusPH.substring(0, 10)); // Mencegah teks yang panjang menabrak LCD

  delay(1000); // Tunggu 1 detik sebelum membaca nilai berikutnya
}
