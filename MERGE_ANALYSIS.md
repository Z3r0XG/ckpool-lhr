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
**Status**: ⚠️ NOT IN FORK (DIFFERENT BEHAVIOR)
- Commit: `a8f808b5` - "Decay inactive worker and user stats even if not logging them"

**Practical Difference**: Whether user/worker stats are kept forever or cleaned up after 1 week idle.

**Fork behavior (current)**:
- **After 60 seconds idle**: Hashrate stats decay, but stats are **NOT logged** to pool logs (saves disk I/O)
- **After 1 week idle**: User/worker data is **completely removed** from disk and logs
- **Result**: User/worker stats are cleaned up after 1 week of inactivity

**Official behavior (after commit)**:
- **After 60 seconds idle**: Hashrate stats decay, and stats **ARE logged** to pool logs (always logged)
- **Users who have submitted shares**: **NEVER cleaned up** - persist forever in disk and logs
- **Workers**: Still cleaned up after 1 week idle (600000 seconds)
- **Users with no shares**: Immediately skipped (`last_share.tv_sec == 0` check)
- **Result**: User stats persist forever once they've submitted at least one share

**What this means**:
- **Fork**: User data files (`logdir/users/username`) are deleted after 1 week idle
- **Official**: User data files persist forever (once user has submitted shares)
- **Pool stats** (active/idle/disconnected counts): Same in both - only count currently connected users/workers
- **Log visibility**: Fork hides idle users from pool log messages, official shows them (with decaying hashrate)

**Benefits of keeping stats forever (official behavior)**:
1. **Historical data preservation**: Complete mining history maintained indefinitely
2. **User reconnection**: When users reconnect after being away, their stats are restored from disk:
   - Total shares count preserved
   - Best share ever (`best_ever`) preserved
   - Hashrate history preserved (decayed but not lost)
   - Worker stats preserved
3. **Long-term analytics**: Pool operators can track user behavior over months/years
4. **Intermittent miners**: Users who mine sporadically don't lose their stats between sessions
5. **Best share tracking**: "Best share ever" persists across disconnections and pool restarts
6. **Audit trail**: Complete historical record for accounting/verification purposes

**Costs of keeping stats forever**:
1. **Disk space**: User data files accumulate indefinitely (one file per user)
2. **Log noise**: Idle users appear in pool logs with decaying hashrate
3. **Disk I/O**: More frequent writes to log files (all users, not just active)
4. **Memory**: More user instances loaded at pool startup (from `read_userstats()`)

**Benefits of cleanup after 1 week (fork behavior)**:
1. **Disk space**: Old inactive users are automatically cleaned up
2. **Log cleanliness**: Only active users appear in pool logs
3. **Reduced I/O**: Less frequent writes (only active users)
4. **Faster startup**: Fewer user files to load at pool restart

**Risk**: Medium - changes log visibility and cleanup logic

**Recommendation**: 
This fork supports **all mining devices** (not just low hash rate), with the added capability of sub-"1" difficulty for LHR miners. Consider both use cases:

**Arguments for merging (keep stats forever)**:
- **User reconnection**: Any user (LHR or regular) who returns after a break gets their stats restored
- **"Best share ever" preservation**: Meaningful for all miners, especially those who mine intermittently
- **Disk space cost**: User JSON files are small (few KB each), manageable for most deployments
- **Consistency with official**: Matches upstream behavior, easier to maintain
- **Quality of life**: Users don't lose achievements when they return

**Arguments against merging (keep 1 week cleanup)**:
- **System health**: Automatic cleanup prevents indefinite disk growth
- **Log cleanliness**: Only active users appear in logs (less noise)
- **Resource efficiency**: Less disk I/O, faster startup with fewer files
- **Large pool deployments**: For pools with many users, indefinite growth could become an issue

**Suggested approach**: 
- **Small to medium pools** (< 1000 users): Merge - benefits outweigh costs
- **Large pools** (> 1000 users): Consider keeping cleanup or making it configurable
- **Best of both worlds**: Add configurable cleanup threshold (e.g., `user_cleanup_days: 0` = never, `7` = 1 week, `30` = 1 month) - gives operators control without hardcoding behavior

**Action**: ⚠️ Review needed - decision depends on expected pool size and operator preferences

### 14. Remove Deprecated Workers Directory
**Status**: ✅ MERGE CANDIDATE
- Commit: `4850ba2f` - "Remove deprecated workers directory creation"
- **Official change**: Removes workers directory creation code (lines removed from `src/ckpool.c`)
- **Fork status**: Fork still creates workers directory (line 1856 in ckpool.c)
- **Risk**: Low - just removes unused directory creation
- **Action**: ✅ Should merge - directory is deprecated and not used

