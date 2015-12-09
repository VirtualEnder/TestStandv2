
// Configuration options

// Rates
#define UARTBAUD 921600    // UART Baud rate (DO NOT set to less than 115200) 
#define SENSORRATE 500     // Refresh rate in HZ of load cell and analog read timer.
#define LOOPDELAY 4000     // Set loop time of main test sequence (doesn't affect brake test). Set to false for lowest stable looptime based on UARTBAUD

// Sensor and Motor configuration
#define MAGSENS true       // Using Magnetic RPM sensor?
#define OPTISENS false     // Using Magnetic RPM sensor?
#define POLES 14           // Number of magnetic poles in test motor.

// Test Range
#define MINTHROTTLE 1030   // Low end of ESC calibrated range
#define MAXTHROTTLE 2000   // High end of ESC calibrated range
#define MINCOMMAND  980    // Value sent to ESC when test isn't running.
#define IDLEPWM     1100   // PWM sent during the idle routine


// Analog Configuation
#define OVERSAMPLING 64    // Analog oversampling multiplier
#define VSCALE 26          // Scale factor for Voltage divider.
#define CSCALE 100         // Scale factor for current sensor.
#define LSCALE -395        // Scale factor for load cell amplifier.

// Brake Test Configuration      NOTE: The braking test always runs at the maximum speed allowed by UARTBAUD
#define BRAKEMAXRPM 30000  // Maximum RPM limit used in braking test.
#define BRAKEMINRPM 8000   // Maximum RPM limit used in braking test.
#define BRAKERPMSAMPLE 250 // Sample size of RPM averaging for target RPM detection during brake test.