# Wireless-Patient-Simulation-EFR32MG12
System that simulates a Wireless Sensor Network (WSN) that monitors a Medical Patient's heartbeat and stability, and also monitors the simulated rooms temperature and humidity.
The Cookies that were used for the WSN had the EFR32MG12 System-on-Chip for processing the data, and the functionalities used were:
  -Temperature sensor (Room 1 and Room 2)
  -Humidity sensor (Room 1 and Room 2)
  -Accelerometer (Check if the patient has fallen down or not)
  -Analog to Digital Converter (Check the patient's hearbeat thorugh a sensor that worked with this feature)

The system was composed of 5 Cookies that communicated between them through UDP protocol:
  -A Central Server that received constantly all the measurements from the other Cookies and turned on or off some LEDs if the measurements received exceed certain thresholds, meeting alarm conditions. It constantly prints the measurements and messages through a serial terminal
  -Two Cookies that measured Temperature and Humidity.
  -A cookie attached to the patient that uses the accelerometer for checking if the patient has fallen down or not.
  -A cookie that has attached to it the hearbeat sensor and uses the ADC for obtaining the patient's vital constants.

The whole system was programmed and ran in Contiki-NG Virtual Machine.
