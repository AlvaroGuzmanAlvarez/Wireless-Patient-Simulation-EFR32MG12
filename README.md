# Wireless-Patient-Simulation-EFR32MG12
System that simulates a Wireless Sensor Network (WSN) that monitors a Medical Patient's heartbeat and stability, and also monitors the simulated rooms temperature and humidity.
The Cookies that were used for the WSN had the EFR32MG12 System-on-Chip for processing the data, and the functionalities used were:

  -Temperature sensor (Room 1 and Room 2)
  
  -Humidity sensor (Room 1 and Room 2)
  
  -Accelerometer (Check if the patient has fallen down or not)
  
  -Analog to Digital Converter (Check the patient's hearbeat thorugh a sensor that worked with this feature)
  
The system is composed of 5 Cookies in a "Star" topology that communicate between them through UDP protocol:

  -A Central Server that receives constantly messages from the client cookies, classifies them depending on the variable received and monitors that the values are within an acceptable range
  
  -Two Cookies that measured Temperature and Humidity.
  
  -A cookie attached to the patient that uses the accelerometer for checking if the patient has fallen down or not.
  
  -A cookie that has attached to it the hearbeat sensor and uses the ADC for obtaining the patient's vital constants.

The system works as it follows:

  -Each UDP client has a periodic process that waits for a timer to end. When that timer ends, they take the measurements they are designed to and send them to the central server in a string codified like:
  
    -Characters that identify the measurement
    
    -The numeric value
    
    -For example "R1T 20"
    
  -When the server receives a message from a client, classifies the measurement depending on the code specified in the first part and, if there is an abnormal value, it displays a message through a serial terminal and toggles a LED periodically (controlled by other process) a determined number of times.

The whole system was programmed and ran in Contiki-NG Virtual Machine.
