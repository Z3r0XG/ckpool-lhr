# ⚠️ TEMPORARY - DELETE AFTER TASKS COMPLETE ⚠️
# This is a local planning document and should NOT be committed to the repository.
# Add to .gitignore and delete once testing infrastructure is implemented.

# Testing Strategy for CKPOOL-LHR

This document outlines a comprehensive testing strategy for CKPOOL-LHR, identifying what can be tested with mocks and what requires integration testing.

## Current State

- **Unit test infrastructure**: ✅ Complete (14 test suites)
- **Unit tests**: ✅ 14 test files covering all critical functions
- **Integration tests**: ❌ Not yet implemented (local testing only)
- **Test framework**: Simple custom framework (no external dependencies)

### Completed Unit Tests (14/14)

1. ✅ test-difficulty.c - Sub-"1" difficulty calculations
2. ✅ test-sha256.c - SHA-256 hashing
3. ✅ test-encoding.c - Encoding/decoding functions
4. ✅ test-donation.c - Donation system
5. ✅ test-useragent.c - Useragent whitelisting
6. ✅ test-address.c - Address encoding
7. ✅ test-string-utils.c - String utilities
8. ✅ test-time-utils.c - Time utilities
9. ✅ test-fulltest.c - Hash vs target validation
10. ✅ test-serialization.c - Number serialization
11. ✅ test-endian.c - Endian conversion
12. ✅ test-time-conversion.c - Time conversion functions
13. ✅ test-base58.c - Base58 decoding
14. ✅ test-base64.c - Base64 encoding

## Testing Approach

### 1. Unit Tests (Pure Functions) - **HIGH PRIORITY**

These functions have no external dependencies and can be tested directly without mocks.

#### A. Difficulty Calculations (`src/libckpool.c`)
**Testability**: ✅ **100%** - Pure mathematical functions

- `le256todouble()` - Convert little-endian 256-bit to double
- `be256todouble()` - Convert big-endian 256-bit to double
- `diff_from_target()` - Calculate difficulty from binary target
- `diff_from_betarget()` - Calculate difficulty from big-endian target
- `diff_from_nbits()` - Calculate difficulty from nbits (block header)
- `target_from_diff()` - Convert difficulty to target (CRITICAL for sub-"1" support)
- `fulltest()` - Validate hash against target

**Test Cases Needed**:
- Sub-"1" difficulty values (0.0005, 0.001, 0.5, etc.)
- Standard difficulty values (1.0, 42.0, 100.0, etc.)
- Edge cases (0.0, very large values, precision limits)
- Round-trip: `diff → target → diff` should match

**Priority**: **CRITICAL** - Core to fork's functionality

#### B. Encoding/Decoding (`src/libckpool.c`)
**Testability**: ✅ **100%** - Pure transformation functions

- `bin2hex()` / `__bin2hex()` - Binary to hex string
- `hex2bin()` / `_hex2bin()` - Hex string to binary
- `validhex()` - Validate hex string format
- `b58tobin()` - Base58 decode (Bitcoin address)
- `bech32_decode()` - Bech32 decode (Segwit address)
- `address_to_txn()` - Convert address to transaction script
- `ser_number()` / `get_sernumber()` - Serialize/deserialize numbers

**Test Cases Needed**:
- Valid/invalid hex strings
- Valid/invalid Bitcoin addresses (legacy, segwit, bech32)
- Round-trip encoding/decoding
- Edge cases (empty strings, maximum lengths)

**Priority**: **HIGH** - Used throughout codebase

#### C. SHA-256 Hashing (`src/sha2.c`)
**Testability**: ✅ **100%** - Pure cryptographic function

- `sha256()` - Single-shot SHA-256
- `sha256_init()` / `sha256_final()` - Streaming SHA-256
- `gen_hash()` - Double SHA-256 (Bitcoin standard)

**Test Cases Needed**:
- Known test vectors (NIST, Bitcoin test vectors)
- Empty input
- Various input sizes
- Double SHA-256 correctness

**Priority**: **CRITICAL** - Core to share validation

#### D. String Utilities (`src/libckpool.c`)
**Testability**: ✅ **100%** - Pure string functions

- `safecmp()` - Safe string comparison
- `safencmp()` - Safe string comparison with length
- `cmdmatch()` - Command matching
- `suffix_string()` - Format numbers with suffixes (K, M, G, etc.)

**Test Cases Needed**:
- NULL handling
- Empty strings
- Various input combinations
- Edge cases

**Priority**: **MEDIUM** - Utility functions

