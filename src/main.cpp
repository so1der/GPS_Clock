#include <Arduino.h>
#include <BigNums2x2.h>
#include <LiquidCrystal.h>
#include <STM32RTC.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <TinyGPS++.h>

#define PWM_Pin PA6
#define LCD_RS  PA0
#define LCD_EN  PA1
#define LCD_D4  PA2
#define LCD_D5  PA3
#define LCD_D6  PA4
#define LCD_D7  PA5

TimeChangeRule uaDST = {"EEST", Last, Sun, Mar, 3, 180}; // EEST = UTC+3 = 180 minutes
TimeChangeRule uaSTD = {"EET", Last, Sun, Oct, 4, 120}; // EET = UTC+2 = 120 minutes

Timezone uaZone(uaDST, uaSTD);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
BigNums2x2 screen(NASA);
HardwareSerial SerialGPS(USART1);
TinyGPSPlus gps;
STM32RTC& rtc = STM32RTC::getInstance();

time_t utc_time, local_time;
tmElements_t tm;

const char days_of_the_week[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const char time_div[3] = "o ";
uint32_t last_update = 0;
const uint32_t update_interval = 500;

// ============ User Settings =============
const uint8_t dim_start_hour = 22; // The hour when the nighttime begins (backlights dimms)
const uint8_t dim_end_hour = 6; // The hour when the nighttime ends  (backlights brighten)
const uint8_t day_dim = 255; // Daytime backlight brightness   (0 - minimum, 255 - maximum)
const uint8_t night_dim = 20; // Nighttime backlight brightness (0 - minimum, 255 - maximum)
const uint8_t min_sats_to_calibrate_time = 6; // Minimum number of satellites for time calibration
// ========================================

void print_div(uint8_t n) {
    for (uint8_t y = 1; y <= 2; y++) {
        for (uint8_t x = 7; x <= 12; x += 5) {
            lcd.setCursor(x, y);
            lcd.print(time_div[n]);
        }
    }
}

void check_dim() {
    if (hour(local_time) <= dim_end_hour || hour(local_time) >= dim_start_hour) {
        analogWrite(PWM_Pin, night_dim);
    } else {
        analogWrite(PWM_Pin, day_dim);
    }
}

void correct_time() {
    if (gps.time.isValid() && gps.time.isUpdated() && gps.date.isValid() && gps.date.isUpdated() &&
        gps.satellites.value() > min_sats_to_calibrate_time && abs(gps.time.second() - rtc.getSeconds()) > 5) {
        rtc.setTime(gps.time.hour(), gps.time.minute(), gps.time.second());
        rtc.setDate(gps.date.day(), gps.date.month(), gps.date.year() - 2000);
    }
}

void update_local_time() {
    tm.Second = rtc.getSeconds();
    tm.Minute = rtc.getMinutes();
    tm.Hour = rtc.getHours();
    tm.Day = rtc.getDay();
    tm.Month = rtc.getMonth();
    tm.Year = rtc.getYear() + 30;
    utc_time = makeTime(tm);
    local_time = uaZone.toLocal(utc_time);
}

void draw_time() {
    screen.print(hour(local_time) % 10, 1, 5, 0);
    screen.print(hour(local_time) / 10 % 10, 1, 3, 0);

    screen.print(minute(local_time) % 10, 1, 10, 0);
    screen.print(minute(local_time) / 10 % 10, 1, 8, 0);

    screen.print(second(local_time) % 10, 1, 15, 0);
    screen.print(second(local_time) / 10 % 10, 1, 13, 0);
}

void draw_date() {
    lcd.setCursor(0, 3);
    char date_buf[21];
    snprintf(
        date_buf,
        sizeof(date_buf),
        "%-10s%02d.%02d.%4d",
        days_of_the_week[weekday(local_time)],
        day(local_time),
        month(local_time),
        year(local_time)
    );
    lcd.print(date_buf);
}

void draw_sats_count() {
    lcd.setCursor(0, 0);
    char sat_buf[25];
    snprintf(sat_buf, sizeof(sat_buf), "Satellites count: %02d", gps.satellites.value());
    lcd.print(sat_buf);
}

void setup() {
    pinMode(PWM_Pin, OUTPUT);
    analogWrite(PWM_Pin, day_dim);
    lcd.begin(20, 4);
    lcd.print("  Starting GPS...   ");
    SerialGPS.begin(9600);
    delay(1000);
    lcd.clear();
    lcd.print("  Checking RTC...   ");
    rtc.setClockSource(STM32RTC::LSE_CLOCK);
    rtc.begin();
    if (!rtc.isTimeSet() || rtc.getYear() < 25) {
        lcd.clear();
        lcd.print("  Time is not set.  ");
        lcd.setCursor(0, 1);
        lcd.print("    Waiting for    ");
        lcd.setCursor(0, 2);
        lcd.print("    satellites!    ");
        delay(500);
        while (!gps.time.isUpdated() || !gps.date.isValid() || gps.satellites.value() < min_sats_to_calibrate_time) {
            while (SerialGPS.available()) {
                gps.encode(SerialGPS.read());
            }
            lcd.setCursor(0, 3);
            char buf3[25];
            snprintf(buf3, sizeof(buf3), "Satellites count: %02d", gps.satellites.value());
            lcd.print(buf3);
        }
        delay(500);
        rtc.setTime(gps.time.hour(), gps.time.minute(), gps.time.second());
        rtc.setDate(gps.date.day(), gps.date.month(), gps.date.year() - 2000);
    }
    lcd.clear();
}

void loop() {
    while (SerialGPS.available()) {
        gps.encode(SerialGPS.read());
    }

    if (rtc.getSeconds() != second(local_time)) {
        update_local_time();
        print_div(0);
        draw_time();
        draw_date();
        draw_sats_count();
        last_update = millis();
    }

    if (millis() - last_update > update_interval) {
        print_div(1);
    }

    check_dim();
    correct_time();
}
