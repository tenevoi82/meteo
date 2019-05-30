#include <iarduino_OLED_txt.h>
#include <BME280I2C.h>
#include <Wire.h>

#define SERIAL_BAUD 115200
#define BTN 7
#define RED_PIN 9
#define BLUE_PIN 11
#define GREEN_PIN 10
#define BUZ_PIN 4
#define NORM_TEMP 21.0
#define Q_TEMP 2.0

extern uint8_t MegaNumbers[];
extern uint8_t MediumFontRus[];
extern uint8_t MediumNumbers[];
extern uint8_t SmallFontRus[];

int buzzer = 4;// setting controls the digital IO foot buzzer

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
				  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

iarduino_OLED_txt  myOLED(0x3c);
BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_torr);
float temp(NAN), hum(NAN), pres(NAN);
bool automatic = false;
unsigned long last_time_change_display;

volatile bool btn_press = 0;
volatile bool btn_pressed = false;
volatile bool btn_settings = 0;
volatile unsigned long btpress_time;
ISR(PCINT2_vect) {
	if (!(PIND & (1 << PD7))) { 
		btpress_time = millis();
		btn_pressed = true;
		tone(BUZ_PIN, 1200, 20);
		btn_press = true;
	}
	if ((PIND & (1 << PD7))) {
		if (millis() - btpress_time < 500) {
			btn_pressed = false;
		}
	}
}


void setup()
{
	pinMode(RED_PIN, OUTPUT);
	pinMode(BLUE_PIN, OUTPUT);
	pinMode(GREEN_PIN, OUTPUT);
	Serial.begin(115200);
	Serial.println("Hello Computer");

	pinMode(BTN, INPUT_PULLUP);
	PCMSK2 = 0b10000000;
	PCICR |= 0b00000100;


	myOLED.begin();                                         // Инициируем работу с дисплеем.

	Wire.begin();
	Wire.setClock(400000);


	while (!bme.begin())
	{
		Serial.println("Could not find BME280 sensor!");
		delay(1000);
	}

	// bme.chipID(); // Deprecated. See chipModel().
	switch (bme.chipModel())
	{
	case BME280::ChipModel_BME280:
		Serial.println("Found BME280 sensor! Success.");
		break;
	case BME280::ChipModel_BMP280:
		Serial.println("Found BMP280 sensor! No Humidity available.");
		break;
	default:
		Serial.println("Found UNKNOWN sensor! Error!");
	}
	BME280::Settings set = bme.getSettings();
	set.filter = BME280::Filter::Filter_16;
	set.mode = BME280::Mode::Mode_Normal;
	set.standbyTime = BME280::StandbyTime::StandbyTime_1000ms;
	set.humOSR = BME280::OSR::OSR_X16;
	set.tempOSR = BME280::OSR::OSR_X16;
	set.presOSR = BME280::OSR::OSR_X16;
	bme.setSettings(set);
	myOLED.setFont(MediumFontRus);

	pinMode(BUZ_PIN, OUTPUT);

}

enum curShow {temp_d,hum_d,pres_d};
curShow cs = temp_d;


void showTemp() {
	static unsigned long start = millis();
	if (millis() - start > 500) {
		myOLED.setFont(MegaNumbers);
		myOLED.print(temp, OLED_L, OLED_B, 2);
		myOLED.setFont(MediumFontRus);
		myOLED.print("o", OLED_R, OLED_T + 9);
		myOLED.print(F("Температура"), OLED_C, OLED_T);
		start = millis();
	}
}

