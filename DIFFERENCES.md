# Differences Between Original CKPOOL and CKPOOL-LHR Fork

This document summarizes the key differences between the original CKPOOL repository
and the CKPOOL-LHR fork.

## Overview

The CKPOOL-LHR fork includes modifications to support sub-"1" difficulty values for low hash rate miners,
along with other enhancements and changes.

## Core Changes

### 1. Fractional Difficulty Support (LHR Feature)

**Purpose**: Enable fractional difficulty values (including sub-1.0) for low hash rate miners (ESP32
devices and other embedded systems).

**Behavior**:
- All difficulty settings (`mindiff`, `startdiff`, `highdiff`, `maxdiff`) changed from integer to double precision floating point
- Configuration options accept fractional values below 1.0 (e.g., 0.0005, 0.001)
- Values >= 1 are automatically rounded to the nearest whole number (e.g., 1.3 → 1, 42.5 → 43)
- Vardiff algorithm performs smooth adjustments using floating-point calculations (e.g., optimal = DSPS × 3.33)
- Configuration validation:
  - Negative values are rejected (pool exits with error)
  - Zero values apply sensible defaults (mindiff=1, startdiff=42, highdiff=1000000)
  - Values below 0.001 trigger performance warnings (but are accepted)
- Automatic vardiff adjustment can now reduce difficulty below 1.0 based on hashrate
- Difficulty values sent to miners as JSON integers for whole numbers >= 1 (e.g., 128 not 128.0)

### 2. User Agent Whitelisting

**Purpose**: Optionally restrict which mining software can connect to the pool.

**Behavior**:
- **Whitelist not configured** (missing or empty array): All user agents allowed,
  including empty strings
- **Whitelist configured**: User agent must match a whitelist entry using prefix matching,
  otherwise connection is rejected. Empty user agents are rejected (they don't match any pattern)

### 3. Bitcoind Cookie Authentication Support

**Purpose**: Support cookie-based authentication for bitcoind connections.

**Behavior**:
- Bitcoind connections can use cookie-based authentication as an alternative to username/password
- Add `"cookie" : "/path/to/.cookie"` to the btcd entry in ckpool.conf
- Cookie file location: `~/.bitcoin/.cookie` (Linux default) or `<datadir>/.cookie` if custom datadir is set



