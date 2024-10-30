// Idle loop and CPU usage
// This gives a crude measure of how busy the CPU is.
// I use this to get an idea if any of the handlers is taking up too much time.
// A better way would be to use the low level callbacks but those aren't enabled
// in the PlatformIO/ESP version currently in use.

/*
Copyright (c) 2022 Peter Martin www.naiadhome.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Arduino.h>
#include <Idle.h>

static bool doingCalib = false;
static const int NSAMPLES = 100;
static uint64_t ops[NSAMPLES][2];
static int cpuAvg[2];
static uint64_t calib[2];

TaskHandle_t idleTaskHandle;
// Task handles for the two idle loops
TaskHandle_t Idle0, Idle1;

// Timer handles
TimerHandle_t loadTimer;

// Idle loop to measure CPU usage
// The parameter is the core to measure.
// The task is pinned to a core
void idleLoop(void *parameter) {
    int core = (int)parameter;
    int sample = 0;

    disableCore0WDT();
    disableCore1WDT();

    while (true) {
        ops[sample][core] = 0;
        uint64_t now = esp_timer_get_time();
        uint64_t end;

        do {
            ops[sample][core]++;
        } while ((end = esp_timer_get_time()) - now < 100 * 1000);
        sample = (sample + 1) % NSAMPLES;
    }
}

// Load timer
void loadTimerFunc(TimerHandle_t xTimer) {
    uint64_t total[2] = {0, 0};
    uint64_t avg;
    int i;
    int c;
    for (c = 0; c < 2; c++) {
        for (i = 0; i < NSAMPLES; i++) {
            total[c] += ops[i][c];
        }
        avg = total[c] / (uint64_t)NSAMPLES;
        cpuAvg[c] = (calib[0] - avg) * 100 / calib[0];
        // Console->printf("CPU Usage for core %d %lld %d%%\n", c, avg, cpuAvg[c]);
    }
    return;
}

int getCpuAvg(int core) {
    if (core >= 0 && core <= 1) {
        return cpuAvg[core];
    }
    return 0;
}

void IdleInit() {
    // Create idle loop tasks for CPU usage monitoring
    // Create task for core 0, loop() runs on core 1
    xTaskCreatePinnedToCore(
        idleLoop, /* Function to implement the task */
        "Idle0",  /* Name of the task */
        10000,    /* Stack size in words */
        0,        /* Task input parameter - the core to measure */
        0,        /* Priority of the task */
        &Idle0,   /* Task handle. */
        0);       /* Core where the task should run */

    // Create task for core 0, loop() runs on core 1
    xTaskCreatePinnedToCore(
        idleLoop,  /* Function to implement the task */
        "Idle1",   /* Name of the task */
        10000,     /* Stack size in words */
        (void *)1, /* Task input parameter - the core to measure */
        1,         /* Priority of the task */
        &Idle1,    /* Task handle. */
        1);        /* Core where the task should run */

    // Timer for cpu load measurements
    TimerHandle_t loadTimer = xTimerCreate("LoadTimer",
                                           pdMS_TO_TICKS(10000),  // msecs to ticks
                                           true,
                                           (void *)0,
                                           loadTimerFunc);

    xTimerStart(loadTimer, pdMS_TO_TICKS(2000));
}

// Loop to see how many loops are executed in 100msecs.
// This is the same code used in the idle loops later

void calibrateCpu() {
    uint64_t now = esp_timer_get_time();
    uint64_t end;

    do {
        calib[0]++;
    } while ((end = esp_timer_get_time()) - now < 100 * 1000);
    Console->printf("Calib loop count %lld in %lld usecs\n", calib[0], end - now);
}
