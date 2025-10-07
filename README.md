# Seismic Data Acquisition Unit

## 🧭 Overview

The **Seismic Data Acquisition Unit (SDAU)** is a core subsystem in earthquake monitoring architectures, designed to **collect, process, and transmit real-time seismic data** from field transducers to centralized data centers for analysis.

This project was developed as part of the **SEP400 (Software Engineering Project)** coursework to demonstrate competencies in **real-time systems design, network programming, concurrent data handling**, and **cybersecurity resilience** against unauthorized access.

---

## ⚙️ Key Features

* **Real-Time Data Collection**
  Continuously retrieves seismic readings from transducers via shared memory, ensuring timely acquisition of seismic signals.

* **Secure Network Transmission**
  Utilizes **INET datagram sockets** for transmitting seismic packets to authenticated data centers with built-in **integrity and confidentiality mechanisms**.

* **Concurrent Data Management**
  Employs **mutex synchronization** to manage data flow between acquisition and transmission threads, minimizing latency and preventing data collisions.

* **Robust Security Mechanisms**
  Implements password-based authentication, brute-force detection, and **denial-of-service (DoS)** mitigation to block rogue or excessive connection attempts.

---

## 🧩 System Architecture

| Component                                | Description                                                                                              |
| ---------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| **Transducer**                           | Simulates seismic activity and writes data to shared memory.                                             |
| **Seismic Data Acquisition Unit (SDAU)** | Retrieves data from shared memory and securely transmits it to authorized data centers.                  |
| **Data Centers**                         | Subscribe to the SDAU and receive validated seismic data streams for further analysis and visualization. |

---

## 🔐 Security Highlights

* **Password Authentication:** Ensures only verified data centers can subscribe to the SDAU.
* **Brute-Force Protection:** Monitors and blocks repeated failed authentication attempts.
* **DoS Mitigation:** Detects and isolates data centers exhibiting malicious or excessive request behavior.

---

## 🧪 Installation & Testing

### 1. Clone the Repository

```bash
git clone https://github.com/Fahad-Ali-Khan-ca/Seismic-Data-Acquisition-Unit-.git
cd Seismic-Data-Acquisition-Unit-
```

### 2. Project Setup

* Place **transducer** and **data acquisition unit** code/binaries in the same directory.
* Place **data center** code/binaries in a separate directory.

### 3. Run the Simulation

* Run the **start batch file** for the transducer and data acquisition unit first.
* After a short delay, run the **start batch file** for the data centers.

### 4. Expected Output

* Data Centers **1–4** (valid) will receive data streams.
* Data Centers **5–6** (rogue) will be automatically blocked.
* After ~30 seconds, all components terminate safely.

---

## 🧱 Design Principles

* **Modularity:** Each subsystem (transducer, acquisition, and data center) operates independently.
* **Scalability:** Supports multiple concurrent data centers without loss of data fidelity.
* **Security-first Design:** Authentication and monitoring embedded in the communication layer.

---

## 📚 Acknowledgments

Developed as part of **SEP400: Software Engineering Project** at **Seneca Polytechnic**.
Special thanks to the course instructors and peers for their valuable feedback and collaboration.
