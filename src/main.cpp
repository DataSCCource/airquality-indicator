#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include "bsec.h"


#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128 // OLED display Breite, in pixels
#define SCREEN_HEIGHT 64 // OLED display Höhe, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
Bsec iaqSensor;

float voc, eco2, iaq, siaq;

int measuredValues[128];
unsigned long lastUpdate = 0;
unsigned long updateRate = 60 * 1000; // in ms

// method signatures
void checkIaqSensorStatus(void);
void addValue(int value);
void printValues(void);
void printGraph(void);
int mapIaqToGraph(int inVal);

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // initialize display
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    delay(500);

    // initialize BME680 sensor
    iaqSensor.begin(BME680_I2C_ADDR_PRIMARY, Wire);
    checkIaqSensorStatus();

    bsec_virtual_sensor_t sensorList[4] = {
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ
    };
    iaqSensor.updateSubscription(sensorList, 4, BSEC_SAMPLE_RATE_ULP);
    checkIaqSensorStatus();

    bsec_virtual_sensor_t sensorListLP[2] = {
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_TEMPERATURE,
    };
    iaqSensor.updateSubscription(sensorListLP, 2, BSEC_SAMPLE_RATE_LP);
    checkIaqSensorStatus();


    delay(500);
}

void loop() {
    if (iaqSensor.run()) { // If new data is available
        // read values
        voc = iaqSensor.breathVocEquivalent;
        eco2 = iaqSensor.co2Equivalent;
        iaq = iaqSensor.iaq;
        siaq = iaqSensor.staticIaq;


        // only add value to graph every "updateRate" ms
        //if(millis() - lastUpdate > updateRate) {
            addValue(siaq);
        //    lastUpdate = millis();
        //}

        display.clearDisplay();
        printValues();
        printGraph();

        display.display(); 
    } else {
        checkIaqSensorStatus();
    }
}

// print values as text to the top two rows of the screen
void printValues() {
    int bottomFirstRow = 12;
    int bottomSecondRow = 30;
    int horizontalCenter = display.width()/2;

    display.setFont(&FreeSans9pt7b);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setTextWrap(false);

    // first row
    display.setCursor(0, bottomFirstRow);
    display.println(iaq);
    display.setCursor(horizontalCenter, bottomFirstRow);
    display.println(siaq);

    // second row
    display.setCursor(0, bottomSecondRow);
    display.println(eco2, 1);
    display.setCursor(horizontalCenter + (voc<=10?10:0), bottomSecondRow);
    display.println(voc);
}

// print the last 128 measured values to a graph
void printGraph() {
    // "coordinate system"
    //display.drawLine(0,display.height()/2, 0,display.height()-1, WHITE);
    //display.drawLine(0,display.height()-1, display.width()-1,display.height()-1, WHITE);

    for(int i=0; i<display.width(); i++) {
        display.drawPixel(i, display.height() - mapIaqToGraph(measuredValues[i]), WHITE);
    }
}

// map max IAQ-Level to graph (assuming it will not surpass 400)
int mapIaqToGraph(int inVal) {
    return (inVal * display.height())/400;
}

// shift measuredValues-Array by dropping the first value
// and add value to last position
void addValue(int value) {
    memcpy(measuredValues, &measuredValues[1], sizeof(measuredValues) - sizeof(int));
    measuredValues[127] = value;
}

// for debugging
void checkIaqSensorStatus(void)
{
    // general sensor status
    if (iaqSensor.status != BSEC_OK) {
        if (iaqSensor.status < BSEC_OK) {
            Serial.println("BSEC error code : " + String(iaqSensor.status));
        } else {
            Serial.println("BSEC warning code : " + String(iaqSensor.status));
        }
    }

    // bme680 status
    if (iaqSensor.bme680Status != BME680_OK) {
        if (iaqSensor.bme680Status < BME680_OK) {
            Serial.println("BME680 error code : " + String(iaqSensor.bme680Status));
        } else {
            Serial.println("BME680 warning code : " + String(iaqSensor.bme680Status));
        }
    }
}
