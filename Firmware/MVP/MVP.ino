//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

#include "libcm.h"

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
	wdt_disable();
	Serial.begin(115200); //USB
	METSCI_begin();
	BATTSCI_begin();
	heater_init();
	LiDisplay_begin();
	lcd_begin();
	LTC68042configure_initialize();

	#ifdef RUN_BRINGUP_TESTER
	  	bringupTester_run(); //this function never returns
	#endif

	if(gpio_keyStateNow() == KEYSTATE_ON){ LED(3,ON); } //turn LED3 on if LiBCM (re)boots while keyON (e.g. while driving)
	
	EEPROM_verifyDataValid();

	Serial.print(F("\n\nLiBCM v" FW_VERSION ", " BUILD_DATE "\n'$HELP' for info\n"));
	debugUSB_printHardwareRevision();
	debugUSB_printConfigParameters();

	wdt_enable(WDTO_2S); //set watchdog reset vector to 2 seconds
}

void loop()
{
	key_stateChangeHandler();
	
	temperature_handler();
	SoC_handler();
	fan_handler();
	heater_handler();
	gridCharger_handler();
	lcdState_handler();

	if( key_getSampledState() == KEYSTATE_ON )
	{
		if(EEPROM_firmwareStatus_get() != FIRMWARE_EXPIRED) { BATTSCI_sendFrames(); } //P1648 occurs if firmware is expired

		LTC68042cell_nextVoltages(); //round-robin handler measures QTY3 cell voltages per call
		METSCI_processLatestFrame();
		adc_updateBatteryCurrent();
		vPackSpoof_setVoltage();
		debugUSB_printLatestData();
	}
	else if( key_getSampledState() == KEYSTATE_OFF )
	{	
		if( time_toUpdate_keyOffValues() == true )
		{ 
			LTC68042cell_sampleGatherAndProcessAllCellVoltages();			
			SoC_updateUsingLatestOpenCircuitVoltage();
			SoC_turnOffLiBCM_ifPackEmpty();
			cellBalance_handler();			
			debugUSB_printLatest_data_gridCharger();
		}
	}

	USB_userInterface_handler();
	wdt_reset(); //Feed watchdog
	blinkLED2(); //Heartbeat
	time_waitForLoopPeriod(); //wait here until next iteration
}