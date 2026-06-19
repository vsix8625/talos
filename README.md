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
git clone --recurse-submodules https://github.com/vsix8625/talos.git   
cd talos
```

```bash
sk init strike
```

### Fan Control (optional)

Talos can adjust fan profiles on supported hardware via a privileged
helper binary. This requires an extra install step beyond the normal
build, since changing fan settings needs elevated permissions.

1. Build as usual
2. Install the fan control helper and its Polkit policy:

```bash
sudo ./scripts/install.sh
```

3. Press F4 in Talos to cycle fan profiles.

Talos automatically probes your hardware at startup to detect which
fan modes are actually supported — not all systems expose controllable
fan profiles, in which case the fan control feature will be silently
disabled.

To remove the helper and policy:

```bash
sudo ./scripts/uninstall.sh
```

**Note:** fan control support varies significantly by hardware/driver
(tested working: HP `hp-wmi` with 2 of 6 possible modes). Behavior
on other vendors/chips is unverified.