#### E. Time Utilities (`src/libckpool.c`)
**Testability**: ⚠️ **PARTIAL** - Some functions need time mocking

- `us_tvdiff()` / `ms_tvdiff()` / `tvdiff()` - Time difference calculations
- `decay_time()` - Exponential decay calculation
- `sane_tdiff()` - Sanitized time difference

**Test Cases Needed**:
- Time difference calculations
- Edge cases (negative, zero, very large)
- Decay calculations with various parameters

**Priority**: **MEDIUM** - Used in statistics

### 2. Functions Requiring Mocks - **MEDIUM PRIORITY**

These functions interact with external systems and need mocking.

#### A. Bitcoind RPC (`src/bitcoin.c`)
**Testability**: ⚠️ **80%** - Mock JSON-RPC responses

Functions to test:
- `getblocktemplate()` - Fetch block template
- `submitblock()` - Submit solved block
- `validateaddress()` - Validate Bitcoin address
- `validate_txn()` - Validate transaction
- `getblockhash()` / `getblockcount()` - Block chain queries

**Mock Strategy**:
- Mock HTTP/JSON-RPC responses
- Use test fixtures for block templates
- Simulate network errors, timeouts

**Test Cases Needed**:
- Valid block templates
- Invalid responses
- Network failures
- Address validation (legacy, segwit, bech32)
- Transaction validation

**Priority**: **HIGH** - Critical for pool operation

#### B. Network I/O (`src/libckpool.c`, `src/connector.c`)
**Testability**: ⚠️ **70%** - Mock sockets and network calls

Functions to test:
- `bind_socket()` - Bind to network port
- `connect_socket()` - Connect to remote host
- `write_socket()` / `read_socket()` - Socket I/O
- `extract_sockaddr()` - Parse URL/port
- Unix socket functions

**Mock Strategy**:
- Mock `socket()`, `bind()`, `connect()`, `send()`, `recv()`
- Use test sockets or mock file descriptors
- Simulate network conditions (timeouts, errors)

**Test Cases Needed**:
- Valid/invalid URLs and ports
- Connection failures
- Timeout handling
- Socket buffer management

**Priority**: **MEDIUM** - Infrastructure

#### C. File I/O (`src/libckpool.c`, `src/ckpool.c`)
**Testability**: ⚠️ **80%** - Mock file operations

Functions to test:
- Configuration file parsing (`parse_config()`)
- JSON file loading
- Log file writing

**Mock Strategy**:
- Use in-memory file buffers
- Mock `fopen()`, `fread()`, `fwrite()`
- Test with various JSON configurations

**Test Cases Needed**:
- Valid/invalid configuration files
- Missing files
- Malformed JSON
- All configuration options

**Priority**: **HIGH** - Configuration is critical

### 3. Integration Points - **LOWER PRIORITY**

These require full system setup or are harder to test in isolation.

#### A. Stratum Protocol (`src/stratifier.c`)
**Testability**: ⚠️ **60%** - Complex state machine

Functions to test:
- `parse_subscribe()` - Subscription handling (CRITICAL for useragent whitelist)
- `parse_authorise()` - Authorization
- `parse_submit()` - Share submission
- `vardiff()` - Difficulty adjustment (CRITICAL for sub-"1" support)

**Mock Strategy**:
- Mock client connections
- Mock workbase data
- Test protocol message parsing
- Test state transitions

**Test Cases Needed**:
- **Useragent whitelisting**: Empty, configured, not configured
- **Sub-"1" difficulty**: mindiff/startdiff below 1.0
- Valid/invalid shares
- Difficulty adjustments
- Protocol edge cases

**Priority**: **CRITICAL** - Core fork features

#### B. Process Communication (`src/libckpool.c`)
**Testability**: ⚠️ **50%** - Unix sockets, multi-process

Functions to test:
- Unix socket communication
- Message queue operations
- Inter-process synchronization

**Mock Strategy**:
- Use test Unix sockets
- Mock process creation
- Test message passing

**Priority**: **LOW** - Infrastructure, less likely to break

#### C. Generator Process (`src/generator.c`)
**Testability**: ⚠️ **40%** - Complex bitcoind interaction

Functions to test:
- Block template management
- Workbase creation
- Block submission

**Mock Strategy**:
- Mock bitcoind RPC calls
- Test workbase generation
- Test block validation

**Priority**: **MEDIUM** - Important but complex

## Recommended Testing Framework

### Option 1: CMocka (Recommended)
- **Language**: C
- **Features**: Unit testing, mocking, assertions
- **Pros**: Lightweight, designed for C, good mocking support
- **Cons**: Requires setup

