#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed_events.h"
#include "mbedtls/error.h"
#include "TLSSocket.h"
#include "MQTTClientMbedOs.h"
#include "MQTT_server_setting.h"
#include <string>


/* Sensors drivers present in the BSP library */
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"


/* Version and device ID setup -----------------------------------------------*/
#define DEVICE_VERSION "1.0"
#define DEVICE_ID 1


/* utils defines -------------------------------------------------------------*/
#define INTERVAL_TIME 10*1000                 // ms  


/* board components ----------------------------------------------------------*/
DigitalOut led(LED1);
#define LED_ON  1   
#define LED_OFF 0



/* Start code ----------------------------------------------------------------*/
int main(int argc, char* argv[])
{
    
    mbed_trace_init(); //debug utils
    
    /* initialize sensors */
    printf("Sensors init... ");
    BSP_TSENSOR_Init();
    BSP_HSENSOR_Init();
    BSP_PSENSOR_Init();
    printf("done\r\n");

    /* initialize wifi-mqtt components */
    WiFiInterface* network = NULL;
    TLSSocket* socket = new TLSSocket;
    MQTTClient* mqttClient = NULL;


    printf("\r\n\nAblativo\r\n");
    printf("Release version %s\r\n", DEVICE_VERSION);
    printf("Device ID:%d\r\n\n", DEVICE_ID);

#ifdef MBED_MAJOR_VERSION
    printf("Mbed OS version %d.%d.%d\r\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
#endif

    led = LED_ON;
    
    /* Make network interface */
    network = WiFiInterface::get_default_instance();
    if (!network) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }


    /* Connect to wifi network and print MAC + IP*/
    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    nsapi_error_t ret = network->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\n", ret);
        return -1;
    }

    printf("Success\n\n");
    printf("MAC: %s\n", network->get_mac_address());
    
    SocketAddress WifiSockAdd;   
    network->get_ip_address(&WifiSockAdd);
    printf("IP: %s\n\n", WifiSockAdd.get_ip_address());
    

    /* Establish connection with AWS */
    printf("Connecting to host %s:%d ...\r\n", MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
    {
        nsapi_error_t ret = socket->open(network);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not open socket! Returned %d\n", ret);
            return -1;
        }
        ret = socket->set_root_ca_cert(SSL_CA_PEM);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not set ca cert! Returned %d\n", ret);
            return -1;
        }
        ret = socket->set_client_cert_key(SSL_CLIENT_CERT_PEM, SSL_CLIENT_PRIVATE_KEY_PEM);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not set keys! Returned %d\n", ret);
            return -1;
        }
        ret = socket->connect(MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not connect! Returned %d\n", ret);
            return -1;
        }
    }
    printf("Connection established.\r\n\n");
    
    
    /* Establish MQTT connection. */
    printf("Client id: %s\r\n", MQTT_CLIENT_ID);  
    printf("MQTT client is connecting to the service ...\r\n");
    {
            
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.cleansession = false;
        data.MQTTVersion = 4; // 3 = 3.1 4 = 3.1.1
        data.clientID.cstring = (char *)MQTT_CLIENT_ID;
    
        mqttClient = new MQTTClient(socket);
        int rc = mqttClient->connect(data);
        if (rc != MQTT::SUCCESS) {
            printf("ERROR: rc from MQTT connect is %d\r\n", rc);
            return -1;
        }
        
    }
    printf("Client connected.\r\n\n");
    led = LED_OFF;

    
    /* Main loop */
    while(1) {
        
        /* Client is disconnected. */
        if(!mqttClient->isConnected()){        
            break;
        }
        
        /* Pass control to other thread. */
        if(mqttClient->yield() != MQTT::SUCCESS) {
            break;
        }

        led = LED_ON;
        
        
        /* extract telemetries */
        std::string temp = std::to_string( BSP_TSENSOR_ReadTemp()); 
        std::string hum = std::to_string( BSP_HSENSOR_ReadHumidity()); 
        std::string press = std::to_string( BSP_PSENSOR_ReadPressure()); 
        
        
        /* Compose message */
        std::string telemetries = std::string("{\"deviceId\":") + std::to_string(DEVICE_ID) + 
            ",\"temp\":" + temp + ",\"hum\":" + hum + ",\"press\":" + press + "}"; 
        char* buf = (char*)telemetries.c_str();

        MQTT::Message message;
        message.retained = false;
        message.dup = false;

        message.payload = (void*)buf;

        message.qos = MQTT::QOS0;
        message.payloadlen = strlen(buf);
        
        
        /* Publish message */
        printf("\r\nPublishing message to the topic %s:\r\n%s\r\n", MQTT_TOPIC_PUB, buf);
        int rc = mqttClient->publish(MQTT_TOPIC_PUB, message);
        if (rc != MQTT::SUCCESS) {
            printf("ERROR: rc from MQTT publish is %d\r\n", rc);
        }
        printf("Message published.\r\n");
        
        delete[] buf;
        led = LED_OFF;
        
        /* sleep for 10 secons */
        ThisThread::sleep_for(INTERVAL_TIME);
          
    }


    /* disconnect */
    if(mqttClient) {
        if(mqttClient->isConnected()) 
            mqttClient->disconnect();
        delete mqttClient;
    }
    if(socket) {
        socket->close();
        delete socket;
    }
    if(network) {
        network->disconnect();
    }
    
    printf("The client has been disconnected \r\nPush reset button to restart\r\n");
}
