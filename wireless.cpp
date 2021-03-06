#include "wireless.h"
#include <EEPROM.h>
#include <TrueRandom.h>
#include <string.h>

#include <RFM69.h>

RFM69 radio;
uint8_t queue_num = 0;
extern uint8_t id;
extern char oled_msg[];

void setup_rfm() {
    // Open a serial port so we can send keystrokes to the module: 

    Serial.print("Node ");
    Serial.print(MYNODEID,DEC);
    Serial.println(" ready");  

    // Set up the indicator LED (optional):

    // pinMode(LED,OUTPUT);
    // digitalWrite(LED,LOW);
    // pinMode(GND,OUTPUT);
    // digitalWrite(GND,LOW);
    
    // Initialize the RFM69HCW:

    radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
    radio.setHighPower(); // Always use this for RFM69HCW

    // Turn on encryption if desired:
    if (ENCRYPT)
        radio.encrypt(ENCRYPTKEY);

}

void Blink(byte PIN, int DELAY_MS)
// Blink an LED for a given number of ms
{
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}


void id_request () {
    uint16_t extra_id= TrueRandom.rand();     // used for in case more than 1 device is requesting id at the same time
    char sendbuffer[62];
    char temp_buff[10];

    strcpy(sendbuffer, "id_rqst:");     
    strcat(sendbuffer, itoa((int)id, temp_buff, 10) );
    strcat(sendbuffer, ":");
    strcat(sendbuffer, itoa((int)extra_id, temp_buff, 10)) ;        
    
    Serial.println(sendbuffer);

    if (USEACK)
    {
        if (radio.sendWithRetry(TONODEID, sendbuffer, strlen(sendbuffer))) {
            Serial.println("ACK received!");
            strncpy(oled_msg, "ACK received! RSSI: ", 40);
            strcat(oled_msg, itoa(radio.RSSI, temp_buff, 10) );
                 
        }
        else {
            Serial.println("no ACK received!");
            strncpy(oled_msg, "No ACK received! RSSI:", 40);
            strcat(oled_msg, itoa(radio.RSSI, temp_buff, 10) );
        }
            
    }
}

void rfm_loop()
{
    // Set up a "buffer" for characters that we'll send:
  
    static char sendbuffer[62];
    static int sendlength = 0;

    // SENDING
    // In this section, we'll gather serial characters and
    // send them to the other node if we (1) get a carriage return,
    // or (2) the buffer is full (61 characters).    
    // If there is any serial input, add it to the buffer:
    if (Serial.available() > 0)
    {
        char input = Serial.read();
        
        if (input != '\r') // not a carriage return
        {
            sendbuffer[sendlength] = input;
            sendlength++;
        }

        // If the input is a carriage return, or the buffer is full:
        
        if ((input == '\r') || (sendlength == 61)) // CR or buffer full
        {
        // Send the packet!
            Serial.print("sending to node ");
            Serial.print(TONODEID, DEC);
            Serial.print(": [");
            for (byte i = 0; i < sendlength; i++)
                Serial.print(sendbuffer[i]);
            Serial.println("]");
            
            // There are two ways to send packets. If you want
            // acknowledgements, use sendWithRetry():
            
            if (USEACK)
            {
                if (radio.sendWithRetry(TONODEID, sendbuffer, sendlength))
                Serial.println("ACK received!");
                else
                Serial.println("no ACK received :(");
            }

            // If you don't need acknowledgements, just use send():
            
            else // don't use ACK
            {
                radio.send(TONODEID, sendbuffer, sendlength);
            }
            
            sendlength = 0; // reset the packet
            // Blink(LED,10);
        }
    }

    // RECEIVING
    // In this section, we'll check with the RFM69HCW to see
    // if it has received any packets:
    if (radio.receiveDone()) // Got one!
    {     
         // Print out the information:        
        Serial.print("received from node ");
        Serial.print(radio.SENDERID, DEC);
        Serial.print(": [");        

        // The actual message is contained in the DATA array,
        // and is DATALEN bytes in size:        
        // for (byte i = 0; i < radio.DATALEN; i++)
        // Serial.print((char)radio.DATA[i]);

        

        radio.DATA[radio.DATALEN] = 0;    // to MAKE it a string (coz not sure radio.DATA is nul terminated)       
        
        
        char *token;
        char *rest = radio.DATA;

        // the format must be -->   event:value
        // example:                 queue:13
        //                          id_req:254
        //                          cook:1
        char *event;
        char *value;

        token = strtok_r(rest, ":", &rest);
        event = token;
        Serial.print(event);

        token = strtok_r(rest, ":", &rest);
        value = token;
        Serial.println(value);

        if ( strcmp(event, "queue") == 0 ) queue_num = atoi(value);
        else if ( strcmp(event, "id_given") == 0 ) {
            uint8_t new_id = atoi(value);
            if ( (new_id > 0) && (new_id < 100) ) {      // for now it is limited to 99 ids
                EEPROM.write(0, new_id);
                id = new_id;
            }
            else Serial.println( "The given id is out of range" );
        }
        else Serial.println("command not recognized.");
        // while ((token = strtok_r(rest, ":", &rest))) {
        //     Serial.println(token);

        // };           
        
        // String received_string = radio.DATA;        
        // Serial.print ("below this is printed as string:");
        // Serial.println (received_string);

        // String stringOne = "Hello String";
        // Serial.println(stringOne);      // prints "Hello String"

        // RSSI is the "Receive Signal Strength Indicator",
        // smaller numbers mean higher power.        
        Serial.print("], RSSI ");
        Serial.println(radio.RSSI);

        // Send an ACK if requested. 
        // (You don't need this code if you're not using ACKs.)        
        if (radio.ACKRequested())
        {
            radio.sendACK();
            Serial.println("ACK sent");
        }
        // Blink(LED,13);

       

        
    }
}

