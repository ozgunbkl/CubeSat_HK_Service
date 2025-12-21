# 🛰️ CubeSat Housekeeping (HK) Service

A **deterministic, thread-safe telemetry management subsystem** designed for 1U/3U CubeSats.  

This service acts as the satellite's **central health record**, providing a reliable interface for storing, validating, and serializing system health data for downlink to Earth.

---

## 🚀 Overview

In a Flight Software (FSW) stack, the Housekeeping (HK) service is mission-critical. It enables the spacecraft to understand its own health and make safe decisions in real time.

This implementation prioritizes:
- **Determinism** (constant-time access)
- **Memory safety**
- **Fault detection and data validity**
- **Compatibility** with a FreeRTOS-based, multi-tasking environment

Multiple subsystems (EPS, Thermal, ADCS, Payload) can safely report telemetry concurrently through a controlled API.

---

## ✨ Key Engineering Features

### 1. Private Telemetry Vault (Data Encapsulation)

Telemetry is stored in a **private static table** inside the implementation file (`.c`).

- Prevents unsafe global variable access
- Enforces controlled interaction through a defined API
- Eliminates accidental corruption by unrelated subsystems

This mirrors real flight software design, where shared state must be tightly controlled.

---

### 2. O(1) Deterministic Access

Each telemetry parameter is accessed using an **enum-based Parameter ID** as a direct array index.

- No string comparisons
- No dynamic lookups
- No linked lists

Every read/write operation executes in **constant time**, ensuring predictable behavior—essential for real-time flight stability.

---

### 3. Integrated FDIR (Fault Detection, Isolation, and Recovery)

The HK service actively monitors telemetry against predefined safe limits. Each parameter is evaluated into one of three states:

- **NOMINAL (0)**  
  Value is within mission-defined safe limits.

- **OUT_OF_BOUNDS (1)**  
  High/low thresholds exceeded.  
  → Triggers automated safety or mitigation actions.

- **STALE / INVALID (-1)**  
  Sensor has not reported or data is invalid.  
  → Prevents decisions based on "blind" or outdated data.

This triple-state logic allows the system to distinguish between *faulty data* and *missing data*, a key requirement in safety-critical systems.

---

### 4. Space-Grade Big-Endian Serialization

To prepare telemetry for downlink, the service serializes internal data structures into a compact byte stream.

- **Bit Shifting:**  
  32-bit values are manually decomposed into 8-bit segments.

- **Big-Endian Format:**  
  Most Significant Byte first, ensuring platform-independent decoding on the Ground Station.

This avoids inefficient text-based telemetry and reflects real spacecraft communication constraints.

---

## 🛠️ Project Structure
```
include/
└── hk_service.h    # Public API, Parameter IDs, packet definitions

lib/
└── hk_service.c    # Core storage, limit checking, serialization logic

test/
└── test_hk.c       # Unit tests for all mission-critical paths
```

---

## 🧪 Verification & Validation

All logic is verified using the **Unity Test Framework** prior to integration.

### Test Coverage

- `test_HK_InitialStateIsInvalid` — PASS  
- `test_HK_UpdateAndReadSuccess` — PASS  
- `test_HK_RejectsInvalidIDs` — PASS  
- `test_HK_CheckLimitsReturnsErrorForStaleData` — PASS  
- `test_HK_DifferenceBetweenNominalAndStale` — PASS  
- `test_HK_SerializationPacksMultipleParams` — PASS  

**Final Status:**  
✅ **6 Tests · 0 Failures · 0 Ignored**

---

## 📘 Summary

This Housekeeping service demonstrates:

- Deterministic real-time design
- Safe shared-state management
- Integrated FDIR logic
- Low-level binary serialization
- Verification-driven development for CubeSat flight software

It is designed to integrate directly with higher-level subsystems such as TMTC, EPS, and ADCS within a full FSW stack.
