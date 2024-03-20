# Seismic Data Acquisition Unit

## Project Overview

The Seismic Data Acquisition Unit is a critical component in earthquake monitoring systems, designed to efficiently collect seismic data from transducers and securely transmit this data to data centers for analysis. This project was developed as part of the SEP400 coursework to demonstrate proficiency in real-time data processing, network communications, and security against potential cyber threats.

## Features

- **Real-Time Data Collection**: Retrieves seismic data from transducers through shared memory, ensuring timely data acquisition.
- **Secure Data Transmission**: Uses INET datagram sockets to transmit seismic data to authenticated data centers, ensuring data integrity and confidentiality.
- **Robust Security Measures**: Implements mechanisms to protect against brute-force password attacks and denial-of-service attacks from rogue data centers.
- **Efficient Data Handling**: Employs mutex synchronization to manage the concurrent data flow between the data acquisition unit and data centers, minimizing data loss and ensuring reliability.

## Installation

1. Clone the repository to your local machine:

    ```
    git clone https://github.com/Fahad-Ali-Khan-ca/Seismic-Data-Acquisition-Unit-.git
    ```

2. Navigate to the project directory:

    ```
    cd Seismic-Data-Acquisition-Unit-
    ```

3. Testing
   - To test the system, code and binaries for the data acquisition unit and the transducer should be in one directory, and code and binaries for the data center in another directory.
   - The start batch file for the transducer and data acquisition unit should be run first, and shortly after the start batch file for the data centers.
   - The start batch file for the data centers will run 4 valid data centers and two rogue data centers.
   - Once communications have been established, you should see data centers 1-4 receiving data from the data center acquisition unit. Data centers 5 and 6 should be blocked.
   - After 30 seconds all data centers shut down.

## Design and Implementation

- **Transducer**: Simulates seismic data generation and writes data to shared memory.
- **Seismic Data Acquisition Unit**: Retrieves data from shared memory and transmits it to subscribed data centers over the network.
- **Data Centers**: Subscribes to the data acquisition unit to receive seismic data streams for analysis.


## Security Features

- **Password Authentication**: Ensures that only authorized data centers can subscribe to receive data.
- **Brute-Force Attack Protection**: Monitors subscription attempts to identify and block rogue entities trying to guess passwords.
- **Denial-of-Service Attack Mitigation**: Tracks data center activity to prevent overwhelming the data acquisition unit with excessive data.

## Acknowledgments

This project was developed as an assignment for the SEP400 course. Special thanks to the course instructors and peers for their support and guidance throughout the project development.
