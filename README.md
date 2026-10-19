The Dual-Authentication Secure Voting System uses two microcontrollers to demonstrate a two-stage voter authentication process. An STM32F103C8T6 Blue Pill verifies the registered RFID card, while an Arduino UNO R3 communicates with an R307 fingerprint sensor to perform biometric verification before allowing candidate selection.

The project explores authentication, embedded systems, persistent data storage, and privacy-aware voting workflows.

Project type: Embedded Systems / Electronics and Communication Engineering
Platform: STM32F103C8T6 and Arduino UNO R3
Development environment: Arduino IDE
Status: Academic hardware prototype in development

Objectives:

1.Implement RFID-based voter identification.

2.Add fingerprint verification as a second authentication factor.

3.Prevent repeat voting within the prototype.

4.Maintain election totals across power cycles using EEPROM.

5.Provide administrative access to election results.

7.Demonstrate deliberate election-data clearing.

8.Avoid storing a voter-to-candidate mapping.
