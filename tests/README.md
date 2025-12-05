# CKPOOL-LHR Test Suite

This directory contains unit tests for CKPOOL-LHR.

## Test Files

1. **test-difficulty.c** - Sub-"1" difficulty calculations (CRITICAL fork feature)
2. **test-sha256.c** - SHA-256 hashing (CRITICAL)
3. **test-encoding.c** - Encoding/decoding functions (HIGH)
4. **test-donation.c** - Donation system (CRITICAL fork feature)
5. **test-useragent.c** - Useragent whitelisting (CRITICAL fork feature)
6. **test-address.c** - Address encoding (HIGH)
7. **test-string-utils.c** - String utilities (MEDIUM)
8. **test-time-utils.c** - Time utilities (MEDIUM)
9. **test-fulltest.c** - Hash vs target validation (CRITICAL)
10. **test-serialization.c** - Number serialization for transactions (MEDIUM)
11. **test-endian.c** - Endian conversion functions (MEDIUM)
12. **test-time-conversion.c** - Time conversion functions (LOW)
13. **test-base58.c** - Base58 decoding (LOW)
14. **test-base64.c** - Base64 encoding (LOW)
15. **test-dropidle.c** - dropidle feature (idle client management) (MEDIUM)

## Structure

- `unit/` - Unit tests for pure functions
- `integration/` - Integration/regression tests for live ckpool instance
- `fixtures/` - Test data and fixtures (future)

## Building and Running Tests

### Prerequisites

Tests use a simple built-in test framework (no external dependencies required).

### Build and Run

```bash
# Regenerate autotools files (if needed)
./autogen.sh

# Configure build system
./configure

# Build and run all tests
make check
```

### Run Individual Unit Tests

```bash
./tests/unit/test-difficulty
./tests/unit/test-sha256
./tests/unit/test-encoding
./tests/unit/test-donation
./tests/unit/test-useragent
./tests/unit/test-address
./tests/unit/test-string-utils
./tests/unit/test-time-utils
./tests/unit/test-fulltest
./tests/unit/test-serialization
./tests/unit/test-endian
./tests/unit/test-time-conversion
./tests/unit/test-base58
./tests/unit/test-base64
./tests/unit/test-dropidle
```

### Local Regression Testing

**Note**: Regression tests are environment-specific and should be kept local (not committed to repo).

For local validation of a running ckpool instance, you can create environment-specific regression tests in `tests/integration/` (this directory is gitignored).

Example test areas:
- Network communication (Stratum protocol)
- Bitcoind connectivity
- File I/O (logs, user data)
- Multi-process coordination
- Service management
- Configuration validation
- Share submission flow (requires full Stratum client)

## Test Framework

Tests use a simple custom test framework defined in `test_common.h`. The framework provides:
- `assert_true()`, `assert_false()`
- `assert_int_equal()`, `assert_double_equal()`
- `assert_string_equal()`, `assert_ptr_equal()`
- `assert_null()`, `assert_non_null()`
- `assert_memory_equal()`

## Adding New Tests

1. Create test file in `tests/unit/` directory (e.g., `test-newfeature.c`)
2. Add test file to `tests/Makefile.am`:
   ```makefile
   unit_test_newfeature_SOURCES = unit/test-newfeature.c
   ```
   Add `unit/test-newfeature` to `check_PROGRAMS` and `TESTS`
3. Write test cases following existing patterns
4. Run `make check` to verify

## Test Coverage

The test suite covers:
- All critical fork-specific features (sub-"1" difficulty, donation system, useragent whitelisting, dropidle)
- Core cryptographic functions (SHA-256, hash validation)
- Encoding/decoding functions (hex, Base58, Base64, address encoding)
- Utility functions (string operations, time conversions, number serialization)
- Edge cases and error handling

## Troubleshooting

### Tests don't compile
- Ensure `libckpool.a` is built first: `cd src && make`
- Check that all required headers are accessible
- Verify autotools files are up to date: `./autogen.sh && ./configure`

### Link errors
- Ensure jansson library is built: `cd src/jansson-2.14 && make`
- Check that `libckpool.a` includes all required objects

### Test failures
- Check `tests/test-suite.log` for detailed error messages
- Run individual test binaries for more verbose output
