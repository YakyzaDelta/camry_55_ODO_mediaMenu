# camry_55_ODO_mediaMenu
demo programm esp32+MCP2515 for Camry 50-55 (2014-2017) for activate media menu on combination meter (car's instrument cluster)

# Toyota Camry XV50 (US-spec) Instrument Cluster Navigation & Media Menu Activator

Arduino sketch for **ESP32** + **MCP2515** (using the mcp_can library by coryjfowler) to emulate a OEM 7-inch Toyota head unit and enable extra Navigation and Media screens on the instrument cluster.

---

## 📌 Background

Most owners of US-spec Toyota Camry XV50 models notice that the instrument cluster display (photo 1) typically features only three standard tabs:
1. **Main Information**
2. **Alerts / Notifications**
3. **Settings**

![Standard 3-tab Instrument Cluster Screen](photo1.jpg)

However, after retrofitting an original Toyota OEM 7-inch touchscreen head unit and connecting two additional CAN bus wires (which were missing in the base trim package)—specifically connecting **Pin 18 (MSCH)** on the instrument cluster to **Pin 3 (CNH1)** on the 24-pin radio connector, and **Pin 19 (MSCL)** to **Pin 4 (CNL1)**—two additional menus appeared instantly upon powering up:
- **Navigation** (compass display)
- **Multimedia** (photo 2)

![Instrument Cluster with activated Media & Navigation Tabs](photo2.jpg)

The multimedia tab allowed controlling audio directly from the steering wheel buttons, switching sources (CD, Radio, USB, Bluetooth), and displaying real-time track info, artist name, and album titles.

---

## 🔍 Investigation & Reverse Engineering

After upgrading to an aftermarket Android head unit, these extra menu tabs disappeared. Rather than accepting the loss, I decided to analyze how the system works using the original 7-inch OEM radio.

1. **Initial Tests (Isolated CAN Bus):**
   - Built a standard CAN sniffer using an **ESP32** and **MCP2515** CAN module.
   - Connected to the head unit at the standard Toyota rate of **500 kbps**.
   - Observed non-standard static packets that didn't respond or change when pressing buttons on the radio, indicating that a handshake/authentication with the instrument cluster was required.

2. **Combined Sniffing:**
   - Connected the OEM radio and the instrument cluster together on a test bench while sniffing the total CAN traffic.

![Test bench setup for sniffing CAN bus traffic between Cluster and OEM Head Unit](photo2_setup.jpg)

---

## 💡 How It Works

From sniffing and analyzing the frame dumps:
- **Handshake Protocol:** The instrument cluster sends initiation query frames; the head unit responds to establish the link, after which structured data transmission begins.
- **Heartbeat Requirement:** The cluster periodically checks if the head unit is still active ("heartbeat" packet). If the response is missed or times out, the link resets and the media/navigation tabs disappear.
- **Data Payload:** Text data (source names, song titles, artists, albums) is transmitted directly in **HEX format** with defined header/footer frame boundaries and structure.

---

## 🚀 Project Status & Features

By emulating these specific CAN responses using an **ESP32 + MCP2515**, the cluster can be forced to enable and display both extra menus without needing the original OEM radio.

- [x] CAN bus handshake initialization logic.
- [x] Heartbeat response maintenance.
- [x] Basic text frame formatting demo.
- [x] Working prototype displaying extra menus on bench & car setup.

> **Note:** This code is a proof-of-concept / functional prototype developed as an enthusiast project. It serves as a working base for further community research and development.

---

## 🔮 Future Development Roadmap

- [ ] **Android Head Unit Integration:** Connect ESP32 via Bluetooth / Serial / Android App to stream track metadata (Title, Artist, Album) directly from Android audio apps to the cluster screen.
- [ ] **Navigation Turn-by-Turn Display:** Reverse-engineer the Navigation tab to output direction arrows (supported natively by the cluster display) fed by Google Maps / Waze navigation prompts (photo 3).

![Example concept for Navigation Turn-by-Turn display on Cluster](photo3.jpg)

---

## 🛠️ Hardware Requirements

- **ESP32** Development Board
- **MCP2515** CAN Bus Module (16 MHz / 8 MHz crystal)
- **Toyota Camry XV50 Instrument Cluster** (US-spec)
- CAN connection w/ terminating resistor (500 kbps)

---

## 📚 Connections

![esp32_connection](images/image4.jpg)

---

## 📄 License

Distributed under the MIT License. Feel free to use, modify, and contribute!
