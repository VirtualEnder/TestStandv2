
void receiveTelemtry(){
    static uint8_t SerialBuf[10];

        if(MySerial.available()){
            SerialBuf[receivedBytes] = MySerial.read();
            receivedBytes++;
        }

        if(receivedBytes > 9){ // transmission complete
          
            uint8_t crc8 = get_crc8(SerialBuf, 9); // get the 8 bit CRC
          
            if(crc8 != SerialBuf[9]) {
//                Serial.println("CRC transmission failure");
                
                // Empty Rx Serial of garbage telemtry
                while(MySerial.available())
                    MySerial.read();
                
                requestTelemetry = true;
            
                return; // transmission failure 
            }
          
            // compute the received values
            ESC_telemetry[0] = SerialBuf[0]; // temperature
            ESC_telemetry[1] = (SerialBuf[1]<<8)|SerialBuf[2]; // voltage
            ESC_telemetry[2] = (SerialBuf[3]<<8)|SerialBuf[4]; // Current
            ESC_telemetry[3] = (SerialBuf[5]<<8)|SerialBuf[6]; // used mA/h
            ESC_telemetry[4] = (SerialBuf[7]<<8)|SerialBuf[8]; // eRpM *100
            
            requestTelemetry = true;

            /*
            if(printTelemetry) {
                Serial.print(millis()); 
                Serial.print(","); 
                Serial.print(dshotUserInputValue); 
                Serial.print(",");
          //      Serial.print("Voltage (V): ");
                Serial.print(ESC_telemetrie[1] / 100.0); 
                Serial.print(",");   
          //      Serial.print("Current (A): ");
                Serial.print(ESC_telemetrie[2] / 10.0); 
                Serial.print(","); 
          //      Serial.print("RPM : ");
                Serial.print(ESC_telemetrie[4] * 100 / (MOTOR_POLES / 2)); 
                Serial.print(",");  
                // Thrust
                Serial.println(thrust);
            }
          */
            temperature = 0.9*temperature + 0.1*ESC_telemetry[0];
            if (temperature > temperatureMax) {
                temperatureMax = temperature;
            }
            
            voltage = 0.9*voltage + 0.1*(ESC_telemetry[1] / 100.0);
            if (voltage < voltageMin) {
                voltageMin = voltage;
            }
            
            current = 0.9*current + 0.1*(ESC_telemetry[2] * 100);
            if (current > currentMax) {
                currentMax = current;
            }
            
            erpm = 0.9*erpm + 0.1*(ESC_telemetry[4] * 100);
            if (erpm > erpmMax) {
                erpmMax = erpm;
            }
            
            rpm = erpm / (MOTOR_POLES / 2);
            if (rpm > rpmMAX) {
                rpmMAX = rpm;
            }
            
            if (rpm) {                  // Stops weird numbers :|
                kv = rpm / voltage / ( (float(dshotUserInputValue) - dshotmin) / (dshotmax - dshotmin) );
            } else {
                kv = 0;
            }
            if (kv > kvMax) {
                kvMax = kv;
            }
          
        }

  return;
  
}

void dshotOutput(uint16_t value, bool telemetry) {
    
    uint16_t packet;
    
    // telemetry bit    
    if (telemetry) {
        packet = (value << 1) | 1;
    } else {
        packet = (value << 1) | 0;
    }

    // https://github.com/betaflight/betaflight/blob/09b52975fbd8f6fcccb22228745d1548b8c3daab/src/main/drivers/pwm_output.c#L523
    int csum = 0;
    int csum_data = packet;
    for (int i = 0; i < 3; i++) {
        csum ^=  csum_data;
        csum_data >>= 4;
    }
    csum &= 0xf;
    packet = (packet << 4) | csum;

    // durations are for dshot600
    // https://blck.mn/2016/11/dshot-the-new-kid-on-the-block/
    // Bit length (total timing period) is 1.67 microseconds (T0H + T0L or T1H + T1L).
    // For a bit to be 1, the pulse width is 1250 nanoseconds (T1H – time the pulse is high for a bit value of ONE)
    // For a bit to be 0, the pulse width is 625 nanoseconds (T0H – time the pulse is high for a bit value of ZERO)
    for (int i = 0; i < 16; i++) {
        if (packet & 0x8000) {
              // DMA construct packet
          } else {
              // DMA construct packet
          }
        packet <<= 1;
    }
    
    //WRITE DMA packet Dshot output HERE
    
    return;

}

uint8_t update_crc8(uint8_t crc, uint8_t crc_seed){
  uint8_t crc_u, i;
  crc_u = crc;
  crc_u ^= crc_seed;
  for ( i=0; i<8; i++) crc_u = ( crc_u & 0x80 ) ? 0x7 ^ ( crc_u << 1 ) : ( crc_u << 1 );
  return (crc_u);
}

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen){
  uint8_t crc = 0, i;
  for( i=0; i<BufLen; i++) crc = update_crc8(Buf[i], crc);
  return (crc);
}
