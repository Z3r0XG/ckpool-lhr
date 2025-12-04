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

**Changes**:
- Changed `mindiff` and `startdiff` from `int64_t` to `double` in `ckpool.h`
- Changed difficulty-related variables from `int64_t` to `double` throughout:
  - `client->diff`, `client->old_diff`, `client->suggest_diff`
  - `client->ssdc` (shares since diff change)
  - Various share counting variables (`shares`, `best_diff`, `best_ever`)
- Modified `json_get_int64()` to `json_get_double()` for `mindiff` and `startdiff`
  in `ckpool.c`
- The automatic vardiff adjustment still prevents reducing difficulty below 1
  (line 5734 in `stratifier.c`: `if (unlikely(optimal < 1)) return;`), but
  manually configured `mindiff` and initial `startdiff` values below 1 are
  accepted and applied correctly

**Files Modified**:
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

**Files Modified**:
- `src/ckpool.h` - Structure definitions
- `src/ckpool.c` - Configuration parsing
- `src/stratifier.c` - Subscription validation and worker tracking

### 3. Donation System

**Purpose**: The donation system is the **only mechanism** in CKPOOL for taking
funds from block rewards. There is no separate pool fee system. The original
design intended this for **software development funding** (as stated in the
README: "the pool by default contributes 0.5% of solved blocks in pool mode to
the development team"), but the same mechanism can be used by pool operators as
a pool fee by setting `donaddress` to their own address.

**How it works**: The donation is deducted from the block reward (coinbase value)
before the remainder is distributed to miners. The donation amount is calculated
as: `(block_reward / 100) * donation`. The remaining value goes to the pool's
miners based on their shares.

**Key Point**: You cannot have both a development donation AND a separate pool fee
- there is only one `donaddress` and one `donation`. Pool operators must choose:
- Set `donaddress` to developer address (support development)
- Set `donaddress` to pool operator address (use as pool fee)
- Set `donation` to 0 (no fee/donation)

**Current Implementation**:
- `donation` is a `double` type supporting fractional percentages (e.g., 0.5%, 1.5%)
- Same as original CKPOOL - no change to donation type or calculation
- `donaddress` is now configurable via JSON config file (was hardcoded in original)
- Default donation address: `bc1q8qkesw5kyplv7hdxyseqls5m78w5tqdfd40lf5` (fork default)
- Original default was: `bc1q28kkr5hk4gnqe3evma6runjrd2pvqyp8fpwfzu` (Con Kolivas's address)
- Validation: Values below 0.1 are treated as 0, values above 99.9 are clamped to 99.9
- The donation is taken from block rewards (coinbase transaction)

**Note on Default Address**: Pool operators should configure `donaddress`
appropriately based on their needs - whether for development funding or as a
pool fee.

**Files Modified**:
- `src/ckpool.h` - Structure definitions
- `src/ckpool.c` - Added `donaddress` configuration parsing and default value
- `src/generator.c` - Donation calculation (unchanged)
- `src/stratifier.c` - Logging (unchanged)

### 4. Bitcoind Cookie Authentication Support

**Purpose**: Support cookie-based authentication for bitcoind connections.

**Changes**:
- Added `cookie` field to bitcoind configuration structure
- Added `btcdcookie` array to `ckpool_t` structure
- Modified `json_get_configstring()` to allow cookie as alternative to auth/pass
- Added cookie parsing in `parse_btcds()` function

**Files Modified**:
- `src/ckpool.h` - Structure definitions
- `src/ckpool.c` - Configuration parsing

### 5. Idle Client Management

**dropidle Feature**:
- Added support for automatically disconnecting idle clients
- Configurable idle timeout in seconds (default: 0 = disabled)
- When enabled, clients idle longer than the threshold are disconnected
- Worker stats are preserved and matched by workername on reconnect
- Safe for low hash rate miners - 1 hour default is very generous

**Removed from CKPOOL-LHR**:
- `dropall` command support (disconnect all clients)
- SHA256 hardware acceleration files:
  - `src/sha256_arm_shani.c` (ARM SHA-NI acceleration)
  - `src/sha256_x86_shani.c` (x86 SHA-NI acceleration)

**Note**: The original CKPOOL has these files, but they may not be actively used
depending on build configuration.

### 6. Documentation Changes

**New Files**:
- `FAQ.md` - Comprehensive documentation of repository structure and workflows
- `README-PROXY` - Proxy mode documentation
- `README-PASSTHROUGH` - Passthrough mode documentation
- `README-NODE` - Node mode documentation
- `README-REDIRECTOR` - Redirector mode documentation
- `README-USERPROXY` - Userproxy mode documentation

**Modified Files**:
- `README` - Reorganized for fork, added sub-"1" difficulty documentation,
  added missing configuration options, reformatted configuration sections
- `README-SOLOMINING` - Updated for fork, streamlined for solo mining focus
- `AUTHORS` - Separated original authors from fork contributors
- `ChangeLog` - Added fork identification

### 7. Code Quality and Type Changes

**Type System Improvements**:
- Changed share counting from `int64_t` to `double` for better precision with
  sub-"1" difficulty shares
- Updated statistics structures to use `double` for share tracking:
  - `unaccounted_shares`, `accounted_shares`
  - `unaccounted_diff_shares`, `accounted_diff_shares`
  - `unaccounted_rejects`, `accounted_rejects`
  - `best_diff` in pool stats

**Files Modified**:
- `src/stratifier.c` - Statistics structures
- `src/ckpool.h` - Type definitions

### 8. Copyright Updates

**Changes**:
- Removed "2025" from copyright notices (fork is based on earlier version)
- Maintained original author attribution (Con Kolivas)

**Files Modified**:
- `src/ckpool.c`
- `src/ckpool.h`
- `src/stratifier.c`

## Version Information

- **Original CKPOOL**: Based on commit `3a6da1fa` (latest in original repo appears
  to be from 2025-12-04)
- **CKPOOL-LHR**: Forked from earlier version, latest commit `38d3d3ea`

## Build System

Both repositories use the same autotools build system. The fork includes:
- Same `configure.ac` structure
- Same `Makefile.am` files
- Additional build artifacts in fork (compiled binaries, object files, etc.)

## Configuration File Differences

**New Options in CKPOOL-LHR**:
- `useragent` - Array of allowed user agent strings
- `donaddress` - Bitcoin address for donations (now configurable, was hardcoded in original)

**New Options in CKPOOL-LHR**:
- `dropidle` - Drop idle clients after specified seconds (default: 0 = disabled)

**Changed Options**:
- `mindiff` - Now accepts double (sub-"1" values supported)
- `startdiff` - Now accepts double (sub-"1" values supported)
- `donaddress` - Now configurable (was hardcoded in original), default address changed

## Summary

The CKPOOL-LHR fork focuses on:
1. **Low Hash Rate Support**: Sub-"1" difficulty for embedded systems
2. **Security**: User agent whitelisting
3. **Flexibility**: Cookie-based bitcoind authentication
4. **Documentation**: Comprehensive mode-specific READMEs
5. **Code Quality**: Improved type system for precision with fractional difficulties

The fork maintains backward compatibility with standard pool operations while
adding specialized features for low hash rate mining scenarios.

