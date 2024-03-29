#include <mcp_can.h>
#include <SPI.h>

// SYSTEM_THREAD(ENABLED);

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char cinderString[128];                        // Array to store string
String victronString;

#define CAN0_INT A1                              // Set INT to pin A1
MCP_CAN CAN0(A2);                               // Set CS to pin A2

unsigned long victronLast = 0;
unsigned long cinderLast = 0;

int publishFlag = 0;
int publishInterval = 60000;                    // 60 second intervals

void setup() {
    
    Serial1.begin(19200);

    // Wait 10 seconds for USB debug serial to be connected (plus 1 more)
    waitFor(Serial.isConnected, 10000);
    delay(1000);

  
    // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println("MCP2515 Initialized Successfully!");
    } else {
        Serial.println("Error Initializing MCP2515...");
    }

    CAN0.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.

    pinMode(CAN0_INT, INPUT);                            // Configuring pin for /INT input
}

void loop() {
    
    unsigned long now = millis();
    
    if (!digitalRead(CAN0_INT)) {                       // If CAN0_INT pin is low, read receive buffer
  
        CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

        if (rxId == 0x99FEB38F) {
            if ((unsigned long)(now - cinderLast) >= publishInterval) {
                
                cinderLast = now;
  
                int x = (rxBuf[2]<<8) + (rxBuf[1]);
                float v = (float)x * 0.05;
  
                int y = (rxBuf[4]<<8) + (rxBuf[3]);
                float a = (float)y * 0.05;
            
                int z = rxBuf[6];
                
                sprintf(cinderString, "V %.2f A %.2f OS %d", v, a, z);
            
                Particle.publish("new-data", cinderString, PRIVATE);
            }
        }
    }
  
    if (Serial1.available()) {
        
        if (!publishFlag) {
            publishFlag = 1;
            victronLast = now;
        
            while (Serial1.available() > 0) {
                char c = char(Serial1.read());
                victronString.concat(c);
            }
        } 
    }
    
    if ((unsigned long)(now - victronLast) >= publishInterval) {
        
        if (publishFlag) {
            publishFlag = 0;
            Particle.publish("data", victronString, PRIVATE);
        } 
        
        victronString = "";
    }
}
