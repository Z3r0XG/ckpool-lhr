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
- Configuration options accept fractional values below 1 (e.g., 0.0005, 0.001)
- Values >= 1 are automatically rounded to the nearest whole number (e.g., 1.3 → 1, 42.5 → 43)
- Vardiff algorithm performs smooth adjustments using floating-point calculations (e.g., optimal = DSPS × 3.33)
- Configuration validation:
  - Negative values are rejected (pool exits with error)
  - Zero values apply sensible defaults (mindiff=1, startdiff=42, highdiff=1000000)
  - Values below 0.001 trigger performance warnings (but are accepted)
- Automatic vardiff adjustment can now reduce difficulty below 1 based on hashrate

### 2. Whole-Number Difficulty Emission

**Purpose**: Improve ASIC compatibility by emitting whole-number diffs as integers while retaining fractional math internally.

**Behavior**:
- Diff values >= 1 are emitted in JSON as integers (not floats)
- Internal calculations still use floating point with DIFF_EPSILON standardization (1e-6)
- Rounding uses round() for clarity and consistency

### 3. User Agent Whitelisting

**Purpose**: Optionally restrict which mining software can connect to the pool.

**Behavior**:
- **Whitelist not configured** (missing or empty array): All user agents allowed, including empty strings
- **Whitelist configured**: User agent must match a whitelist entry using prefix matching, otherwise connection is rejected. Empty user agents are rejected (they don't match any pattern)

### 4. User Agent Aggregation & Exposure

**Purpose**: Expose normalized user-agent usage in operator-facing endpoints.

**Behavior**:
- Pool aggregates normalized UA strings and publishes counts in pool/pool.status (capped by max_pool_useragents; 0 disables publishing)
- Individual user/worker pages show the specific useragent string for each worker

### 5. Zombie/Ghost Cleanup Improvements

**Purpose**: Faster eviction of orphaned/ghost clients and safer refcount handling.

**Behavior**:
- Tightened refcount invariant checks and lazy invalidation paths
- Improved connector/stratifier cleanup to avoid stale clients and FD reuse hazards
- Enhanced LOGDEBUG instrumentation to investigate zombie scenarios (production-safe)
- Automatic behavior (no new config); distinct from dropidle, which remains user-tunable

### 6. Bitcoind Cookie Authentication Support

**Purpose**: Support cookie-based authentication for bitcoind connections.

**Behavior**:
- Bitcoind connections can use cookie-based authentication as an alternative to username/password
- Add `"cookie" : "/path/to/.cookie"` to the btcd entry in ckpool.conf
- Cookie file location: `~/.bitcoin/.cookie` (Linux default) or `<datadir>/.cookie` if custom datadir is set

### 7. Pool Status Enhancements

**Purpose**: Expose additional operational metrics and identify blocks mined by this fork.

**Behavior**:
- **Network difficulty in pool.status**: Current Bitcoin network difficulty exposed via `netdiff` field for monitoring blockchain state
- **Worker connection timestamps**: Worker connection time persisted as `started` field (Unix timestamp) in `logs/users/*.json` for session tracking; maintains backward compatibility with legacy `connected` field

### 8. Password Field Variable Support

**Purpose**: Allow miners to provide parameters via the password field when their miner cannot send `mining.suggest_difficulty`.

**Behavior**:
- Password supports comma-separated parameters (e.g., `x, diff=200, f=9`).
- Supported variable:
  - `diff`: Suggest difficulty. Format: `diff=X` where `X` is numeric (e.g., `diff=0.001`). Applied after successful authorization and clamped to pool `mindiff`.



