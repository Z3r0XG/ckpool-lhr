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
- Cookie file path is specified in the bitcoind configuration

### 4. Configurable User Data Cleanup

**Purpose**: Control how long user and worker statistics are retained on disk.

**Behavior**:
- `user_cleanup_days` configuration option (integer, days)
- Default: `0` (never cleanup - user data persists indefinitely)
- When set to a positive value: User and worker statistics are removed from disk after the specified number of days of inactivity (no shares submitted)
- Users who have never submitted a share are always skipped regardless of this setting
- Provides flexibility: `0` = keep forever, `7` = 1 week, `365` = 1 year, etc.

## Configuration File Differences

**Additional Options in CKPOOL-LHR**:
- `useragent` - Array of allowed user agent strings
- `user_cleanup_days` - Days after which inactive user/worker data is cleaned up (0 = never)

**Different Behavior**:
- `mindiff` - Accepts double type (sub-"1" values supported)
- `startdiff` - Accepts double type (sub-"1" values supported)

## Summary

The CKPOOL-LHR fork provides:
1. **Low Hash Rate Support**: Sub-"1" difficulty for embedded systems
2. **Security**: User agent whitelisting
3. **Flexibility**: Cookie-based bitcoind authentication
4. **Resource Management**: Configurable user data cleanup

The fork maintains backward compatibility with standard pool operations while
providing specialized features for low hash rate mining scenarios.

