# Real-time Wearable Respiratory Monitoring System

This project develops a proof-of-concept real-time respiratory monitoring system designed to be non-intrusive, capable of capturing recognizable breathing signals, and transmitting the data over UDP. It is aimed at patients requiring continuous respiratory monitoring, ensuring minimal user interference and adaptability to different patient positions.

## Proof-of-Concept Design Goals

- **Minimal Interference**: Nonintrusive mounting on the user.
- **Breathing Signal Capture**: Recognizable inhale and exhale patterns capture, effective in both sitting and lying down positions.
- **Data Transmission**: Efficient breathing signal data transmission over UDP.

## Platform

- **Device**: M5 StickC Plus, built on the ESP32-PICO SoC.
- **IMU**: MPU6886 for motion tracking.
- **Power**: Equipped with a 120mAh battery.
- **WiFi**: Onboard 2.4GHz antenna utilizing the ESP32 WiFi stack for data transmission.

<p align="center">
  <img src="https://github.com/mai-amber/Media/blob/1bce66f0f789bc35341d74f84794446841127932/Sensor%204.jpg" width="400"/>
</p>

## Wi-Fi Transmitter Details

- **Wi-Fi Protocol**: 2.4GHz (802.11b/g/n).
- **UDP Port**: 1234 for data streaming.
- **Output Data Rate**: 167-333Hz, corresponding to every 3-6ms.
- **IP Address**: Assigned dynamically.
- **Device Identifier**: Incorporates a "patient_id" as an unsigned 4-digit integer.
- **Packet Type**: 20-byte string format "patient_id,elapsedTime,ac_sig", where:
  - `patient_id` is an unsigned 4-digit integer.
  - `elapsedTime` is an unsigned 32-bit integer, indicating time in milliseconds since power-on, with a maximum stored value representing 49.7 days.
  - `ac_sig` is a signed 16-bit integer showing the accelerometer signal in G-force (units of 0.1mG).

## User Input/Output

- **Button A**: Toggles the device's LED on or off.
- **Button B**: Pauses or resumes the UDP stream, effectively toggling the stream state.
- **Button PWR**: Powers the device on (long press ~2 seconds) or off (long press ~6 seconds).

<p align="center">
  <img src="https://github.com/mai-amber/Media/blob/1bce66f0f789bc35341d74f84794446841127932/Sensor%203.jpg" width="400"/>
</p>

## High-Level State Diagram

The system's operation is illustrated through a state diagram focusing on the "stream" state, which is controlled by Button B. This button also allows for resetting the `elapsedTime` to 0, providing flexibility in monitoring sessions.

<p align="center">
  <img src="https://github.com/mai-amber/Media/blob/1bce66f0f789bc35341d74f84794446841127932/Diagram.jpg" width="400"/>
</p>

## Initial Setup

To run the code effectively, modify the IMU header file within the M5StickC library to set the resolution to its maximum by editing the `MPU6886.h` file: change line 66 to `Ascale Acscale = AFS_2G;`.

## Mounting

The device utilizes a 3D printable clip that can be attached to the user's clothing, ensuring a reliable but non-permanent attachment method.

<p align="center">
  <img src="https://github.com/mai-amber/Media/blob/1bce66f0f789bc35341d74f84794446841127932/sensor%201.jpg" width="400"/>
</p>

## Placement

For optimal signal capture, the device should be placed over clothing on the clavicle, approximately midway between the sternoclavicular and acromioclavicular joints. The M5 StickC Plus should be perpendicular to the clavicle, ensuring accurate signal detection.

<p align="center">
  <img src="https://github.com/mai-amber/Media/blob/1bce66f0f789bc35341d74f84794446841127932/sensor%202.jpg" width="400"/>
</p>

---

For more information on setup, operation, and data interpretation, refer to the detailed sections above. This system represents a significant step forward in patient monitoring technology, providing a user-friendly and efficient solution for real-time respiratory monitoring.
