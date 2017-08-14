
// Configuration options
#define PART_TM4C123GH6PM  //Set board type to TivaC instead of Stellaris

// Rates
#define UARTBAUD 921600     // UART Baud rate (DO NOT set to less than 115200) 
#define SENSORRATE  250     // Refresh rate in HZ of load cell and analog read timer.
#define LOOPDELAY  4000     // Set loop time of main test sequence (doesn't affect brake test). Set to false for lowest stable looptime based on UARTBAUD

// Sensor and Motor configuration
#define MAGSENS    true     // Using Magnetic RPM sensor?
#define OPTISENS  false     // Using Magnetic RPM sensor?
#define POLES        14     // Number of magnetic poles in test motor.

//RPM calculation configuration
#define USEAVG     true     // Use averaging on RPM calculations
#define AVGSIZE      10     // Sample size of rolling average used in RPM calculations if USEAVG is true.

// Test Range
#define MINTHROTTLE 1030   // Low end of ESC calibrated range
#define MAXTHROTTLE 2000   // High end of ESC calibrated range
#define MINCOMMAND  1000   // Value sent to ESC when test isn't running.
#define IDLEPWM     1100   // PWM sent during the idle routine


// Analog Configuation
#define OVERSAMPLING 64    // Analog oversampling multiplier
#define VSCALE    36.31    // Scale factor for Voltage divider.
#define VOFFSET       0    // Voltage offset value.
#define CSCALE  -0.1078    // Scale factor for current sensor.
#define COFFSET       1    // Current offset value.
#define LSCALE     -429    // Scale factor for load cell amplifier.

// Brake Test Configuration      NOTE: The braking test always runs at the maximum speed allowed by UARTBAUD
#define BRAKEMAXRPM 30000  // Maximum RPM limit used in braking test.
#define BRAKEMINRPM 8000   // Maximum RPM limit used in braking test.
#define BRAKERPMSAMPLE 250 // Sample size of RPM averaging for target RPM detection during brake test.
