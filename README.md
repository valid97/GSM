				freeRTOS GSM project documentation
Project structure:
-GSM project contains some standard functions that implement HAL drivers for used microcontroller and freeRTOS folder that implement freeRTOS environment. It contains drivers for direct communication between console and gsm module, middleware layar that implement functionality of gsm project and application folder that implement mqtt client mode. In this diagram we can see whole structure of GSM project:
.
├── freeRTOS_GSM
   ├── Drivers
   ├── freeRTOS
   ├── Src
   │   ├── APPLICATION
   │      ├── mqtt_client.c
   │      └── mqtt_client.h
   │   ├── DRIVER
   │      ├── driver_common.h
   │      ├── driver_console.c
   │      ├── driver_console.h
   │      ├── driver_gsm.c
   │      └── driver_gsm.h
   │   ├── MIDLEWARE
   │      ├── gsm.h
   │      ├── gsm.c
   │      ├── mqtt.h
   │      ├── mqtt.c
   │      ├── time.h
   │      └── time.c
   ├── Inc
   ├── startup
   ├── Debug
   └── README.md
   
freeRTOS implementation:
-GSM project contains 6 tasks. Two tasks are for console (receiving characters from console task and transmitting characters to console task), another 2 tasks are for gsm (receiving response from gsm module and transmitting message to gs module). One task is main task that has the lowest priority and he calls all other functions in project. Also this task blocks when we have to work with console or gsm. The last task is application task to implement mqtt client. When we switch to client mode we can only listen buffer for receiving response from gsm and wait asynchronous message from broker to be sent.

					DRIVER layer
Console implementation:
-Console has initialization function that set UART for console and his callback function, create tasks for receiving and transmitting characters from/to console and sets the buffer used to collect characters. You can get characters from console using DRIVER_CONSOLE_Get() function. You can put characters to console using DRIVER_CONSOLE_Put() function. These two functions works with freeRTOS queues and notification for synchronization between main task(Demo task) and console's tasks. Characters are collected with UART interrupt routine callback function. These functions are implemented in DRIVER folder in driver_console.c and driver_console.h files.

Gsm implementation:
-Gsm has initialization function that set UART for gsm module and his callback function, create tasks for receiving and transmitting characters from/to gsm and sets the buffer used to collect characters from gsm module. You can read characters from gsm using DRIVER_GSM_Read() function. You can put message to gsm using DRIVER_GSM_Write() function. You can flush gsm and bring him to initial state with DRIVER_GSM_Flush() function. Characters are collected with UART interrupt routine callback function. These functions are implemented in DRIVER folder in driver_gsm.c and driver_gsm.h files.

Common driver file:
-It contains necessary things for both the gsm and the console.
	
				     MIDDLEWARE layer
Gsm implementation:
-Gsm implementation in middleware layer is using both console and gsm from DRIVER layer. Console is used to write some specific messages when we are executing command such as error message, timeout message, okay message etc. These is the short list of all function implemented here:

	(#) Initialize the gsm low level resources by implementing the GSM_Init() function
	(#) Set echo mode in gsm module
	(#) Set message format (pdu or text(default mode))
	(#) Set message storage for all type of messages(reading,deleting,sending,writing,receiving)
	(#) Test which storage we have (currently available is SIM storage)
	(#) List messages from storage
	(#) Read particular message using index of message parameter
	(#) Delete specific message or all messages of particular type
	(#) Send message using gsm or store message on storage and later send

	(#) Set socket in defined gsmHandler when you open new socket
	(#) Close socket in defined gsmHandler when you close opened socket
	(#) Check how many are opened sockets currently

	(#) Registers gsm module to mobile network station in Serbia region
	(#) Deregisters gsm module from mobile network
	(#) Check if it is gsm module registered on the network
	(#) Set Access Point Name (APN) for current region (Serbia)
	(#) Check setted Access Point Name
	(#) Set wireless connection to GPRS service
	(#) Get assigned IP address
	(#) Attach to GPRS service
	(#) Dettach from GPRS service
	(#) Set Packet Data Protocol(PDP) context
	(#) Active currently setted PDP context
	(#) Check setted PDP contexts
	(#) Check active PDP contexts
	(#) Deactive currently active PDP context
	(#) Connect to server with his IP address, opened port and specifed type of protocol
	(#) Disconnect from server
	(#) Check server connection
	(#) Send data to server
	(#) Establish TCP\IP connection(calling 4 function for this implementation)

	(#) onlyPutNumber() is function that checks users input and demanding only to put number
	(#) waitUntil() function is used to wait response from gsm by using TIME_Delay() function
	 	which uses timer for timing

Mqtt implementation:
-Mqtt files contains implementation of mqtt protocol. For mqtt protocol needs to be active network service, to be setted one PDP context and activated that context. Gsm must be connected to specified server with TCP IP connection. All that functions are in MIDLEWARE layer in gsm.c file. After that configuration we can use mqtt protocol. First function is to initialize the mqtt low level resources by implementing the MQTT_Init(). After that we can connect to broker with MQTT_Connect() function or disconnect from broker with MQTT_Disconnect() function. Also, we can set hexadecimal format of sending packets to broker with MQTT_SetHexFormat() function. We can publish message to topic on connected broker with MQTT_Publish() function or subscribe to the specified topic on broker with  MQTT_Subscribe() function. We can ping server with PINGREQ Packet. The Server MUST send a PINGRESP Packet in response to a PINGREQ Packet. This is implemented using MQTT_PingReq() function. When we are connected to broker we established connection with broker that lasts 1 hour. That means that we don't have to send any ping or command to broker for 1 hour time and connection will be active. After that time, if we dont send any command, broker will disconnect us from him and we will not be able to send any packets anymore, until we establish new connection with broker. We have qualty of service setted to zero(QoS is 0), so we dont wait for response from broker when we are trying to connect to broker (we hope that connection is established). We have some additional function for converting from decimal number to hexadecimal (convDecToHexchar() function), converting fro decimal to base 128 (convDecToBase128() function). We have function for adding continuation bit in remaining length if it neccessery (search more about mqtt protocol for more details of continuation bit) addCB() function. We have function to reverse array from start to end that is rverseArray() function.

 					APPLICATION layer
Mqtt client implementation:
-Mqtt client hase two function: one is to initialize the mqtt client low level resources by implementing the MQTT_CLIENT_Init() function and another is to set state of client using MQTT_CLIENT_SetState() function. State can be either BLOCK STATE or LISTEN STATE. Using MQTTClientState_t enum, you can set desirable state. In this section we have one task that handles mqtt client state. With MQTT_CLIENT_SetState() function we are changing blocking period of queue. When we want to listen buffer to see if any message was received from broker, we set blocking period to infinity and we wait in mqtt client LISTEN state. If we dont won't to wait (so we are not in mqtt client LISTEN state, so we are in mqtt client BLOCK state) our blocking period is setted to zero.














































