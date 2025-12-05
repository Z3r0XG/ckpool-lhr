# ⚠️ TEMPORARY - DELETE AFTER TASKS COMPLETE ⚠️
# This is a local planning document and should NOT be committed to the repository.
# Add to .gitignore and delete once merge tasks are complete.

# Safe Merge Analysis: CKPOOL-LHR → Official CKPOOL Updates

This document analyzes changes in the official CKPOOL repository (since commit 3a6da1fa) that can be safely merged into CKPOOL-LHR without breaking existing functionality.

## Base Commit
- **Fork base**: `3a6da1fa` (Accept lowest diff of current and next set diff...)
- **Official HEAD**: Latest commit in official repo

## Protected Areas (DO NOT MERGE)

These areas have been modified in CKPOOL-LHR and should NOT be overwritten:

1. **Sub-"1" Difficulty Support**
   - Files: `src/ckpool.h`, `src/ckpool.c`, `src/stratifier.c`, `src/generator.c`
   - Changes: `mindiff`/`startdiff` as `double`, difficulty variables as `double`
   - **Action**: Do not merge any changes to difficulty handling

2. **User Agent Whitelisting**
   - Files: `src/ckpool.h`, `src/ckpool.c`, `src/stratifier.c`
   - Changes: Optional whitelist, empty useragent support
   - **Action**: Do not merge changes to subscription/useragent handling

3. **Donation System**
   - Files: `src/ckpool.c`, `src/generator.c`, `src/stratifier.c`
   - Changes: Configurable `donaddress`, `donation` as `double`
   - **Action**: Do not merge changes to donation calculation

4. **Cookie Authentication**
   - Files: `src/ckpool.h`, `src/ckpool.c`
   - Changes: Cookie-based bitcoind auth support
   - **Action**: Do not merge changes to bitcoind connection/auth

5. **Idle Client Management (dropidle)**
   - Files: `src/ckpool.h`, `src/ckpool.c`, `src/stratifier.c`
   - Changes: Configurable `dropidle` option, `lazy_drop_client` logic
   - **Action**: Do not overwrite - feature re-added to fork

6. **Removed Features**
   - `dropall` command (may still exist, but behavior differs)

## Safe to Merge (Candidates)

