# LIGHT INTENSITY MONITOR ( in progress)
This project focuses on the LDR sensor `NSL 19M51` and how to uztilize `Modbus RTU` frame to send and receive signals from and to master device. In this project, the PCB module is designed and manufactured to plug in STM32L152RE to process the temperature and humidity data, then send the result to the master device. `Wapice IoT-ticket` is used to show the data sent to master device as a real-time graphs.




Table of Content  
	
	1. Equipment and Component  
	
	2. Description 


	
	
1. Equipment and Component

* 1 x `NUCLEO-L152RE` (development board)
* LDR sensor `NSL 19M51`
* USB to RS485 Serial Converter Cable


2. Description

The overview of this project is to build a system that can meassure the Intensity of the light and analyses the signal and change it from particular shape to another and transmite the signal from one point to another over a network and store it in database then visuallize it on website

Project diagram 

![diagram](https://github.com/ThinhLe279/Light_Intensity_Monitor/blob/main/pictures/project_diagram.png)

This project is on the Slave site, it receives a request frame ( 8 bytes ) from the Master 

![request_frame](https://github.com/ThinhLe279/Light_Intensity_Monitor/blob/main/pictures/request_frame.png)

After that it checks the address in the request frame ( the 1st byte) if the address is the same with its address ( in our case the address is 0x04 ) and start processing the data, making the response frame (7 bytes) and send to the Master through MODBUS. The sensor value is represented in 2 bytes (the 4th and the 5th byte from the left)

![respond](https://github.com/ThinhLe279/Light_Intensity_Monitor/blob/main/pictures/respond_frame.png)

Finally the data will be visualized on the website through Wapice IoT Ticket.




