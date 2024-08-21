#include "network.h"


EthernetUDP network_pcb_output;

uint8_t output_buffer[OUTPUT_BUFFER_SIZE];
volatile uint16_t output_buffer_cnt;


void networkSetup(){
  Serial.println("Network setup");

  // Start ethernet and UDP
  byte mac[] = MAC_ADDRESS;
  if (Ethernet.begin(mac, 1000, 200) == 0){
    Serial.println("DHCP connection failed, initializing with static IP");
    Ethernet.begin(mac, DEFAULT_IP);
  }
  
  if (!network_pcb_output.begin(LOCAL_PORT)){
    Serial.print("Failed to start UDP socket on port: ");
    Serial.println(LOCAL_PORT);
  };
    
  // Disable SD card (to be safe)
  pinMode(SD_CARD_PIN, OUTPUT); 
  digitalWrite(SD_CARD_PIN, HIGH);
}

bool sendData(){
  uint32_t bytesToWrite = output_buffer_cnt;
  if (bytesToWrite > UDP_MIN_PAYLOAD_SIZE){
    if (!sendUdpMsg(&network_pcb_output, REMOTE_IP, REMOTE_PORT, output_buffer, output_buffer_cnt))
      return false;
    
    output_buffer_cnt -= bytesToWrite; // Buffer reset
  }
  return true;
}

void networkUpdate(){
  Ethernet.maintain();

  if (!sendData()){
    Serial.println("Network update: Failed to send data...");
  }
}


void networkPushData(uint8_t* src_buffer, uint16_t size){
  noInterrupts();
  if (size <= OUTPUT_BUFFER_SIZE - output_buffer_cnt){
    memcpy(output_buffer + output_buffer_cnt, src_buffer, size);
    output_buffer_cnt += size;
  }
  interrupts();
};


bool sendUdpMsg(EthernetUDP *pcb, IPAddress dst_ip, int dst_port, uint8_t *buffer, uint16_t size) {
  // Begin packet
  if (!pcb->beginPacket(dst_ip, dst_port)) {
    return false;
  };

  // Write bytes
  uint32_t bytesWritten = pcb->write(buffer, size);
  if (bytesWritten != size) {
    return false;
  };

  // Send packet
  if (!pcb->endPacket()) {
    return false;
  }

  return true;
}