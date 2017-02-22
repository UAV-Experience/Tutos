#include <ArduinoJson.h> /* https://bblanchon.github.io/ArduinoJson/api/ */
#include <BufferedSerial.h>
#include <ByteBuffer.h>

int analogPin = A0;

long msg_id ;
BufferedSerial serial = BufferedSerial(256, 256);
ByteBuffer send_buffer;
ByteBuffer receive_buffer;

#define MAX_BUF (200)
#define COMM_SPEED (19200)
#define SEPARATEUR '\n'

// capteur 1 : GPS
void genere_gps_data() {
  static char data2send[MAX_BUF] ;
  int i ;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root  = jsonBuffer.createObject();
  root["msg_id"] = msg_id++ ;
  root["sensor"] = "gps";
  root["time"] = millis() ;
  JsonArray& data = root.createNestedArray("data");
  data.add(double_with_n_digits(48.756080, 6));
  data.add(double_with_n_digits(2.302038, 6));
  
  // generer le message au format JSOn dans data2send
  data2send[0] = (char)0;
  root.printTo(data2send, MAX_BUF);
  
  // mettre les données dans send_buffer
  send_buffer.clear();
  
  while( (data2send[i] != (char)0 ) && (i<MAX_BUF)) {
       send_buffer.put( data2send[i++] );
  }
  send_buffer.put( (int)SEPARATEUR ); 
  /*
  
  digitalWrite(LED_BUILTIN, LOW);
  */
}

void genere_dig_data() {
  static int last_value = 0 ;
  int current_value ;
  static char data2send[MAX_BUF] ;
  int i ;

  current_value = analogRead(analogPin);
  if( current_value == last_value ) {
      // pas de changement, on envoie rien
      return ;
  }
  last_value = current_value ;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root  = jsonBuffer.createObject();
  root["msg_id"] = msg_id++ ;
  root["sensor"] = "analog";
  root["time"] = millis() ;
  JsonArray& data = root.createNestedArray("data");
  data.add( current_value ); 
  
  // generer le message au format JSOn dans data2send
  data2send[0] = (char)0;
  root.printTo(data2send, MAX_BUF);
  
  // mettre les données dans send_buffer
  send_buffer.clear();
   
  while( (data2send[i] != (char)0 ) && (i<MAX_BUF)) {
       send_buffer.put( data2send[i++] );
  }
  send_buffer.put( (int)SEPARATEUR );   
}

void genere_data() {
  static int iteration = 0 ;

  iteration = (iteration + 1) % 50  ;
  if( iteration == 0 ) {
    genere_gps_data();
  } else {
    genere_dig_data();
  }
}
 
void setup() {
   msg_id = 0 ;
   
   pinMode(LED_BUILTIN, OUTPUT);
  
  // initialize the serial communication:
   serial.init(0, COMM_SPEED);
   serial.setPacketHandler(handlePacket);

   // Initialize the send buffer that we will use to send data
   send_buffer.init(256);
   receive_buffer.init(256);
}

void handlePacket(ByteBuffer* packet){    
   if( receive_buffer.getSize() == 0 ) {
      // buffer de reception plein....
      while( packet->getSize() > 0 )
          packet->get() ; 
   } else {
      while( packet->getSize() > 0 )
       receive_buffer.put( packet->get() ); 
   }
 }

 void traiteRx() {
  static bool v = false ;
  // on attend un message "basique" par exemple :{"command": "led_status", "value": 0}
  //      StaticJsonBuffer<100> jsonBuffer;
  //      JsonObject& root = jsonBuffer.parseObject(json);

  receive_buffer.clear();
  v = ~v ;
  if( v ) 
    digitalWrite(LED_BUILTIN, HIGH); 
  else
    digitalWrite(LED_BUILTIN, LOW); 
 }

void loop() {
    serial.update();
    while( serial.isBusySending() ) {
        serial.update();
    }

    // est ce qu'on peut envoyer qqchose ?
    if( send_buffer.getSize() > 0 ){      
      // donnees a transmettre         
      serial.sendRawSerial( &send_buffer );
      send_buffer.clear();                   
    }
    
    if( receive_buffer.getSize() > 0 ) {
        traiteRx() ;
    }

    genere_data() ;     
}
