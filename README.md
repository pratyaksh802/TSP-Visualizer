# 🧭 TSP Visualizer (C++ + SFML)

An interactive visualizer for the **Travelling Salesman Problem (TSP)** built using **C++ and SFML**.
This project allows users to place cities on a canvas and visualize how the algorithm computes a tour connecting all cities.

---

## 🎥 Demo

*(Add a GIF or short video here showing city placement and path visualization)*

---

## ✨ Features

* 🖱️ Click to add cities dynamically
* 🔄 Real-time path visualization
* ⚡ Interactive solving using keyboard input
* 🎨 Clean canvas + side panel UI
* 📊 Displays status and updates during execution

---

## 🎮 Controls

| Key / Action | Function         |
| ------------ | ---------------- |
| Left Click   | Add a city       |
| Enter        | Solve TSP        |
| R            | Reset all cities |
| ESC          | Exit application |

---

## ⚙️ Tech Stack

* **C++ (C++17)**
* **SFML 3.x** (Graphics & Window handling)

---

## 🛠️ Setup & Installation

### 1. Install SFML 3

Make sure SFML 3.x is installed and properly linked.

### 2. Clone the Repository

```bash
git clone https://github.com/your-username/tsp-visualizer.git
cd tsp-visualizer
```

### 3. Compile

```bash
g++ -std=c++17 src/main.cpp -o tsp -lsfml-graphics -lsfml-window -lsfml-system
```

### 4. Run

```bash
./tsp
```

---

## 📊 How It Works

1. User places cities using mouse clicks
2. The algorithm computes a tour connecting all cities
3. The path is rendered visually on the canvas

---

## 📌 Notes

* Designed for **educational and visualization purposes**
* Performance is best with a **moderate number of cities**
* Requires SFML 3 (not compatible with SFML 2 APIs)

---

## 🚀 Future Improvements

* Add multiple algorithms (Nearest Neighbor, Genetic Algorithm, etc.)
* Compare performance between approaches
* Add speed controls and animations
* Improve UI/UX with better controls

---

## 📁 Project Structure

```
tsp-visualizer/
├── src/
├── include/
├── assets/
├── README.md
```

---

## 🤝 Contributing

Feel free to fork the repo and submit improvements or new algorithms.

---

## 📜 License

This project is open-source and available under the MIT License.
