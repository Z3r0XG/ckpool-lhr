# Testing Status and Recommendations

## Current Test Coverage

### Unit Tests (18 tests)
✅ **Core Functions Tested:**
- Sub-"1" difficulty calculations (CRITICAL fork feature)
- SHA-256 hashing (CRITICAL)
- Encoding/decoding (hex, Base58, Base64)
- Donation system (CRITICAL fork feature)
- Useragent whitelisting (CRITICAL fork feature)
- Address encoding (P2PKH, P2SH, Bech32)
- String utilities
- Time utilities
- Hash validation (fulltest)
- Number serialization
- Endian conversion
- Time conversion
- dropidle feature

### Integration Tests (Local-only)
✅ **Live System Tests:**
- Pool connection
- Stratum subscription
- User agent handling
- Configuration validation
- Bitcoind connectivity
- Log file writing
- User data persistence

### Tested Modes
✅ **Solo Mode (BTCSOLO)** - Extensively tested in production

### Untested Modes
❌ **Proxy Mode** - Not tested
❌ **Passthrough Mode** - Not tested
❌ **Node Mode** - Not tested
❌ **Redirector Mode** - Not tested
❌ **Userproxy Mode** - Not tested
❌ **Pool Mode** - Not tested (only solo)

## Missing Unit Tests

### High Priority
1. ✅ **Vardiff (Variable Difficulty) Logic** - COMPLETED
   - Optimal difficulty calculation
   - Difficulty adjustment based on share rate
   - Mindiff/maxdiff clamping
   - File: `test-vardiff.c`

2. ✅ **Share Parameter Validation** - COMPLETED
   - Nonce validation
   - ntime validation
   - job_id validation
   - File: `test-share-params.c`

3. **Share Hash Validation**
   - Share hash vs target comparison
   - Extranonce handling
   - File: `test-share-validation.c` (future)

4. **Workbase/Block Template Functions**
   - Coinbase generation
   - Merkle root calculation
   - Transaction handling
   - File: `test-workbase.c` (future)

### Medium Priority
4. ✅ **Configuration Validation** - COMPLETED
   - JSON type validation
   - Array parsing
   - Default value handling
   - File: `test-config.c`

5. **User/Worker Stats**
   - Stats decay calculation
   - Hashrate averaging
   - Best share tracking
   - File: `test-stats.c` (future)

6. **Proxy Mode Functions** (requires multi-instance setup)
   - Upstream connection handling
   - Share forwarding logic
   - Failover behavior
   - File: `test-proxy.c` (future - low priority)

## Testing Other Modes

### Proxy Mode Testing

**Setup:**
1. Run main pool (upstream) on one machine/port
2. Run proxy on another machine/port pointing to upstream
3. Connect miner to proxy
4. Verify shares flow through proxy to upstream

**Test Script:**
```bash
# Terminal 1: Start upstream pool
ckpool -c ckpool.conf

# Terminal 2: Start proxy
ckpool -p -c ckproxy.conf

# Terminal 3: Connect miner to proxy
# Verify in upstream pool logs that shares arrive
```

**What to Test:**
- Proxy connects to upstream
- Shares forwarded correctly
- Upstream pool receives shares
- Failover if upstream disconnects
- Multiple miners aggregated correctly

### Passthrough Mode Testing

**Setup:**
1. Run main pool (upstream)
2. Run passthrough node pointing to upstream
3. Connect multiple miners to passthrough
4. Verify each miner maintains identity on upstream

**Test Script:**
```bash
# Terminal 1: Start upstream pool
ckpool -c ckpool.conf

# Terminal 2: Start passthrough
ckpool -P -c ckpassthrough.conf

# Terminal 3: Connect multiple miners to passthrough
# Verify in upstream pool that each miner appears separately
```

**What to Test:**
- Passthrough connects to upstream
- Individual miner identities preserved
- Shares forwarded with correct credentials
- Scaling with many miners

### Node Mode Testing

**Setup:**
1. Run main pool (upstream)
2. Run node with local bitcoind
3. Connect miners to node
4. Verify node can submit blocks

**Test Script:**
```bash
# Terminal 1: Start upstream pool
ckpool -c ckpool.conf

# Terminal 2: Start node (requires local bitcoind)
ckpool -N -c cknode.conf

# Terminal 3: Connect miners
# Verify node can submit blocks to bitcoind
```

**What to Test:**
- Node connects to upstream and bitcoind
- Block submission capability
- Hashrate monitoring

### Redirector Mode Testing

**Setup:**
1. Run multiple target pools
2. Run redirector pointing to target pools
3. Connect miners to redirector
4. Verify miners get redirected after first share

**Test Script:**
```bash
# Terminal 1: Start target pool 1
ckpool -c ckpool1.conf

# Terminal 2: Start target pool 2
ckpool -c ckpool2.conf

# Terminal 3: Start redirector
ckpool -R -c ckredirector.conf

# Terminal 4: Connect miner
# Verify miner gets redirected after first accepted share
```

**What to Test:**
- Redirector filters inactive miners
- Redirect happens after first accepted share
- Redirect cycles through multiple URLs
- IP-based redirect consistency

### Userproxy Mode Testing

**Setup:**
1. Run upstream pool
2. Run userproxy pointing to upstream
3. Connect miners with different credentials
4. Verify each miner uses their own credentials upstream

**Test Script:**
```bash
# Terminal 1: Start upstream pool
ckpool -c ckpool.conf

# Terminal 2: Start userproxy
ckpool -u -c ckproxy.conf

# Terminal 3: Connect miners with different usernames
# Verify upstream pool sees each miner separately
```

**What to Test:**
- Individual credentials preserved
- Each miner gets separate upstream connection
- Credential forwarding correct

## Recommended Test Implementation Plan

### Phase 1: Additional Unit Tests (Low effort, high value) ✅ COMPLETED
1. ✅ Created `test-vardiff.c` - Test difficulty adjustment logic
2. ✅ Created `test-share-params.c` - Test share parameter validation
3. ✅ Created `test-config.c` - Test configuration validation

### Phase 2: Mode-Specific Integration Tests (Medium effort)
1. Create simple test scripts for each mode
2. Document setup requirements
3. Add to `tests/integration/` (local-only)

### Phase 3: Comprehensive Mode Testing (High effort)
1. Automated mode testing with mock upstream pools
2. Multi-mode integration tests
3. Performance/load testing

## Current Gaps

### Critical Gaps
- ✅ Vardiff logic tested (core functionality)
- ✅ Share parameter validation tested
- ❌ Share hash validation edge cases (future)
- ❌ Other operation modes completely untested (requires multi-instance setup)

### Medium Gaps
- ✅ Configuration validation tested
- ⚠️ Stats calculation accuracy (future)
- ⚠️ Multi-process coordination (requires integration tests)

### Low Priority Gaps
- ⚠️ Error handling in edge cases
- ⚠️ Memory leak detection
- ⚠️ Performance benchmarks

## Next Steps

1. ✅ **Completed**: Added unit tests for vardiff, share params, and config validation
2. **Current Status**: 18 unit tests covering core functionality
3. **Future**: Additional edge case tests as needed (share hash validation, stats decay)
4. **Note**: Mode-specific testing (proxy, passthrough, etc.) requires multi-instance setup and is lower priority for single-server deployment

