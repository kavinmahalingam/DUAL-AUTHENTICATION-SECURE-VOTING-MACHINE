Secure Dual-Authentication Electronic Voting Machine (EVM) using ESP32, STM32, RFID (RC522), Fingerprint Sensor (R307), and TFT LCD Display

ABSTRACT

Electronic voting systems must address voter eligibility, duplicate participation, reliable vote recording, and clear operation. This project presents a laboratory-scale dual-authentication electronic voting machine prototype that separates voter authentication from ballot processing using two microcontrollers: an ESP32 authentication unit and an STM32F103C8 voting unit.
The ESP32 interfaces with an MFRC522 RFID reader, an R307 fingerprint sensor, and an ILI9341 TFT display. The RFID card identifier is checked against locally stored voter records. For a registered voter, the fingerprint sensor performs biometric verification, and the returned template identifier is compared with the expected template associated with the voter record. Following successful verification of both factors, the ESP32 sends an authorization message through a UART connection to the STM32 voting unit.
The STM32 controls the candidate-selection interface, accepts a valid candidate selection, and maintains candidate-wise vote totals. The prototype also includes status indicators, a cancellation input, result display, and a guarded reset sequence. The design emphasizes separation of authentication and voting functions, readable user feedback, and structured testing of the hardware interfaces.
The system is intended for educational demonstration. It does not provide election-grade security, cryptographic end-to-end verifiability, an independent audit trail, or certified resistance to biometric spoofing. Its plain-text UART authorization message and persistent-storage behaviour require further security and reliability improvements before any real-world use.
Keywords: Electronic voting machine, dual authentication, RFID, fingerprint verification, ESP32, STM32F103C8, UART, embedded systems.

INTRODUCTION

1.1 Background

Electronic voting machines are designed to record voter selections and produce vote totals through an electronic process. In addition to accepting a candidate selection, a voting system must control access to the ballot, prevent repeated participation, preserve recorded results, and provide understandable feedback to the user.
A single identification method may not be sufficient for a demonstration system intended to explore voter verification. An RFID card can provide a convenient, contactless identifier, but the card may be lost, borrowed, or copied. Fingerprint verification provides a second type of evidence by comparing a presented fingerprint against an enrolled template. Combining these two methods can make casual impersonation more difficult than relying on either factor alone. However, the combination does not by itself prove legal identity or guarantee resistance to spoofing.
This project investigates a dual-authentication workflow using an ESP32 and an STM32F103C8 microcontroller. The ESP32 performs RFID and fingerprint checks, while the STM32 manages candidate selection and vote counting. The division of responsibilities is intended to make the system easier to develop, test, and maintain.
The project is a laboratory prototype for studying embedded authentication, sensor interfacing, serial communication, display control, and voting-state management. It is not intended to replace certified election equipment.
