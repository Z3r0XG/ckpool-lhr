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
16. **test-vardiff.c** - Variable difficulty adjustment logic (HIGH)
17. **test-share-params.c** - Share submission parameter validation (HIGH)
18. **test-config.c** - Configuration parsing and validation (HIGH)
19. **test-adjustment-hysteresis.c** - Difficulty adjustment hysteresis (MEDIUM)
20. **test-backwards-compatibility.c** - Backwards compatibility checks (MEDIUM)
21. **test-fractional-config.c** - Fractional difficulty configuration (HIGH fork feature)
22. **test-fractional-stats.c** - Fractional difficulty statistics (MEDIUM fork feature)
23. **test-fractional-vardiff.c** - Fractional difficulty vardiff behavior (HIGH fork feature)
24. **test-low-diff.c** - Low difficulty share handling (HIGH fork feature)
25. **test-network-diff-interactions.c** - Network difficulty interactions (MEDIUM)
26. **test-share-orphan-prevention.c** - Share orphan prevention (HIGH)
27. **test-ua-aggregation.c** - Useragent aggregation (MEDIUM fork feature)
28. **test-vardiff-comprehensive.c** - Comprehensive vardiff testing (HIGH)

## Building and Running Tests

### Prerequisites

> [!NOTE]
> Tests use a simple built-in test framework (no external dependencies required).

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
./tests/unit/test-vardiff
./tests/unit/test-share-params
./tests/unit/test-config
./tests/unit/test-adjustment-hysteresis
./tests/unit/test-backwards-compatibility
./tests/unit/test-fractional-config
./tests/unit/test-fractional-stats
./tests/unit/test-fractional-vardiff
./tests/unit/test-low-diff
./tests/unit/test-network-diff-interactions
./tests/unit/test-share-orphan-prevention
./tests/unit/test-ua-aggregation
./tests/unit/test-vardiff-comprehensive
```

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
- Variable difficulty adjustment logic (vardiff)
- Share submission parameter validation
- Configuration parsing and validation
- Encoding/decoding functions (hex, Base58, Base64, address encoding)
- Utility functions (string operations, time conversions, number serialization)
- Edge cases and error handling

## Troubleshooting

> [!TIP]
> **Tests don't compile?**
> - Ensure `libckpool.a` is built first: `cd src && make`
> - Check that all required headers are accessible
> - Verify autotools files are up to date: `./autogen.sh && ./configure`

> [!TIP]
> **Link errors?**
> - Ensure jansson library is built: `cd src/jansson-2.14 && make`
> - Check that `libckpool.a` includes all required objects

> [!TIP]
> **Test failures?**
> - Check `tests/test-suite.log` for detailed error messages
> - Run individual test binaries for more verbose output
