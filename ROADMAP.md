# M4 Track Protocol Implementation Roadmap

This roadmap outlines the features to be implemented based on the mfx® Rail format specification (`docs/M4-track-protocol.md`), split into Decoder and Central Station components.

# Part 1: Decoder Implementation

## 1. Signal Reception (Physical & Data Link Layer)
- [x] Implement rail signal decoding (square wave alternating voltage processing)
- [x] Implement bit decoding (biphase mark decoding, 10 kBaud)
- [x] Implement synchronization pattern detection (01110 with violations)
- [ ] Implement bit stuffing removal (remove 0 after 8 ones)
- [ ] Implement data frame parsing (Address, Commands, Checksum)
- [ ] Implement CRC checksum verification (Polynomial x^8 + x^2 + x + 1)
- [ ] Implement feedback window detection (sync patterns, 0011 sequence)

## 2. Return Channel (Feedback Generation)
- [ ] Implement RDS carrier signal generation (52.63 kHz)
- [ ] Implement 1-bit feedback encoding (current pulses, ASK modulation)
- [ ] Implement data feedback modulation (PSK, biphase coding, 1096.5 baud)
- [ ] Implement data feedback frame generation (Preamble 11111 010, Data bytes, Checksum)
- [ ] Implement feedback checksum calculation

## 3. Command Handling (Application Layer)
- [ ] Handle "No command" (Ping) - Send 1-bit feedback if addressed
- [ ] Handle "Drive (short)" command (000) - Update speed/direction
- [ ] Handle "Drive" command (001) - Update speed/direction (127 steps)
- [ ] Handle "Functions (short)" command (010) - Update F0-F4
- [ ] Handle "Functions (extended)" command (011) - Update F0-F15
- [ ] Handle "Functions (individual control)" command (100)
- [ ] Handle "Configuration" command (111)
  - [ ] Handle "Read CV" (111 000) - Send data feedback
  - [ ] Handle "Write CV" (111 001) - Update memory
  - [ ] Handle "DecoderSearch" (111 010) - Check UID match, send 1-bit feedback
  - [ ] Handle "Assignment of rail address" (111 011) - Store assigned SID
  - [ ] Handle "Existence query (Ping, mfx Verify)" (111 100) - Verify UID/SID match
  - [ ] Handle "Headquarters" (111 101) - Store Central UID and Registration Counter

## 4. Protocol Logic (Decoder Side)
- [ ] Implement "Switching on" sequence (Wait for Central command, check stored UID/Counter)
- [ ] Implement Registration response logic (Respond to Search, accept Address)
- [ ] Implement Re-registration logic (if Central UID/Counter changes)
- [ ] Implement Emergency stop execution (Speed level 1 handling)
- [ ] Implement Protocol switching (Detect mfx/MM/DCC/Analog)
- [ ] Implement Decoder Reset functionality

## 5. mfx+ Extension (Decoder Side)
- [ ] Implement mfx+ mode setting (Standard vs mfx+)
- [ ] Implement Resource management (Fuel A, Fuel B, Sand) storage
- [ ] Implement Consumption calculation logic (based on load/speed/time)
- [ ] Implement Emergency operation (limit speed on Low level/Empty)
- [ ] Implement Operational recording (update Distance, Time counters)

## 6. Configuration Memory (CVs)
- [ ] Implement CV addressing mechanism (Number + Index)
- [ ] Implement Block structure (Block Table, Block Descriptions)
- [ ] Implement Block 01: Decoder properties (Manufacturer, Version, etc.)
- [ ] Implement Block 02: Functions presence
- [ ] Implement Block 03: Automatic functions
- [ ] Implement Block 04: Function assignment (Mapping, Symbols)
- [ ] Implement Block 05: Driving characteristics (Motor type, Curves, etc.)
- [ ] Implement Block 06: Hardware functions (Outputs, Light effects)
- [ ] Implement Block 07: Additional protocols (MM, DCC, Analog)
- [ ] Implement Block 08: Sound settings
- [ ] Implement Block 09: Special options
- [ ] Implement Block 0A: mfx+ configuration

---

# Part 2: Central Station Implementation

## 1. Signal Generation (Physical & Data Link Layer)
- [ ] Implement rail signal generation (square wave alternating voltage +/- 18V)
- [ ] Implement bit encoding (biphase mark encoding, 10 kBaud)
- [ ] Implement synchronization pattern generation (01110 with violations)
- [ ] Implement bit stuffing insertion (insert 0 after 8 ones)
- [ ] Implement Big Endian bit order transmission
- [ ] Implement data frame construction (Address, Commands, Checksum)
- [ ] Implement CRC checksum calculation/appending
- [ ] Implement feedback window generation for 1-bit feedback (pauses)
- [ ] Implement feedback window generation for data feedback (pauses with clock signals)
- [ ] Implement embedding of MM control commands (pauses and MM frames)

## 2. Return Channel (Feedback Reception)
- [ ] Implement detection of 1-bit feedback (current pulse detection)
- [ ] Implement demodulation of data feedback (PSK demodulation)
- [ ] Implement data feedback frame parsing
- [ ] Implement feedback checksum verification

## 3. Command Sending (Application Layer)
- [ ] Implement sending "No command" (Ping)
- [ ] Implement sending "Drive (short)" command (000)
- [ ] Implement sending "Drive" command (001)
- [ ] Implement sending "Functions (short)" command (010)
- [ ] Implement sending "Functions (extended)" command (011)
- [ ] Implement sending "Functions (individual control)" command (100)
- [ ] Implement sending "Configuration" command (111)
  - [ ] Implement "Read CV" command generation (111 000)
  - [ ] Implement "Write CV" command generation (111 001)
  - [ ] Implement "DecoderSearch" command generation (111 010)
  - [ ] Implement "Assignment of rail address" command generation (111 011)
  - [ ] Implement "Existence query" command generation (111 100)
  - [ ] Implement "Headquarters" command generation (111 101)

## 4. Protocol Logic (Central Side)
- [ ] Implement Sleep mode cycle (Cyclic transmission of Headquarters, Search, Query, Refresh)
- [ ] Implement Registration master process (Binary search algorithm for UID, Address management)
- [ ] Implement Log out process (Manage registered list, increment counter)
- [ ] Implement Central Unit reset handling (Increment counter)
- [ ] Implement Driving and operating logic (Prioritize user commands vs cyclic refresh)
- [ ] Implement Emergency stop logic (Send Speed level 1)
- [ ] Implement Configuration reading logic (Navigate CV blocks, Read Block Table)

## 5. mfx+ Extension (Central Side)
- [ ] Implement mfx+ mode activation
- [ ] Implement cyclic querying of resources (Fuel A, B, Sand)
- [ ] Implement logic to display resource levels and trigger "Refill" (Write CV)
