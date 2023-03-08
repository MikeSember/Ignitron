

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <string>
#include <NimBLEDevice.h> // github NimBLE

#include "src/SparkButtonHandler.h"
#include "src/SparkDataControl.h"
#include "src/SparkDisplayControl.h"
#include "src/SparkLEDControl.h"

// Device Info Definitions
const std::string DEVICE_NAME = "Ignitron";
const std::string VERSION = "1.0";

// Control classes
SparkDataControl spark_dc;
SparkButtonHandler *spark_bh;
SparkLEDControl *spark_led;
SparkDisplayControl *spark_display;

// Check for initial boot
bool isInitBoot;
int operationMode = SPARK_MODE_APP;


/////////////////////////////////////////////////////////
//
// INIT AND RUN
//
/////////////////////////////////////////////////////////


void setup() {

	isInitBoot = true;
	Serial.begin(115200);
	while (!Serial)
		;

	Serial.println("Initializing");
	//spark_dc = new SparkDataControl();
	spark_bh = new SparkButtonHandler(&spark_dc);
	operationMode = spark_bh->checkBootOperationMode();

	// Setting operation mode before initializing
	operationMode = spark_dc.init(operationMode);
	spark_bh->configureButtons();
	Serial.printf("Operation mode: %d", operationMode);

	if (operationMode == SPARK_MODE_APP) {
		Serial.println("======= Entering APP mode =======");
	} else if (operationMode == SPARK_MODE_LOOPER) {
		Serial.println("======= Entering Looper mode =======");
	} else if (operationMode == SPARK_MODE_AMP) {
		Serial.println("======= Entering AMP mode =======");
	}


	spark_display = new SparkDisplayControl(&spark_dc);
	spark_dc.setDisplayControl(spark_display);
	spark_display->init(operationMode);
	// Assigning data control to buttons;
	spark_bh->setDataControl(&spark_dc);
	// Initializing control classes
	spark_led = new SparkLEDControl(&spark_dc);

	Serial.println("Initialization done.");

}

void loop() {

	// Methods to call only in APP mode
	if (operationMode == SPARK_MODE_APP || operationMode == SPARK_MODE_LOOPER) {
		while (!(spark_dc.checkBLEConnection())) {
			spark_display->update(isInitBoot);
			spark_bh->readButtons();
		}

		//After connection is established, continue.
		// On first boot, set the preset to Hardware setting 1.
		if (isInitBoot == true) {
			DEBUG_PRINTLN("Initial boot, setting preset to HW 1");
			spark_dc.switchPreset(1, true);
			if (!(spark_dc.activePreset()->isEmpty)) {
				isInitBoot = false;
			} else {
				delay(2000);
			}

		}

	}

	// Check if presets have been updated
	spark_dc.checkForUpdates();
	// Reading button input
	spark_bh->readButtons();
	// Update LED status
	spark_led->updateLEDs();
	// Update display
	spark_display->update();

}
