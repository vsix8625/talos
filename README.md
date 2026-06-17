# Talos System Monitor

A native Linux system monitor.  

## Prerequisites

### 1. Toolchains & Compilers
Verified working on:
* **GCC:** `16.1.1` or newer
* **Clang:** `22.1.6` or newer
* **Linker:** `mold` (Recommended)

### 2. Dependencies
Ensure the following core packages are installed via your package manager (e.g., `pacman`, `apt`):
* **SDL3** (Development headers and library files)

### 3. Build Engine
* **Storm-Knell (`sk`):** Version `>= 0.8.x`.   

---

## Quick Start & Build Pipeline

Run:

```bash
git clone --recurse-submodules https://github.com/vsix8625/talos
cd talos

sk init strike
```
