# CKPOOL-LHR Test Suite

This directory contains unit tests for CKPOOL-LHR.

## Test Files

1. **test-difficulty.c** - Sub-"1" difficulty calculations (fork feature)
2. **test-sha256.c** - SHA-256 hashing
3. **test-encoding.c** - Encoding/decoding functions
4. **test-donation.c** - Donation system (fork feature)
5. **test-useragent.c** - Useragent whitelisting (fork feature)
6. **test-address.c** - Address encoding
7. **test-string-utils.c** - String utilities
8. **test-time-utils.c** - Time utilities
9. **test-fulltest.c** - Hash vs target validation
10. **test-serialization.c** - Number serialization for transactions
11. **test-endian.c** - Endian conversion functions
12. **test-dropidle.c** - dropidle feature (idle client management)
13. **test-vardiff.c** - Variable difficulty adjustment logic
14. **test-share-params.c** - Share submission parameter validation
15. **test-config.c** - Configuration parsing and validation
16. **test-password-diff.c** - Password field difficulty suggestion parsing
17. **test-password-diff-job-id.c** - Password diff job ID assignment
18. **test-fractional-stats.c** - Fractional difficulty statistics (fork feature)
19. **test-fractional-config.c** - Fractional difficulty configuration (fork feature)
20. **test-share-orphan-prevention.c** - Share orphan prevention
21. **test-adjustment-hysteresis.c** - Difficulty adjustment hysteresis
22. **test-network-diff-interactions.c** - Network difficulty interactions
23. **test-backwards-compatibility.c** - Backwards compatibility checks
24. **test-ua-aggregation.c** - Useragent aggregation (fork feature)
25. **test-worker-ua-recalc.c** - Worker UA recalculation
26. **test-persistent-ua-tracking.c** - Persistent UA tracking
27. **test-zombie-cleanup.c** - Zombie/ghost cleanup and refcount invariants (fork feature)
28. **test-auth-rejection.c** - Share rejection during auth window

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

# Run all tests with opt-in performance sections enabled
make check-perf
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
./tests/unit/test-dropidle
./tests/unit/test-vardiff
./tests/unit/test-share-params
./tests/unit/test-config
./tests/unit/test-adjustment-hysteresis
./tests/unit/test-backwards-compatibility
./tests/unit/test-fractional-config
./tests/unit/test-fractional-stats
./tests/unit/test-network-diff-interactions
./tests/unit/test-share-orphan-prevention
./tests/unit/test-ua-aggregation
./tests/unit/test-worker-ua-recalc
./tests/unit/test-persistent-ua-tracking
./tests/unit/test-password-diff
./tests/unit/test-password-diff-job-id
./tests/unit/test-auth-rejection
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
- Useragent aggregation/reporting (pool.status, user pages exposure)
- Zombie/ghost cleanup behavior and refcount safety
- Core cryptographic functions (SHA-256, hash validation)
- Variable difficulty adjustment logic (vardiff)
- Share submission parameter validation
- Configuration parsing and validation
- Encoding/decoding functions (hex, Base58, Base64, address encoding)
- Utility functions (string operations, time conversions, number serialization)
- UA normalization, aggregation, and persistence
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
> - Each unit test writes its own `tests/unit/*.log` and `tests/unit/*.trs`