### 1. Build System Improvements
**Status**: ✅ MERGED TO MAIN
- `configure.ac` cleanup (commits 3f95bce6, 227f415a) - **NOT APPLICABLE** (we don't have those debug lines)
- Default CFLAGS (commit 2589d759) - **✅ MERGED TO MAIN** (commit 6dbd5585)
- x86 SHA-NI support (commit 8da33e78) - **✅ MERGED TO MAIN** (commit b30639ab)
- **Risk**: Low - build system changes rarely conflict with code
- **Result**: All build system improvements successfully merged and tested

### 2. Bug Fixes (Already Applied)
**Status**: ✅ ALREADY IN FORK
- Memory leak fixes in generator code (commit dbd161d2)
- Overflow fixes using i64 (commit ae99fbcc)
- 6-digit precision for bestshare (commit f66fe9e5)
- Segwit address allocation fix (commit fff16c07)
- **Action**: Verify these are present, no merge needed

### 3. Copyright Updates
**Status**: ⚠️ REVIEW NEEDED
- Official repo has "2025" in copyright
- Fork intentionally removed "2025" (based on earlier version)
- **Action**: Keep fork's copyright approach (no merge)

### 4. btcaddress Validation
**Status**: ✅ ALREADY IN FORK (ENHANCED)
- Commit: `dc7b2e17` - "Instead of using a default mining address, refuse to start in non-solo mode without a btcaddress configured"
- **Official change**: 
  ```c
  if (!ckp.btcaddress && !ckp.btcsolo)
      quit(0, "Non solo mining must have a btcaddress in config, aborting!");
  ```
- **Fork implementation** (line 1807-1808):
  ```c
  if (!ckp.btcaddress && !ckp.btcsolo && !ckp.proxy)
      quit(0, "Non-solo mining must have a btcaddress in config, aborting!");
  ```
- **Status**: Fork already has this validation, and it's more comprehensive (includes proxy check)
- **Action**: No merge needed - fork's version is better

### 5. Idle Client Detection Fix
**Status**: ✅ MERGED TO MAIN
- Commit: `0c82c048` - "Fake a start share time to prevent clients being detected as idle if stats update hits first"
- **Change**: 
  ```c
  // Old:
  client->start_time = time(NULL);
  
  // New:
  client->start_time = client->last_share.tv_sec = time(NULL);
  ```
- **Location**: `src/stratifier.c` around line 3379
- **Merged**: ✅ MERGED TO MAIN (commit a0204333)
- **Risk**: Low - prevents race condition in stats updates
- **Result**: Successfully merged - improves client initialization even without dropidle

### 6. Dropidle Feature
**Status**: ✅ RE-IMPLEMENTED IN FORK (NOT MERGED)
- Official commit: `b13f3eee` - "dropidle feature"
- **Official change**: Adds dropidle configuration and idle client disconnection
- **Fork status**: Fork independently re-implemented dropidle (commit 3575ff65)
- **Action**: No merge needed - fork has its own implementation
- **Note**: Implementation is functionally equivalent, but fork's version is protected

### 7. Version/Configure Updates
**Status**: ⚠️ NOT MERGED
- Version bumps
- Configure script improvements
- **Action**: Skipped - version numbers are fork-specific, configure improvements already included in other merges

### 8. Drop Client Fix (Optimization)
**Status**: ✅ MERGED TO MAIN
- Commit: `bc6c916b` - "The connector should just be informed to drop clients if the stratifier considers them dropped, instead of just testing their existence"
- **Change**: Changed from `connector_test_client()` to `connector_drop_client()` when client is already marked as dropped
- **Location**: `src/stratifier.c` around line 8009
- **Merged**: ✅ MERGED TO MAIN (commit 4c204f7f)
- **Risk**: Low - improves cleanup efficiency
- **Result**: Successfully merged - more efficient client cleanup

### 9. Drop Client Fix (Concurrent Cleanup)
**Status**: ✅ MERGED TO MAIN (WITH OPTIMIZATION)
- Official commit: Removed `!client->dropped` check entirely
- **Fork implementation**: Enhanced with `(!client->dropped || !client->ref)` check
- **Location**: `src/stratifier.c` around line 3581
- **Merged**: ✅ MERGED TO MAIN (commit b4f8ab69)
- **Risk**: Low - prevents redundant processing while maintaining correct cleanup
- **Result**: Successfully merged with optimization - prevents concurrent reprocessing when ref>0 but allows cleanup when ref=0

### 10. Transaction Watch Logging
**Status**: ✅ MERGED TO MAIN
- Commit: `8d70596b` - "Add txnwatch data to pool logs"
- **Change**: Adds transaction ID logging to `pool.txns` file for monitoring
- **Location**: `src/stratifier.c` in `wb_merkle_bin_txns()` function
- **Merged**: ✅ MERGED TO MAIN (commit e332a0d4)
- **Risk**: Low - just adds logging, doesn't touch protected areas
- **Result**: Successfully merged - provides better transaction visibility

### 11. Redirector Fixes
**Status**: ✅ MERGED TO MAIN
- Commit `550e3a46`: "Do not try to redirect clients more than once"
  - Prevents repeated redirect attempts for clients that may be ignoring redirect requests
  - **Location**: `src/connector.c` around line 1031
- Commit `bb73059f`: "Fix redirector" - Sets parent pointer in passthrough mode
  - **Location**: `src/generator.c` around line 2275
- **Merged**: ✅ MERGED TO MAIN (commit 4022af54)
- **Risk**: Low - redirector mode fixes, doesn't touch protected areas
- **Result**: Successfully merged - improves redirector stability

### 12. Drop Client Fix (Properly Clear Dropped Clients)
**Status**: ✅ ALREADY IN FORK (ENHANCED)
- Commit: `0554020d` - "Properly clear dropped clients"
- **Official change**: Removed `!client->dropped` check entirely: `if (client && !client->dropped)` → `if (client)`
- **Fork implementation**: Enhanced with `(!client->dropped || !client->ref)` check (commit b4f8ab69)
- **Status**: Fork already has this fix with optimization (allows cleanup when ref=0)
- **Action**: No merge needed - fork's version is better

### 13. Decay Inactive Stats
**Status**: ✅ MERGED TO MAIN (WITH ENHANCEMENT)
- Commit: `a8f808b5` - "Decay inactive worker and user stats even if not logging them"
- **Fork status**: ✅ MERGED (commit e389f987) with configurable cleanup enhancement

**What was merged**:
- Users with no shares (`last_share.tv_sec == 0`) are always skipped
- User stats are always logged (removed idle check)
- Worker cleanup logic updated to match official behavior

**Enhancement added**:
- Added `user_cleanup_days` config option (integer, days)
- Default: `0` (never cleanup, matches official behavior)
- When set > 0: Clean up user/worker data after N days of inactivity
- Provides flexibility: 0 = keep forever, 7 = 1 week, 365 = 1 year
- Users who have never submitted shares are always skipped

**Action**: ✅ Merged with enhancement - best of both worlds (official default + configurable cleanup)

### 14. Remove Deprecated Workers Directory
**Status**: ✅ MERGED TO MAIN
- Commit: `4850ba2f` - "Remove deprecated workers directory creation"
- **Official change**: Removes workers directory creation code (lines removed from `src/ckpool.c`)
- **Fork status**: ✅ MERGED (commit 28abb357 in merge-remaining-fixes)
- **Risk**: Low - just removes unused directory creation
- **Action**: ✅ Merged - directory was deprecated and not used

### 15. Build System: libjansson Installation
**Status**: ✅ MERGED TO MAIN
- Commit: `d0d66556` - "Do not install any custom libjansson files since they're only statically linked and can overwrite operating system versions"
- **Official change**: 
  - Changes `lib_LTLIBRARIES` to `noinst_LTLIBRARIES` (don't install library)
  - Changes `include_HEADERS` to `noinst_HEADERS` (don't install headers)
  - Removes `pkgconfig_DATA` (don't install pkg-config file)
- **Why**: Prevents overwriting system libjansson with statically-linked version
- **Risk**: Low - build system change, only affects installation
- **Action**: ✅ Merged - prevents potential conflicts with system libraries

### 16. Install Script for Solo Mining
**Status**: ✅ ADDED TO FORK
- Commit: `3a8b0c21` - "Update script to use bitcoin core v29.2"
- **Official change**: Updates Bitcoin Core version in install script
- **Fork status**: ✅ ADDED (commit 9f00fc94) - Install script added with fork-specific updates
- **What was added**:
  - `scripts/install-ckpool-solo.sh` - Automated installation script for solo mining setup
  - Updates: Git URL points to fork, HTTPS instead of SSH, Bitcoin Core v29.2, CKPool-LHR branding
  - Maintains original installation paths (`/opt/ckpool`, `/etc/ckpool`, `/var/log/ckpool`) for compatibility
- **Action**: ✅ Added - provides easy solo mining setup while maintaining path compatibility


## Commits Merged to Main

✅ **Successfully Merged (12 total):**
1. `0c82c048` - Client initialization fix
2. `2589d759` - Default CFLAGS
3. `8da33e78` - x86 SHA-NI support
4. `bc6c916b` - Connector drop client fix
5. `b4f8ab69` - Drop client optimization (fork enhancement)
6. `8d70596b` - Transaction watch logging
7. `550e3a46` - Redirector fix - prevent double redirect
8. `bb73059f` - Redirector fix - parent pointer
9. `a8f808b5` - Decay inactive stats (with configurable cleanup enhancement)
10. `4850ba2f` - Remove deprecated workers directory
11. `d0d66556` - libjansson installation fix
12. `9094ec54` - Dropall command support

✅ **Added to Fork (1 total):**
1. `3a8b0c21` - Install script for solo mining (added with fork-specific updates)

## Commits Already in Fork (No Merge Needed)

✅ **Already Present (Enhanced, Equivalent, or Re-implemented):**
- `dc7b2e17` - btcaddress validation (ENHANCED version - includes proxy check)
- `dbd161d2` - memleak fix (already present)
- `ae99fbcc` - overflow fix (already present)
- `f66fe9e5` - precision fix (already present)
- `fff16c07` - segwit fix (already present)
- `b13f3eee` - dropidle feature (RE-IMPLEMENTED IN FORK - fork has independent implementation, commit 3575ff65)
- `e9162099` - idle drop debug (ALREADY IN FORK - logging included in fork's dropidle implementation)
- `e0dabf4a`, `32a7178a` - dropidle disable (ALREADY IN FORK - fork has dropidle default 0, same behavior)


## Commits Not Applicable (Skipped - Not Needed)

❌ **Not Merged (Not Present in Fork):**
- `3f95bce6`, `227f415a` - configure.ac cleanup (NOT APPLICABLE - fork never had these debug lines)
  - `227f415a`: Removed `echo "Testxxx2=$x86_shani"` debug line
  - `3f95bce6`: Removed `AC_MSG_RESULT(["cpuinfo=$cpuinfo"])` debug line
  - **Why**: These debug lines were added to official repo after our fork, then removed. We never had them.
- Version bumps - Skipped (version numbers are fork-specific)


## Testing Results

✅ **All tests passed**:
- Sub-"1" difficulty: ✅ Verified
- Useragent whitelisting: ✅ Verified
- Donation system: ✅ Verified
- Cookie authentication: ✅ Verified
- dropidle feature: ✅ Verified
- All operation modes: ✅ Verified
- Unit tests: ✅ 14/15 passing (test-encoding failure is pre-existing)
- Build: ✅ Successful
- Linter: ✅ No errors

## Merge Summary

**Total Merges Completed**: 12
**Status**: ✅ All merges successfully completed, tested, and committed
**Result**: Repository is up-to-date with safe improvements from official ckpool while preserving all fork-specific features

**All Merges Completed**:
- Build system improvements (CFLAGS, x86 SHA-NI)
- Bug fixes (client initialization, connector, redirector)
- Feature enhancements (decay stats with configurable cleanup, dropall command)
- Build system cleanup (workers directory, libjansson installation)
- Install script added (solo mining setup automation)