### Option 2: Unity + CMock
- **Language**: C
- **Features**: Unit testing framework + mocking
- **Pros**: Good integration, widely used
- **Cons**: More complex setup

### Option 3: Check
- **Language**: C
- **Features**: Unit testing framework
- **Pros**: Mature, good documentation
- **Cons**: Less mocking support

## Implementation Plan

### Phase 1: Foundation (Week 1-2)
1. Set up CMocka testing framework
2. Create test directory structure (`tests/`)
3. Add test build targets to Makefile
4. Write tests for **critical pure functions**:
   - `target_from_diff()` / `diff_from_target()` (sub-"1" support)
   - `sha256()` / `gen_hash()` (share validation)
   - `address_to_txn()` (address handling)

### Phase 2: Core Utilities (Week 3-4)
1. Test encoding/decoding functions
2. Test string utilities
3. Test time utilities
4. Achieve **80%+ coverage** of `libckpool.c` pure functions

### Phase 3: Mocked Functions (Week 5-6)
1. Set up mocking framework
2. Test bitcoind RPC functions (with mocks)
3. Test network I/O functions (with mocks)
4. Test configuration parsing

### Phase 4: Integration Tests (Week 7-8)
1. Test Stratum protocol handling
   - **Focus**: Useragent whitelisting
   - **Focus**: Sub-"1" difficulty handling
2. Test vardiff logic
3. Test donation calculations

### Phase 5: Continuous Integration
1. Add automated test runs
2. Add coverage reporting
3. Add regression test suite

## Priority Test Cases (Fork-Specific)

### 1. Sub-"1" Difficulty Support
**Critical Test Cases**:
```c
// Test target_from_diff with sub-1 values
target_from_diff(target, 0.0005);  // Should work
target_from_diff(target, 0.001);  // Should work
target_from_diff(target, 0.5);    // Should work
target_from_diff(target, 1.0);    // Standard case
target_from_diff(target, 42.0);   // Standard case

// Test round-trip
double diff = 0.0005;
uchar target[32];
target_from_diff(target, diff);
double result = diff_from_target(target);
assert(fabs(result - diff) < 0.0001);  // Allow small precision error
```

### 2. Useragent Whitelisting
**Critical Test Cases**:
```c
// Test empty useragent (should be allowed if whitelist not configured)
// Test empty useragent (should be rejected if whitelist configured)
// Test matching useragent (should be allowed)
// Test non-matching useragent (should be rejected)
// Test prefix matching
// Test empty whitelist array (should allow all)
```

### 3. Donation System
**Critical Test Cases**:
```c
// Test donation calculation with double precision
// Test 0.5% donation
// Test 1.0% donation
// Test edge cases (0.0, 99.9)
```

## Test Coverage Goals

- **Phase 1**: 30% coverage (critical pure functions)
- **Phase 2**: 60% coverage (all pure functions)
- **Phase 3**: 75% coverage (with mocked functions)
- **Phase 4**: 85% coverage (with integration tests)

## Testing Best Practices

1. **Test-Driven Development**: Write tests before fixing bugs
2. **Regression Tests**: Add tests for every bug fix
3. **Edge Cases**: Test boundary conditions, NULL inputs, empty strings
4. **Fork-Specific**: Prioritize tests for fork modifications
5. **Continuous Testing**: Run tests on every commit
6. **Documentation**: Document test cases and expected behavior

## Estimated Effort

- **Pure Functions**: 2-3 weeks (high value, easy to test)
- **Mocked Functions**: 2-3 weeks (medium value, moderate effort)
- **Integration Tests**: 3-4 weeks (high value, complex setup)
- **Total**: 7-10 weeks for comprehensive test suite

## Quick Wins (Can Start Immediately)

1. **Difficulty calculations** - 1-2 days, high value
2. **SHA-256 functions** - 1 day, critical
3. **Address encoding** - 1-2 days, high value
4. **Useragent whitelist logic** - 2-3 days, fork-specific

These four areas alone would provide significant confidence in the fork's core functionality.

---

## Local Integration Testing (Not Committed to Repo)

While unit tests with mocks provide excellent coverage of isolated functions, **local integration testing** with real bitcoind and miners fills critical gaps in testing. This approach tests the full system in a realistic environment without requiring test infrastructure in the repository.

### Test Environment Setup

**Requirements**:
- Local bitcoind instance (regtest or testnet)
- One or more mining clients (CPU miner, ESP32, or low-hash-rate device)
- Isolated network environment
- Test Bitcoin addresses

