# Turing Machine Simulator - Copying with OpenGL Visualization

This project is a C-based simulation of a Turing Machine designed to copy binary data, with an animated visualization using OpenGL. It is ideal for understanding how Turing Machines work through a graphical representation of tape movement, state transitions, and internal operations.

## 🛠 Features

- 📼 **Binary Tape Simulation** — Input a decimal number, convert to binary, and visualize tape operations.
- 🎞 **OpenGL Visualization** — Real-time animation of the tape and head movement.
- 🔄 **State Transition Diagram** — Graphically represents state changes and transitions.
- 📦 **Data Structures Integration** — Uses stack, queue, and hashtable to simulate internal Turing Machine logic.
- 🖼 **Image Rendering** — Uses textures for better UI/UX experience.

## 📷 Tape Example

- **Before Copy**: `BB1010C`
- **After Copy**: `BB1010C1010BBBB`

## 🧑‍💻 How to Run

### Prerequisites

- GCC compiler (for C)
- OpenGL libraries (`GL`, `GLU`, `GLUT`)
- Make sure `freeglut` is installed if you're on Windows/Linux

### Build & Run

```bash
gcc main.c -o turing -lGL -lGLU -lglut -lm
./turing
```

## Contact
For any questions or feedback, please contact [omhase9955@gmail.com](omhase9955@gmail.com).
