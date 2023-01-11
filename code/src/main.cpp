#include "main.h"

// float photo_voltage = 0.0;
// float led_voltage = 0.0;
// float power_voltage = 0.0;
// float led_power_voltage = 0.0;



bool led_enabled = false;
bool led_status_change = false;

DS3231  rtc(SDA, SCL);
Adafruit_ADS1115 ads1115;

#define fadePin 5
#define LP_MULTIPLER 11.117117117

void setup()
{
    Serial.begin(9600);
    analogReference(INTERNAL);
    pinMode(fadePin, OUTPUT);
    // pinMode(A1, INPUT);

    // Initialize the rtc object
    rtc.begin();

    // The following lines can be uncommented to set the date and time
    // rtc.setDOW(FRIDAY);     // Set Day-of-Week to SUNDAY
    // rtc.setTime(16, 9, 5);     // Set the time to 12:00:00 (24hr format)
    // rtc.setDate(1, 9, 2017);   // Set the date to January 1st, 2014

    ads1115.begin();

    // Serial.println("LoRa Sender");
    setup_lora();
    delay(1000);
    // pinMode(2, OUTPUT);
    // analogWrite(fadePin, 255);
    // analogWrite(2, 255);
    // analogWrite(3, 255);
}


void loop()
{
    Serial.print(rtc.getDateStr());
    Serial.print(" -- ");
    Serial.println(rtc.getTimeStr());
    get_voltage();

    delay(600000);
    // LowPower.powerDown(SLEEP_, ADC_OFF, BOD_OFF);
}

void get_voltage()
{
    uint8_t msg[100];
    int16_t adc0, adc1, adc2, adc3;
    adc0 = ads1115.readADC_SingleEnded(0);
    adc1 = ads1115.readADC_SingleEnded(1);
    adc2 = ads1115.readADC_SingleEnded(2);
    adc3 = ads1115.readADC_SingleEnded(3);
    // photo_voltage = (adc0 * 0.1875)/1000;
    // power_voltage = (adc1 * 0.1875)/1000;
    // led_voltage = (adc2 * 0.1875)/1000;
    // led_power_voltage = (adc3 * 0.1875)/1000;
    // Serial.print("PHOTORESISTOR: "); Serial.print(photo_voltage); Serial.println(" V");
    // Serial.print("POWER_SOURCE: "); Serial.print(power_voltage); Serial.println(" V");
    // Serial.print("LED_VOLTAGE: "); Serial.print(led_voltage); Serial.println(" V");
    // Serial.print("LED_BATERRY: "); Serial.print(led_power_voltage * 11.117117117); Serial.println(" V");

    char str_pr[6];
    char str_ps[6];
    char str_lv[6];
    char str_lb[6];
    float pr_v = (adc0 * 0.1875)/1000;

    /* 4 is mininum width, 2 is precision; float value is copied*/
    dtostrf(pr_v, 4, 2, str_pr);
    dtostrf((adc1 * 0.1875)/1000, 4, 2, str_ps);
    dtostrf((adc2 * 0.1875)/1000, 4, 2, str_lv);
    dtostrf((adc3 * 0.1875)/1000 * LP_MULTIPLER, 4, 2, str_lb);

    if (pr_v < 1.0 && !led_enabled) {
        led_status_change = true;
        led_enabled = true;
        analogWrite(fadePin, 255);
    }

    if (pr_v > 1.0 && led_enabled) {
        led_status_change = true;
        led_enabled = false;
        analogWrite(fadePin, 0);
    }

    if (led_status_change) {
        // sprintf((char*)msg, "TS: %ld, PR: %s, PS: %s, LV: %s, LB: %s, LED_%s", rtc.getUnixTime(rtc.getTime()), str_pr, str_ps, str_lv, str_lb, led_enabled ? "ENABLED" : "DISABLED");
        sprintf((char*)msg, "TS: %ld, PR: %s, PS: %s, LB: %s, LED_%s", rtc.getUnixTime(rtc.getTime()), str_pr, str_ps, str_lb, led_enabled ? "ENABLED" : "DISABLED");
        led_status_change = false;
    } else {
        sprintf((char*)msg, "TS: %ld, PR: %s, PS: %s, LB: %s", rtc.getUnixTime(rtc.getTime()), str_pr, str_ps, str_lb);
        // sprintf((char*)msg, "TS: %ld, PR: %s, PS: %s, LV: %s, LB: %s", rtc.getUnixTime(rtc.getTime()), str_pr, str_ps, str_lv, str_lb);
    }

    Serial.println((char*) msg);

    send_message(msg);
}