**Recommended Setup**:
```
┌─────────────┐
│  bitcoind   │ (regtest mode)
│  (regtest)  │
└──────┬──────┘
       │ JSON-RPC
       │
┌──────▼──────┐
│  ckpool-lhr │ (pool mode)
│  (test)     │
└──────┬──────┘
       │ Stratum
       │
┌──────▼──────┐
│   Miner     │ (CPU/ESP32)
│  (test)     │
└─────────────┘
```

### What Can Be Tested Locally

#### 1. Sub-"1" Difficulty Support ✅ **CRITICAL**
**Test Scenarios**:
- Configure `mindiff: 0.0005`, `startdiff: 0.0005`
- Connect low-hash-rate miner (ESP32, CPU miner)
- Verify miner receives work with sub-"1" difficulty
- Verify shares are accepted at sub-"1" difficulty
- Verify vardiff doesn't go below configured `mindiff`
- Test various sub-"1" values (0.0001, 0.001, 0.5)

**Validation**:
- Check pool logs for difficulty assignments
- Verify share submissions are accepted
- Monitor miner's received difficulty in Stratum messages
- Test edge cases (very low values, precision limits)

**Priority**: **HIGHEST** - Core fork feature

#### 2. Useragent Whitelisting ✅ **CRITICAL**
**Test Scenarios**:
- **Whitelist not configured**: Connect with various useragents (including empty)
- **Whitelist configured**: 
  - Connect with matching useragent → should succeed
  - Connect with non-matching useragent → should be rejected
  - Connect with empty useragent → should be rejected
- Test prefix matching behavior
- Test multiple whitelist entries

**Validation**:
- Check connection acceptance/rejection
- Verify error messages
- Test with real mining software (different useragents)

**Priority**: **HIGHEST** - Fork-specific feature

#### 3. Donation System ✅
**Test Scenarios**:
- Configure `donation: 1.0`, `donaddress: <test-address>`
- Mine a block (or simulate block reward)
- Verify donation amount is correct (1% of reward)
- Verify donation goes to `donaddress`
- Test fractional donations (0.5%, 1.5%)
- Test edge cases (0.0, 99.9)

**Validation**:
- Check block reward distribution
- Verify transaction outputs
- Test with various donation percentages

**Priority**: **HIGH** - Fork modification

#### 4. Stratum Protocol Integration ✅
**Test Scenarios**:
- Full subscription flow
- Authorization with username/password
- Work distribution
- Share submission and validation
- Difficulty adjustment (vardiff)
- Reconnection handling

**Validation**:
- Monitor Stratum messages
- Verify protocol compliance
- Test error handling

**Priority**: **HIGH** - Core functionality

#### 5. Bitcoind Integration ✅
**Test Scenarios**:
- Block template fetching
- Address validation (legacy, segwit, bech32)
- Block submission
- Network difficulty retrieval
- ZMQ block notifications (if available)

**Validation**:
- Verify bitcoind RPC calls
- Check block template format
- Test with regtest/testnet

**Priority**: **HIGH** - Critical dependency

#### 6. Cookie Authentication ✅
**Test Scenarios**:
- Configure bitcoind with cookie auth
- Verify connection succeeds
- Test fallback to user/pass if cookie fails

**Validation**:
- Check authentication method used
- Verify connection stability

**Priority**: **MEDIUM** - Fork feature

#### 7. Pool Modes ✅
**Test Scenarios**:
- **Pool mode**: Multiple miners, share distribution
- **Solo mode**: Single miner, direct block rewards
- **Proxy mode**: Forward to upstream pool
- **Passthrough mode**: Direct bitcoind connection

**Validation**:
- Verify mode-specific behavior
- Test mode switching
- Check configuration requirements

**Priority**: **MEDIUM** - Core functionality

#### 8. Vardiff (Variable Difficulty) ✅
**Test Scenarios**:
- Start with `startdiff: 0.001`
- Monitor difficulty adjustments over time
- Verify difficulty increases with higher hash rate
- Verify difficulty decreases with lower hash rate
- Test `mindiff` and `maxdiff` limits
- Test with sub-"1" `mindiff` values

**Validation**:
- Check difficulty changes in logs
- Verify adjustments are reasonable
- Test boundary conditions

**Priority**: **HIGH** - Important for sub-"1" support

#### 9. Share Validation ✅
**Test Scenarios**:
- Submit valid shares → should be accepted
- Submit invalid shares → should be rejected
- Submit shares at various difficulties
- Test share validation with sub-"1" difficulty

**Validation**:
- Check share acceptance/rejection
- Verify share difficulty matches target
- Monitor share statistics

**Priority**: **HIGH** - Core functionality

