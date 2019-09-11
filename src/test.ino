#include "application.h"
#include "ArduCAM.h"
#include "memorysaver.h"


#define VERSION_SLUG "7n"
#define TX_BUFFER_MAX 1024 //set just below max 1024

const int SPI_CS = B0;
ArduCAM myCAM(OV5642, SPI_CS);

TCPClient client;

int runOnce = 0;

void setup() {

    Serial.begin(9600);

    //pinMode(C5, INPUT);
    //pinMode(C4, OUTPUT);
    Wire1.setSpeed(CLOCK_SPEED_400KHZ);
	  Wire1.begin();

    delay(100);

    SPI.begin(B0);

    delay(100);

    myCAM.set_format(JPEG);
    delay(100);
    myCAM.InitCAM();
    delay(100);
    myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    delay(100);
    myCAM.clear_fifo_flag();
    delay(100);
    myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
    delay(100);
    myCAM.OV5642_set_JPEG_size(OV5642_640x480);
    delay(100);


}


void loop() {

  if (runOnce <= 0) {
    captureImage();
    connectToClient();
    runOnce = 1;
  }

}

void captureImage() {

    delay(100);
	  myCAM.flush_fifo();
    delay(100);
	  myCAM.clear_fifo_flag();
    delay(100);
	  myCAM.start_capture();
    //Serial.println("camera: START CAPTURE");
	  return;
}



void connectToClient(){
    client.stop();
    if (!client.connect("SERVER_INFO", 80)) {
      Serial.println("failed to connect");
      retryClientConnection();
    } else {
      Serial.println("connected New");
      sendCapturedImage();
    }
}

void retryClientConnection() {
    Serial.println("retrying");

    delay(1000);
    // Wait a few seconds before retrying
    connectToClient();
}

void sendCapturedImage() {


    String dvcId = Particle.deviceID();
    uint8_t buffer[TX_BUFFER_MAX+1];
    int tx_buffer_index = 0;
    uint8_t temp = 0xff, temp_last = 0;

    int dvcLen = dvcId.length();
    size_t len0 = myCAM.read_fifo_length();

    int extraChar = 150; //Still need to resolve the CONNECTION IDLE issue. Server still waits for more data
    int totalSize = len0+extraChar;

    Serial.println(len0);


    if (client.connected()) {

        Serial.println("serving");
        client.println("POST /upload HTTP/1.1");
      	client.println("Host: SERVER_INFO");
      	client.println("Connection: close");
      	client.println("Content-Type: multipart/form-data; boundary=abcd123");
      	client.print("Content-Length: ");
        client.println(String(totalSize));

        client.println();
        client.println("--abcd123");
        client.println("Content-Disposition: form-data; name=\"fileToUpload\"; filename=\""+dvcId+"\"");
        client.println();

          	if(myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)){
          			 delay(100);


          			    myCAM.set_fifo_burst();

          					tx_buffer_index = 0;
          					temp = 0;

          					//while (bytesRead < length)
          					while( (temp != 0xD9) | (temp_last != 0xFF) ){
          							temp_last = temp;
          							temp = myCAM.read_fifo();

          							buffer[tx_buffer_index++] = temp;

          							if (tx_buffer_index >= TX_BUFFER_MAX) {
          									client.write(buffer, tx_buffer_index);
                            tx_buffer_index = 0;
          							}
          					}

          					if (tx_buffer_index != 0) {
          							client.write(buffer, tx_buffer_index);
          					}

          					//Clear the capture done flag
          					myCAM.clear_fifo_flag();
          				}



          client.println();
          client.println("--abcd123--");
          Serial.println("sent");

          //client.flush();
        }

    return;
}