void showHum() {
	static unsigned long start = millis();
	if (millis() - start > 500) {
		myOLED.setFont(MediumFontRus);
		myOLED.print(F("Влажность"), OLED_C, OLED_T);
		myOLED.setFont(MegaNumbers);
		myOLED.print(hum, OLED_L, OLED_B, 1);
		myOLED.setFont(MediumFontRus);
		myOLED.print("%");
		start = millis();
	}
}
void showAuto() {
	myOLED.clrScr();
	myOLED.setFont(MediumFontRus);
	myOLED.print("Настройки", OLED_C, OLED_T);
	myOLED.print("Авт.перекл", OLED_C, OLED_C);
	if(automatic)myOLED.print("Включено", OLED_C, OLED_B);
	        else myOLED.print("Выключено", OLED_C, OLED_B);
}

void showPress() {
	static unsigned long start = millis();
	if (millis() - start > 500) {
		myOLED.setFont(MediumFontRus);
		myOLED.print(F("Давление"), OLED_C, OLED_T);
		myOLED.setFont(MegaNumbers);
		myOLED.print(pres, OLED_L, OLED_B, 2);
		start = millis();
	}
}

void settings_sound() {
	if (automatic) {
		tone(BUZ_PIN, 1500, 100);
		delay(125);
		tone(BUZ_PIN, 1500, 100);
		delay(125);
		tone(BUZ_PIN, 1500, 100);
		delay(125);
		tone(BUZ_PIN, 1500, 100);
		delay(125);

	}
	else {
		//tone(buzzer, 1200, 100);
		//delay(100);
		tone(BUZ_PIN, 0, 100);
		delay(100);
		tone(BUZ_PIN, 1400, 100);
		delay(100);
		tone(BUZ_PIN, 1200, 200);
		delay(200);

	}
}

void change_display() {
		switch (cs)
		{
		case temp_d:
			cs = hum_d;
			break;
		case hum_d:
			cs = pres_d;
			break;
		case pres_d:
			cs = temp_d;
			break;
		default:
			break;
		}
		last_time_change_display = millis();
		myOLED.clrScr();
}


void ledwork() {
	static unsigned long last_led_time = millis();
	if (millis() - last_led_time < 500)
		return;
	else
		last_led_time = millis();
	if (abs(NORM_TEMP - temp) <= Q_TEMP) {
		float quality = abs(NORM_TEMP - temp);
		int greenval = map(quality * 100, 0, Q_TEMP*100, 50, 0);
		analogWrite(GREEN_PIN, greenval);
		//Serial.print("Green=");
		//Serial.println(greenval);

	}
	else {
		analogWrite(GREEN_PIN, 0);
	}
	if (temp > NORM_TEMP + Q_TEMP/2)
	{
		int redval = map(temp*100, NORM_TEMP*100, (NORM_TEMP + 10)*100, 0, 128);
		redval = (redval > 255) ? 255 : redval;
		analogWrite(RED_PIN, redval);
		//Serial.print("RED=");
		//Serial.println(redval);
	}
	else {
		analogWrite(RED_PIN, 0);
	}
	if (temp < NORM_TEMP - Q_TEMP/2) {
		int blueval = map(temp*100, NORM_TEMP*100, (NORM_TEMP - 10)*100, 0, 128);
		blueval = (blueval > 255) ? 255 : blueval;
		analogWrite(BLUE_PIN, blueval);
		//Serial.print("BLUE=");
		//Serial.println(blueval);
	}
	else {
		analogWrite(BLUE_PIN, 0);
	}
}

void loop()
{
	if (btn_pressed == true && millis() - btpress_time > 1000) {
		btn_pressed = false;
		automatic = !automatic;
		showAuto();
		settings_sound();
		delay(1000);
		myOLED.clrScr();
	}
		
	bme.read(pres, temp, hum, tempUnit, presUnit);
	ledwork();
	if (btn_press) {
		btn_press = false;
		change_display();
	};
	if (automatic) {
		if (millis() - last_time_change_display > 5000) {
			change_display();
		}
	}
	switch (cs)
	{
	case temp_d:
		showTemp();
		break;
	case hum_d:
		showHum();
		break;
	case pres_d:
		showPress();
		break;
	default:
		break;
	}
	
}