# GPS_Clock

So this is a simple clock that uses a GPS module NEO-6M to obtain precise time. It also uses 20x4 Char LCD screen to display time, date, and quantity of GPS satellites. As an MCU it uses STM32 Blue Pill - STM32F103C8T6. 

Features:

- Auto time sync when time error is above 5 sec.
- Night backlight brightness adjustment 
- Cool big numbers on char display
- Auto timezone selection

I've chosen this MCU because it has its own RTC inside. Indeed, NEO-6M has its own too, but I've found it kinda unstable when there are no satellites visible (which may be a default case, since the clock is placed indoors, and uses a simple passive antenna). So, I thought it was a nice reason to try out the STM32 MCU series, which has an RTC in almost all of its MCUs. Moreover, they sometimes cost even less than Arduino ones, which will also need a dedicated RTC module to count time precisely.

## Schematic 

The connections should be as follows:

Since it takes precise time from satellites, and automatically chooses the correct timezone/summer time, it doesn't need any buttons.

## Code

In the `main.cpp` file there is a block of code, where you can change various things:

```cpp
// ============ User Settings =============
const uint8_t dim_start_hour = 22; // The hour when the nighttime begins (backlights dimms)
const uint8_t dim_end_hour = 6; // The hour when the nighttime ends  (backlights brighten)
const uint8_t day_dim = 255; // Daytime backlight brightness   (0 - minimum, 255 - maximum)
const uint8_t night_dim = 20; // Nighttime backlight brightness (0 - minimum, 255 - maximum)
const uint8_t min_sats_to_calibrate_time = 6; // Minimum number of satellites for time calibration
// ========================================
```
    
There is also a timezone definition using a library called `Timezone`. You need to define your time zones because satellites return time in UTC. For example, I defined two time zones for Ukraine, when they will start,  which offset they use, etc:

```cpp
TimeChangeRule uaDST = {"EEST", Last, Sun, Mar, 3, 180}; // EEST = UTC+3 = 180 minutes
TimeChangeRule uaSTD = {"EET", Last, Sun, Oct, 4, 120}; // EET = UTC+2 = 120 minutes

Timezone uaZone(uaDST, uaSTD);
``` 

## Compile

To compile this project, you'll need PlatformIO. Just open this git repo as a PlatformIO project, compile it, and upload. There are also different STM32 chips on the Blue Pill board, STM32F103**C6**T8 and STM32F103**C8**T6. By default, STM32F103**C8**T6 is defined in the `platformio.ini` file, but if you have an STM32F103**C6**T8, you'll need to change `board` in it:

STM32F103**C8**T6:

```ini
board = bluepill_f103c8
```
STM32F103**C6**T8:

```ini
board = bluepill_f103c6
```
