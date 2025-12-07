# Differences Between Original CKPOOL and CKPOOL-LHR Fork

This document summarizes the key differences between the original CKPOOL repository
and the CKPOOL-LHR fork.

## Overview

The CKPOOL-LHR fork includes modifications to support sub-"1" difficulty values for low hash rate miners,
along with other enhancements and changes.

## Core Changes

### 1. Sub-"1" Difficulty Support (LHR Feature)

**Purpose**: Enable difficulty values below 1.0 for low hash rate miners (ESP32
devices and other embedded systems).

**Behavior**:
- `mindiff` and `startdiff` configuration options accept decimal values below 1.0 (e.g., 0.0005, 0.001)
- Manually configured sub-"1" difficulty values are accepted and applied to miners
- Automatic vardiff adjustment will not reduce difficulty below 1.0 (manual configuration required for sub-"1" values)

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

### 4. Configurable User Data Cleanup

**Purpose**: Control how long user and worker statistics are retained on disk.

**Behavior**:
- `user_cleanup_days` configuration option (integer, days)
- Default: `0` (never cleanup - user data persists indefinitely)
- When set to a positive value: User and worker statistics are removed from disk after the specified number of days of inactivity (no shares submitted)
- Provides flexibility: `0` = keep forever, `7` = 1 week, `365` = 1 year, etc.

> [!NOTE]
> Users who have never submitted a share are not saved to disk regardless of this setting.

### 5. Password Field Variable Support

**Purpose**: Allow miners to specify parameters via password field.

**Behavior**:
- Password field supports comma-separated parameter format (e.g., `x, diff=200, f=9`)
- Supported variables:
  - `diff`: Suggest difficulty. Format: `diff=X` where X is a number (e.g., `diff=0.001`).  
    Applied after successful authorization. Supports values between pool minimum (`mindiff`)  
    and maximum (`maxdiff`) difficulty settings
