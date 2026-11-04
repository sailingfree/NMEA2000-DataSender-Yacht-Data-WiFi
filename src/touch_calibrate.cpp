// The SPIFFS (FLASH filing system) is used to hold touch screen
// calibration data
// read this if it exists and apply it to the supplied tft object

#include "FS.h"

#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <display.h>

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData2"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL true

// Code to run a screen calibration, not needed when calibration values set in
// setup()
void touch_calibrate(TFT_eSPI &tft) {
    uint16_t calData[5];
//    uint8_t calDataOK = 0;

    // check file system
    if (!SPIFFS.begin()) {
        Serial.println("formatting file system");

        SPIFFS.format();
        SPIFFS.begin();
    }

    // Calibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
        f.write((const unsigned char *)calData, 14);
        f.close();
    }

    Serial.println();
    Serial.println();
    Serial.println("// Use this calibration code in setup():");
    Serial.print("  uint16_t calData[5] = ");
    Serial.print("{ ");

    for (uint8_t i = 0; i < 5; i++) {
        Serial.print(calData[i]);
        if (i < 4)
            Serial.print(", ");
    }

    Serial.println(" };");
    Serial.print("  tft.setTouch(calData);");
    Serial.println();
    Serial.println();

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");
    tft.println("Calibration code sent to Serial port.");

    delay(4000);
}

bool touch_calibrate_apply(TFT_eSPI *tft) {
    uint16_t calData[5];
    uint8_t calDataOK = 0;
    bool status = false;

    // check file system exists
    if (!SPIFFS.begin()) {
        Serial.println("formatting file system");
        SPIFFS.format();
        SPIFFS.begin();
    }

    // check if calibration file exists and size is correct
    if (SPIFFS.exists(CALIBRATION_FILE)) {
        File f = SPIFFS.open(CALIBRATION_FILE, "r");
        if (f) {
            if (f.readBytes((char *)calData, 14) == 14)
                calDataOK = 1;
            f.close();
        }
    }

    if (calDataOK && !REPEAT_CAL) {
        // calibration data valid
        tft->setTouch(calData);
        status = true;
    } else {
        // data not valid so just return
        // printing a message on the console
        Serial.printf("Cannot load the calibration data\n");
    }
    return status;
}