#### 10. Block Solving ✅
**Test Scenarios**:
- Mine a block (regtest makes this easier)
- Verify block is submitted to bitcoind
- Verify block reward distribution
- Test with donation configured

**Validation**:
- Check block submission
- Verify block is accepted by bitcoind
- Check reward distribution

**Priority**: **MEDIUM** - Less frequent but critical

### Local Testing Workflow

#### Setup Script (local, not committed)
```bash
#!/bin/bash
# test/local-setup.sh (in .gitignore)

# Start bitcoind in regtest mode
bitcoind -regtest -daemon -server -rpcuser=test -rpcpassword=test

# Generate test blocks (regtest)
bitcoin-cli -regtest -rpcuser=test -rpcpassword=test generate 101

# Start ckpool-lhr with test config
./ckpool -c test/ckpool-test.conf

# Connect test miner
# (miner command depends on miner software)
```

#### Test Configuration (local, not committed)
```json
// test/ckpool-test.conf (in .gitignore)
{
  "btcd": [{
    "server": "127.0.0.1",
    "rpcport": 18443,
    "rpcuser": "test",
    "rpcpassword": "test"
  }],
  "btcaddress": "bcrt1qtest...",
  "btcsolo": false,
  "mindiff": 0.0005,
  "startdiff": 0.0005,
  "maxdiff": 1.0,
  "useragent": ["TestMiner"],
  "donation": 1.0,
  "donaddress": "bcrt1qdonation..."
}
```

#### Test Checklist
- [ ] Sub-"1" difficulty works end-to-end
- [ ] Useragent whitelisting works correctly
- [ ] Donation system calculates correctly
- [ ] Shares are validated properly
- [ ] Vardiff adjusts correctly
- [ ] Block solving works
- [ ] Reconnection handling works
- [ ] Error conditions are handled gracefully

### Advantages of Local Integration Testing

1. **Real-world scenarios**: Tests actual system behavior
2. **End-to-end validation**: Tests full stack integration
3. **Protocol compliance**: Verifies Stratum protocol correctness
4. **Performance testing**: Can test under load
5. **Edge case discovery**: Finds issues unit tests might miss
6. **No repository bloat**: Keeps test infrastructure local

### Limitations

1. **Requires setup**: Needs bitcoind and miner software
2. **Time-consuming**: Slower than unit tests
3. **Environment-dependent**: May behave differently on different systems
4. **Not automated**: Requires manual execution (though can be scripted)
5. **Resource-intensive**: Requires running multiple processes

### Recommended Testing Approach

**Combination Strategy**:
1. **Unit tests** (in repo): Fast, automated, comprehensive coverage of pure functions
2. **Local integration tests** (not in repo): Real-world validation, protocol testing, end-to-end verification

**Workflow**:
1. Write unit tests for pure functions (committed to repo)
2. Run local integration tests before releases
3. Use local tests to validate fixes and new features
4. Document test procedures (but not test infrastructure)

### Test Documentation (Can Be Committed)

While test infrastructure stays local, **test procedures and checklists** can be documented:

- Test environment setup instructions
- Test scenarios and expected results
- Troubleshooting guide
- Known issues and workarounds

This provides value without committing test-specific code or configurations.

---

## Combined Testing Strategy

### Unit Tests (Repo) + Local Integration Tests (Local)

| Component | Unit Tests | Local Integration | Coverage |
|-----------|------------|-------------------|----------|
| Difficulty calculations | ✅ 100% | ✅ Validation | ~100% |
| SHA-256 hashing | ✅ 100% | ✅ Validation | ~100% |
| Address encoding | ✅ 100% | ✅ Validation | ~100% |
| Useragent whitelist | ⚠️ Logic only | ✅ Full protocol | ~90% |
| Sub-"1" difficulty | ✅ Calculations | ✅ End-to-end | ~95% |
| Donation system | ⚠️ Calculations | ✅ Full flow | ~85% |
| Stratum protocol | ⚠️ Parsing | ✅ Full protocol | ~80% |
| Bitcoind RPC | ⚠️ Mocked | ✅ Real RPC | ~75% |
| Network I/O | ⚠️ Mocked | ✅ Real sockets | ~70% |
| Process coordination | ❌ Limited | ✅ Real processes | ~60% |

**Overall Coverage**: ~85-90% with combined approach

This combination provides:
- **Fast feedback** from unit tests (CI/CD)
- **Real-world validation** from local integration tests
- **Comprehensive coverage** of both isolated and integrated components
- **Confidence** in fork-specific features (sub-"1" difficulty, useragent whitelisting)