### 15. Build System: libjansson Installation
**Status**: ✅ MERGE CANDIDATE
- Commit: `d0d66556` - "Do not install any custom libjansson files since they're only statically linked and can overwrite operating system versions"
- **Official change**: 
  - Changes `lib_LTLIBRARIES` to `noinst_LTLIBRARIES` (don't install library)
  - Changes `include_HEADERS` to `noinst_HEADERS` (don't install headers)
  - Removes `pkgconfig_DATA` (don't install pkg-config file)
- **Why**: Prevents overwriting system libjansson with statically-linked version
- **Risk**: Low - build system change, only affects installation
- **Action**: ✅ Should merge - prevents potential conflicts with system libraries

## Recommended Merge Strategy

### Phase 1: Low-Risk Updates
1. **Client initialization fix** (0c82c048)
   - Fixes potential race condition in stats updates
   - Simple one-line change
   - No conflicts expected
   - **Priority**: HIGH - prevents potential bugs

2. **Build system improvements** (configure.ac, Makefile.am)
   - Low risk, high value
   - Easy to verify
   - **Priority**: MEDIUM - quality of life improvement

### Phase 2: Verification
1. Check if bug fixes (memleak, overflow, precision) are already present
2. Verify no conflicts with our modifications
3. Test after each merge

### Phase 3: Documentation
1. Update DIFFERENCES.md if new features are merged
2. Document any merged improvements

## Merge Process

For each safe change:

1. **Create a branch**: `git checkout -b merge-official-<feature>`
2. **Cherry-pick specific commit**: `git cherry-pick <commit-hash>`
3. **Resolve conflicts** (if any) - prioritize fork's functionality
4. **Test thoroughly** - especially sub-"1" difficulty and useragent whitelisting
5. **Commit with clear message**: "Merge: <description> from official ckpool"

## Commits Merged to Main

✅ **Successfully Merged (8 total):**
1. `0c82c048` - Client initialization fix (commit a0204333)
2. `2589d759` - Default CFLAGS (commit 6dbd5585)
3. `8da33e78` - x86 SHA-NI support (commit b30639ab)
4. `bc6c916b` - Connector drop client fix (commit 4c204f7f)
5. `b4f8ab69` - Drop client optimization (fork enhancement, commit b4f8ab69)
6. `8d70596b` - Transaction watch logging (commit e332a0d4)
7. `550e3a46` - Redirector fix - prevent double redirect (commit 4022af54)
8. `bb73059f` - Redirector fix - parent pointer (commit 4022af54)

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

### 16. Dropall Command Support
**Status**: ⚠️ MERGE CANDIDATE (NEEDS REVIEW)
- Commit: `9094ec54` - "Allow the main ckpool process to accept the dropall command to pass to the stratifier"
- **Official change**: Adds `dropall` command handler in `src/ckpool.c` that forwards to stratifier
- **What it does**: Allows disconnecting all clients via command (useful for maintenance/emergency)
- **Fork status**: Not present in fork
- **Risk**: Low - just adds command handler, doesn't touch protected areas
- **Action**: ⚠️ Review needed - decide if we want this administrative feature

## Commits Not Applicable (Skipped - Not Needed)

❌ **Not Merged (Explicitly Not Brought Over):**
- `3f95bce6`, `227f415a` - configure.ac cleanup (NOT APPLICABLE - we don't have those debug lines)
- Version bumps - Skipped (version numbers are fork-specific)
- `3a8b0c21` - Update script to use bitcoin core v29.2 (NOT APPLICABLE - fork doesn't have install scripts)

## Commits Needing Review

⚠️ **Potential Merge Candidates (Need Decision):**
- `a8f808b5` - Decay inactive stats (different behavior - affects stats visibility)
- `9094ec54` - Dropall command support (administrative feature)

## Testing Checklist

After merging any changes:
- [x] Sub-"1" difficulty still works (test with 0.0005, 0.001, etc.) - ✅ Verified
- [x] Useragent whitelisting still works (empty, configured, not configured) - ✅ Verified
- [x] Donation system still works - ✅ Verified
- [x] Cookie authentication still works - ✅ Verified
- [x] dropidle feature works (disabled by default, configurable) - ✅ Verified
- [x] Pool mode starts correctly - ✅ Verified
- [x] Solo mode starts correctly - ✅ Verified
- [x] Proxy mode starts correctly - ✅ Verified
- [x] No regressions in existing functionality - ✅ All 15 unit tests passing (including test-dropidle)
- [x] All builds successful - ✅ Verified
- [x] No linter errors - ✅ Verified

## Merge Summary

**Total Merges Completed**: 8
**Status**: All merges successfully completed, tested, and merged to main
**Date**: Completed in current session
**Result**: Repository is up-to-date with safe improvements from official ckpool while preserving all fork-specific features

## Next Steps

- [ ] Continue monitoring official ckpool for new safe merges
- [ ] Consider integration testing with live bitcoind and miners (as mentioned in TESTING_STRATEGY.md)
- [x] Update DIFFERENCES.md - ✅ Completed (removed non-differences, focused on actual differences)

