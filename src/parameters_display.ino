#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// LCD pin connections
//LiquidCrystal lcd(7, 6, 5, 4, 3, 11);
#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Sensor pin definitions
const int phSensorPin = A0;
const int mq135SensorPin = A1;
const int tdsSensorPin = A2;
const int conductivitySensorPin = A3;
// const int button1Pin = 8;
// const int button2Pin = 9;
// const int button3Pin = 10;
// const int button4Pin = 12;
// const int button5Pin = 13;

// OneWire pin for DS18B20 temperature sensor
#define ONE_WIRE_BUS 2

//int sensorDisplay = -1;
const float phSlope = -7.8328;
const float intercept = 3.232;  // Adjust this value based on your pH sensor calibration
const int mq135Threshold = 500;  // Adjust based on your threshold for air quality

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// TDS sensor settings
#define VREF 5.0  // Analog reference voltage (volt)
#define SCOUNT 30  // Sample count

int analogBuffer[SCOUNT];  // Store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;  // Water temperature (Celsius), you can also read this from a temperature sensor
float calibrationFactor = 342.0 / 462.0;  // Initial Calibration factor

// Conductivity sensor settings
float conductivityValue = 0;
float conductivityCalibrationFactor = 664.0;  // Update this line with the new calibration factor

void setup() {
  // pinMode(button1Pin, INPUT_PULLUP);
  // pinMode(button2Pin, INPUT_PULLUP);
  // pinMode(button3Pin, INPUT_PULLUP);
  // pinMode(button4Pin, INPUT_PULLUP);
  // pinMode(button5Pin, INPUT_PULLUP);
  //lcd.begin(16, 2);
  //lcd.print("Sensor Display");
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);

  // Start serial communication for debugging
  Serial.begin(9600);
  delay(2000);
  sensors.begin();

  // Initialize the TDS and Conductivity sensor readings array
  for (int i = 0; i < SCOUNT; i++) {
    analogBuffer[i] = 0;
  }
}

void loop() {
  tft.fillScreen(ST7735_BLACK);

   // Read and display sensor values
  float x = displayPh();
  float y = displayMQ135();
  float z = displayDS18B20();
  float a = displayTDS();
  float b = displayConductivity();
  tft.setCursor(0, 0);
  tft.print("pH: ");
  tft.print(x, 2);
  tft.setCursor(0, 20);
  tft.print("MQ135: ");
  tft.print(y);
  tft.print(y > mq135Threshold ? " High" : " Low");
  tft.setCursor(0, 40);
  tft.print("Temp: ");
  tft.print(z, 2);
  tft.print(" C");
  tft.setCursor(0, 60);
  tft.print("TDS: ");
  tft.print(a, 0);
  tft.print(" ppm");
  tft.setCursor(0, 80);
  tft.print("Conductivity: ");
  tft.print(b, 2);
  tft.print(" uS/cm");


  
  delay(3000); // Debounce delay for button press
}

float  displayPh() {
  int phValue = analogRead(phSensorPin);
  float voltage = phValue * (5.0 / 1023.0);
  float ph = 7 - phSlope * (intercept - voltage);  // Calculate pH using the slope
  // tft.setCursor(0, 0);
  // tft.print("pH: ");
  // tft.print(ph, 2);
  return ph ; 
}

float displayMQ135() {
  int mq135Value = analogRead(mq135SensorPin);
  // tft.setCursor(0, 20);
  // tft.print("MQ135: ");
  // tft.print(mq135Value);
  // tft.print(mq135Value > mq135Threshold ? " High" : " Low");
  return mq135Value ;
}

float displayDS18B20() {
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  // tft.setCursor(0, 40);
  // tft.print("Temp: ");
  // tft.print(temperatureC, 2);
  // tft.print(" C");
  return temperatureC ; 
}

float displayTDS() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {  // Every 40 milliseconds, read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(tdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (int i = 0; i < SCOUNT; i++) {
      analogBufferTemp[i] = analogBuffer[i];
    }
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;  // Read the analog value and convert to voltage
    tdsValue = (133.42 * averageVoltage * averageVoltage * averageVoltage - 255.86 * averageVoltage * averageVoltage + 857.39 * averageVoltage) * calibrationFactor / 2; // TDS value in ppm

    
    // tft.setCursor(0, 60);
    // tft.print("TDS: ");
    // tft.print(tdsValue, 0);
    // tft.print(" ppm");
    return tdsValue;
  }
}

float displayConductivity() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {  // Every 40 milliseconds, read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(conductivitySensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (int i = 0; i < SCOUNT; i++) {
      analogBufferTemp[i] = analogBuffer[i];
    }
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;  // Read the analog value and convert to voltage
    conductivityValue = averageVoltage * conductivityCalibrationFactor;

    // tft.setCursor(0, 80);
    // tft.print("Conductivity: ");
    // tft.print(conductivityValue, 2);
    // tft.print(" uS/cm");
    return conductivityValue ;
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) {
    bTab[i] = bArray[i];
  }
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}
