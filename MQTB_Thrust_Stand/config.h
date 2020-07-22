
// Configuration options
#define PART_TM4C123GH6PM        // Set board type to TivaC instead of Stellaris
#define USE_LOAD_CELL     true   // Set load cell availability
#define USE_MOTORS           1   // Number of motors to use in test
#define RUN_RPM_TEST      true   // Run RPM test

// Rates
#define UARTBAUD 2000000     // UART Baud rate (DO NOT set to less than 115200)
#define SENSORRATE   250     // Refresh rate in HZ of load cell and analog read timer.
#define ESCRATE     8000     // PWM update rate for ESC in HZ
#define ESCOUTPUT      4     // Type of PWM signal: 0 - Standard PWM 1 - Oneshot125, 2 - Oneshot42, 3 - MultiShot, 4 - dShot

// Sensor and Motor configuration
#define MOTOR_POLES        14     // Number of magnetic poles in test motor.

//RPM calculation configuration
#define USEAVG     true     // Use averaging on RPM calculations
#define AVGSIZE      10     // Sample size of rolling average used in RPM calculations if USEAVG is true.
#define STEP_COUNT 20 //number of steps to average for RPM calculation

// Test Range
#define MINTHROTTLE 1030   // Low end of ESC calibrated range
#define MAXTHROTTLE 2000   // High end of ESC calibrated range
#define MINCOMMAND  900   // Value sent to ESC when test isn't running.
#define IDLEPWM     1100   // PWM sent during the idle routine


// Analog Configuation
#define OVERSAMPLING 64    // Analog oversampling multiplier

#define VSCALE  36.42        // Scale factor for Unit 2 Voltage divider.  36.47 0.008890511
#define VOFFSET 0.032248175  // Offset value for Voltage divider.
#define CSCALE  -0.131         // Scale factor (in mV per Amp) for Unit 2 current sensor. -0.130701754 -0.1314375
#define COFFSET -1.7            // Offset value for current sensor. 266.0359649 268.180375
#define LSCALE  391          // Scale factor for load cell amplifier.

// Brake Test Configuration      NOTE: The braking test always runs at the maximum speed allowed by UARTBAUD
#define BRAKEMAXRPM 30000  // Maximum RPM limit used in braking test.
#define BRAKEMINRPM 8000   // Maximum RPM limit used in braking test.
#define BRAKERPMSAMPLE 250 // Sample size of RPM averaging for target RPM detection during brake test.
