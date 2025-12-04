# Differences Between Original CKPOOL and CKPOOL-LHR Fork

This document summarizes the key differences between the original CKPOOL repository
and the CKPOOL-LHR fork.

## Overview

The CKPOOL-LHR fork is based on an earlier version of CKPOOL (commit 3a6da1fa) and
includes modifications to support sub-"1" difficulty values for low hash rate miners,
along with other enhancements and changes.

## Core Changes

### 1. Sub-"1" Difficulty Support (LHR Feature)

**Purpose**: Enable difficulty values below 1.0 for low hash rate miners (ESP32
devices and other embedded systems).

**Implementation**:
- `mindiff` and `startdiff` are `double` type in `ckpool.h` (support sub-"1" values)
- Difficulty-related variables are `double` type:
  - `client->diff`, `client->old_diff`, `client->suggest_diff`
  - `client->ssdc` (shares since diff change)
  - Various share counting variables (`shares`, `best_diff`, `best_ever`)
- Uses `json_get_double()` for `mindiff` and `startdiff` parsing in `ckpool.c`
- Automatic vardiff adjustment prevents reducing difficulty below 1
  (line 5734 in `stratifier.c`: `if (unlikely(optimal < 1)) return;`)
- Manually configured `mindiff` and initial `startdiff` values below 1 are
  accepted and applied correctly

**Relevant Files**:
- `src/ckpool.h` - Type definitions
- `src/ckpool.c` - Configuration parsing
- `src/stratifier.c` - Difficulty handling and vardiff logic
- `src/generator.c` - Difficulty calculations
- `src/libckpool.c` - Library functions

### 2. User Agent Whitelisting

**Purpose**: Optionally restrict which mining software can connect to the pool.

**Implementation**:
- `useragent` array and `useragents` count in `ckpool_t` structure
- `parse_useragents()` function in `ckpool.c` for configuration parsing
- User agent validation in `stratifier.c` subscription handling
- `useragent` field in worker instances for persistence
- Uses prefix matching via `safencmp` function

**Behavior**:
- **Whitelist not configured** (missing or empty array): All user agents allowed,
  including empty strings (matches original CKPOOL behavior)
- **Whitelist configured**: User agent must match a whitelist entry using prefix matching,
  otherwise connection is rejected. Empty user agents are rejected (they don't match any pattern)

**Relevant Files**:
- `src/ckpool.h` - Structure definitions
- `src/ckpool.c` - Configuration parsing
- `src/stratifier.c` - Subscription validation and worker tracking

### 3. Donation System Default Address

**Difference**: Default `donaddress` value differs from official CKPOOL.

- **CKPOOL-LHR default**: `bc1q8qkesw5kyplv7hdxyseqls5m78w5tqdfd40lf5`
- **Official CKPOOL default**: `bc1q28kkr5hk4gnqe3evma6runjrd2pvqyp8fpwfzu`

The donation system implementation is otherwise identical (both use `double` type for `donation`, both have configurable `donaddress`).

### 4. Bitcoind Cookie Authentication Support

**Purpose**: Support cookie-based authentication for bitcoind connections.

**Implementation**:
- `cookie` field in bitcoind configuration structure
- `btcdcookie` array in `ckpool_t` structure
- `json_get_configstring()` allows cookie as alternative to auth/pass
- Cookie parsing in `parse_btcds()` function

**Relevant Files**:
- `src/ckpool.h` - Structure definitions
- `src/ckpool.c` - Configuration parsing

### 5. Features Not Present in CKPOOL-LHR

**Missing Features**:
- `dropall` command support (disconnect all clients)
- SHA256 hardware acceleration files:
  - `src/sha256_arm_shani.c` (ARM SHA-NI acceleration)
  - `src/sha256_x86_shani.c` (x86 SHA-NI acceleration) - Note: x86 SHA-NI support exists via merged code

### 6. Documentation

**Additional Documentation Files**:
- `FAQ.md` - Comprehensive documentation of repository structure and workflows
- `README-PROXY` - Proxy mode documentation
- `README-PASSTHROUGH` - Passthrough mode documentation
- `README-NODE` - Node mode documentation
- `README-REDIRECTOR` - Redirector mode documentation
- `README-USERPROXY` - Userproxy mode documentation

**Fork-Specific Documentation**:
- `README` - Fork-focused documentation with sub-"1" difficulty support details
- `README-SOLOMINING` - Solo mining guide
- `AUTHORS` - Separated original authors from fork contributors
- `ChangeLog` - Fork identification


## Version Information

- **CKPOOL-LHR Fork Base**: Commit `3a6da1fa` from original CKPOOL repository

## Build System

CKPOOL-LHR uses autotools build system:
- `configure.ac` - Build configuration
- `Makefile.am` files - Build rules
- Test suite in `tests/` directory

## Configuration File Differences

**Additional Options in CKPOOL-LHR**:
- `useragent` - Array of allowed user agent strings

**Different Behavior**:
- `mindiff` - Accepts double type (sub-"1" values supported)
- `startdiff` - Accepts double type (sub-"1" values supported)
- `donaddress` - Default address differs (see Section 3)

## Summary

The CKPOOL-LHR fork provides:
1. **Low Hash Rate Support**: Sub-"1" difficulty for embedded systems
2. **Security**: User agent whitelisting
3. **Flexibility**: Cookie-based bitcoind authentication
4. **Documentation**: Comprehensive mode-specific READMEs

The fork maintains backward compatibility with standard pool operations while
providing specialized features for low hash rate mining scenarios.

