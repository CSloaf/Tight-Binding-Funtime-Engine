# Tight Binding Funtime Engine

The Tight Binding Funtime Engine is a lightweight graphical application for calculating and visualising quantum transmission spectra of tight-binding/Hückel type systems.

## About

The Tight Binding Funtime Engine provides a fast, user-friendly interface for quantum transport simulations. It calculates the transmission probability T(E) across a device using the Non-Equilibrium Green's Function (NEGF) formalism. It makes use of 1D semi-infinite chain type electrodes to capture the energy dependence of coupling and resonance positions.  

By abstracting away the scripting required by library based solutions, users can visually analyse data on the fly with multiple input files processable through an automated batch-processing queue.  

## Features

* **Interactive Visualizer:** Real-time, semi-log plotting of transmission spectra via Dear ImGui and ImPlot.

* **Batch Processing:** By pointing the program to a folder of Hamiltonians & setting a visual delay timer, the program automatically calculates and plots data sequentially.

* **Drag-and-Drop Workflow:** Seamlessly drop input files directly into the configuration paths.

* **High-Performance Math:** Under the hood, matrix operations and self-energy calculations are driven by Eigen3.

## How to Cite

If you use the TB Funtime Engine software or any of its constituent code in your research or educational materials, please cite this artifact as follows:

> J. M. F. Morris (2026). Tight Binding Funtime Engine: A Graphical Application for Quantum Transmission Spectra (Version 1.0.0). Zenodo. https://doi.org/10.5281/zenodo.

## License

Copyright (C) 2026 James M. F. Morris.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.


## Installation (Recommended for Windows Users)
If you simply want to run the TB Funtime Engine without compiling code, you can download the latest pre-compiled executable directly.

1. Navigate to the **[Releases](https://github.com/CSloaf/Tight-Binding-Funtime-Engine/releases)** page on GitHub.
2. Download the latest `TB_Funtime_Engine_GUI.zip`.
3. Extract the folder and double-click the `.exe` to run the application.

---

## Building from Source (For Developers)

This project uses modern C++20 and CMake. Dependencies are managed via `vcpkg` in manifest mode.

### Prerequisites
* A C++20 compatible compiler (MSVC, GCC, or Clang)
* [CMake](https://cmake.org/) (version 3.20 or higher)
* [vcpkg](https://vcpkg.io/) installed and bootstrapped

### Build Instructions
The easiest way to build the engine is via the command line using standard CMake commands. 

1. Clone this repository to your local machine:
   ```bash
   git clone https://github.com/CSloaf/Tight-Binding-Funtime-Engine.git
   cd Tight-Binding-Funtime-Engine
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```

## Dependencies
The following libraries are automatically fetched and linked via `vcpkg.json`:
* `Eigen3` - Core linear algebra and Green's function solvers
* `ImGui` & `ImPlot` - Graphical interface and plotting
* `GLFW3` & `OpenGL3` - Window and rendering context
* `rapidcsv` & `fmt` - Data parsing and string formatting

## Usage & Input Format
The engine expects standard tab-separated `.txt` files representing the Hamiltonian of the system.  

Input file formatting and examples can be found in `docs/user_guide.md` and the `examples/` folder.  

The user may be interested to explore a prototype Python-based program for generating input files from SMILES strings (generated from ChemDraw by highlighting and copying as smiles via Ctrl+Alt+C).  

I wrote this specifically for the Tight Binding Funtime Engine and it can be found over at The University of Liverpool in our dedicated group repository: [TB_Input Generator: SMILES To Hückel](https://github.com/Molecular-Electronics-Liverpool/SMILES-2-Huckel).