void rfm_loop2() {
    
  // Set up a "buffer" for characters that we'll send:
  
  static char sendbuffer[62];
  static int sendlength = 0;

  // SENDING

  // In this section, we'll gather serial characters and
  // send them to the other node if we (1) get a carriage return,
  // or (2) the buffer is full (61 characters).
  
  // If there is any serial input, add it to the buffer:

  if (Serial.available() > 0)
  {
    char input = Serial.read();
    
    if (input != '\r') // not a carriage return
    {
      sendbuffer[sendlength] = input;
      sendlength++;
    }

    // If the input is a carriage return, or the buffer is full:
    
    if ((input == '\r') || (sendlength == 61)) // CR or buffer full
    {
      // Send the packet!


      Serial.print("sending to node ");
      Serial.print(TONODEID, DEC);
      Serial.print(": [");
      for (byte i = 0; i < sendlength; i++)
        Serial.print(sendbuffer[i]);
      Serial.println("]");
      
      // There are two ways to send packets. If you want
      // acknowledgements, use sendWithRetry():
      
      if (USEACK)
      {
        if (radio.sendWithRetry(TONODEID, sendbuffer, sendlength))
          Serial.println("ACK received!");
        else
          Serial.println("no ACK received :(");
      }

      // If you don't need acknowledgements, just use send():
      
      else // don't use ACK
      {
        radio.send(TONODEID, sendbuffer, sendlength);
      }
      
      sendlength = 0; // reset the packet
      //Blink(LED,10);
    }
  }

  // RECEIVING

  // In this section, we'll check with the RFM69HCW to see
  // if it has received any packets:

  if (radio.receiveDone()) // Got one!
  {
    // Print out the information:
    
    Serial.print("received from node ");
    Serial.print(radio.SENDERID, DEC);
    Serial.print(": [");

    // The actual message is contained in the DATA array,
    // and is DATALEN bytes in size:
    
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);

    // RSSI is the "Receive Signal Strength Indicator",
    // smaller numbers mean higher power.
    
    Serial.print("], RSSI ");
    Serial.println(radio.RSSI);

    // Send an ACK if requested.
    // (You don't need this code if you're not using ACKs.)
    
    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.println("ACK sent");
    }
    //Blink(LED,10);
  }
}
