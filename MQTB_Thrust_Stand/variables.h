
// Scale Pins
// HX711.DOUT  - pin #38
// HX711.PD_SCK - pin #19
HX711 scale(38, 19);

//dShot hardware serial
HardwareSerial tlmSerial(1); //dShot serial
  
// RPM input variables
int rpmPins[5][2] = {{0,0},
                     {GPIO_PORTD_BASE, GPIO_PIN_7},
                     {GPIO_PORTF_BASE, GPIO_PIN_0},
                     {GPIO_PORTC_BASE, GPIO_PIN_7},
                     {GPIO_PORTA_BASE, GPIO_PIN_4}
                    };
volatile uint32_t stepCount[] = {0,0,0,0,0};
volatile uint64_t stepTime[] = {0,0,0,0,0};
volatile uint32_t stepDiff[] = {0,0,0,0,0};
Average<int> avgStepDiff[] = { 0,
                              Average<int> (AVGSIZE),
                              Average<int> (AVGSIZE),
                              Average<int> (AVGSIZE),
                              Average<int> (AVGSIZE)};
int theseRpms[5];
void countRPMs(int inputID = 0);

// dShot variables
uint8_t receivedBytes = 0;
volatile bool requestTelemetry = false;
bool printTelemetry = true;
uint16_t dshotUserInputValue = 0;
static uint16_t dshotPacket[16];  //Array to save the Dshot Packet states
bool dShotWriteActive = false;

//dShot telemetry variables

int16_t ESC_telemetry[5]; // Temperature, Voltage, Current, used mAh, eRpM
uint32_t currentTime;
uint8_t temperature = 0;
uint8_t temperatureMax = 0;
float voltage = 0;
float voltageMin = 99;
uint32_t current = 0;
uint32_t currentMax = 0;
uint32_t erpm = 0;
uint32_t erpmMax = 0;
uint32_t rpm = 0;
uint32_t rpmMAX = 0;
uint32_t kv = 0;
uint32_t kvMax = 0;


// analog value variables
uint32_t ulADC0Value[8];
volatile int voltageValue = 0;
volatile int currentValue = 0;
volatile int thrust = 0;

//PWM Output Variables
void updatePWM(unsigned pulseWidth, unsigned pwmOutput = 0);

int pwmOutputs[] = {
                0 ,
                PWM_OUT_2,
                PWM_OUT_4,
                PWM_OUT_6,
                PWM_OUT_7};


// Misc Variables
char character;
String input;
uint64_t startTime;
uint64_t loopStart;
boolean isTestRunning = false;
uint32_t currentMicros = 0;
boolean isTared = false;
uint32_t loopDelay = 4000;

union f_bytes
{
  byte b[4];
  float fval;
}u;

//for average step times
//need more testing to determine optimal number of steps to average
volatile uint32_t step_times[5][STEP_COUNT];
volatile uint32_t step_idx[] = {0,0,0,0,0};

uint16_t pwmMultiplier = 1000;
