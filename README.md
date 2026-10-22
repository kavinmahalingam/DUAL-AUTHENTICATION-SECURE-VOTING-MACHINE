Secure Dual-Authentication Electronic Voting Machine (EVM) using ESP32, STM32, RFID (RC522), Fingerprint Sensor (R307), and TFT LCD Display

ABSTRACT:

The integrity of democratic processes hinges on the trustworthiness of electoral systems. As Electronic Voting Machines (EVMs) replace traditional ballot paper systems worldwide, ensuring their security against fraud, tampering, and unauthorized access becomes paramount. This paper presents the comprehensive design, implementation, and analysis of a Secure Dual-Authentication Electronic Voting Machine that employs a distributed two-microcontroller architecture to deliver robust voter verification and tamper-resistant vote recording. The proposed system segregates functionality into two physically and logically isolated modules: Module 1 (Authentication Unit) built around an ESP32 microcontroller, and Module 2 (Voting and Display Unit) built around an STM32F103C8 microcontroller. Voter identity is established through a strict two-factor authentication scheme: possession-based verification using an MFRC522 RFID reader and inherent biometric verification using an R307 optical fingerprint sensor. Only upon successful dual verification does the ESP32 transmit a cryptographic-token authorization string over a unidirectional UART link to the STM32, which then enables the voting interface displayed on an ILI9341 TFT LCD. Votes are securely persisted in the STM32's non-volatile EEPROM to survive power interruptions. Additional security features include 24-hour duplicate-vote prevention through persistent vote logs, a physical session-cancel mechanism for voter privacy, and a two-step confirmation dialog to prevent accidental reset of election results. The prototype was rigorously tested across functional, fault-injection, and edge-case scenarios. Results indicate 100% accuracy in rejecting unauthorized voters, near-zero false-acceptance rate for mismatched RFID-fingerprint pairs, a mean authentication latency of 2.4 seconds, and complete data persistence across 50+ power cycles. The work demonstrates that a low-cost, distributed, dual-factor EVM can achieve a security posture significantly superior to conventional single-factor systems while remaining deployable in resource-constrained environments.

Index Terms—Electronic Voting Machine (EVM), Dual-Factor Authentication, ESP32, STM32F103C8, RFID, Biometric Verification, Fingerprint Authentication, Embedded Security, UART Communication, EEPROM Persistence, TFT Display, Election Integrity, Voter Privacy.

I. INTRODUCTION:

A. Background and Motivation

Electronic voting has emerged as a cornerstone technology in modern democratic elections, offering advantages in efficiency, accessibility, and rapid tabulation of results [1]. Since their introduction in the late 20th century, Electronic Voting Machines (EVMs) have been deployed at national scale in countries such as India, Brazil, and the Philippines, and piloted extensively in the United States and Europe [2]. Despite these advantages, the adoption of EVMs is often accompanied by public skepticism and legal challenges rooted in legitimate concerns about cybersecurity, tamper-resistance, and verifiability [3], [4].

Traditional EVMs, including many commercially deployed Direct Recording Electronic (DRE) systems, typically rely on a single authentication factor — most commonly a password, a mechanical key, or a single RFID card [5]. A single point of trust is inherently fragile: a stolen or cloned card, a shared password, or a coerced insider is sufficient to compromise the system. Furthermore, physical security alone cannot guarantee that an authenticated voter is the legitimate cardholder rather than an impersonator.

Multi-factor authentication (MFA) has long been established in high-security domains such as banking, cloud computing, and physical access control [6]. The U.S. National Institute of Standards and Technology (NIST) Special Publication 800-63B recommends MFA for all systems requiring substantial identity assurance, ideally combining two or more of the categories: something you know, something you have, and something you are [7]. Applying these principles to EVMs is both theoretically sound and practically feasible with modern low-cost microcontrollers and sensors.

B. Problem Statement:

Conventional low-cost EVMs suffer from several systemic weaknesses:

Single-Factor Vulnerability: A lost, stolen, or cloned credential alone is sufficient to cast a vote.

Centralized Trust: A single firmware or a single microcontroller performs all security-relevant operations, meaning a single exploit compromises the entire election.

Lack of Re-verification: Some systems allow re-entry or partial re-authentication without full validation.

Poor Voter Privacy: Inadequate cancel mechanisms force voters to complete a vote once started, potentially under coercion.

Data Loss on Power Failure: Volatile-only vote storage risks losing ballots during power interruption.

Accidental Reset Risk: A single mis-triggered button can erase all recorded results.

C. Objectives:

1.This project addresses each of the above weaknesses through the following design objectives:

2.Design a dual-factor authentication EVM where voter identity requires both possession (RFID card) and biometrics (fingerprint).

3.Implement a distributed two-module architecture that separates authentication from voting, thereby eliminating single points of failure and improving auditability.

4.Ensure 24-hour duplicate-vote prevention through persistent vote logging.

5.Provide non-volatile vote storage in EEPROM to guarantee data durability.

6.Deliver a user-friendly graphical interface via dual TFT LCD displays with animations and confirmation screens.

7.Implement robust security logic including session cancellation, confirm-to-reset, and result-viewing controls.

8.Validate the system through functional, fault-injection, and reliability testing.

D. Contributions:

1.The principal contributions of this work are:

2.A reference architecture for a distributed, dual-factor EVM suitable for classroom, campus, or organizational elections.

3.A concrete hardware and software implementation validated on real hardware.

E. Organization:

Section II reviews related work and background. Section III describes the system architecture. Section IV provides hardware design and pin-level interfacing. Section V details the software methodology and security logic. Section VI analyzes security threats and mitigations. Section VII presents results and discussion. Section VIII concludes with future scope.

A security analysis identifying attack vectors and corresponding mitigations.

Empirical performance data on authentication latency, false-acceptance rate, false-rejection rate, and persistence reliability.

A modular baseline for future research in blockchain-verified and cloud-aggregated voting.
