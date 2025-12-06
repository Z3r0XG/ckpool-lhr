# Integration/Regression Tests

This directory contains integration and regression tests that require a live ckpool instance.

## Prerequisites

- Running ckpool instance (via systemd service or manually)
- Python 3
- Access to ckpool logs and configuration

## Running Tests

```bash
# Test against default pool (127.0.0.1:3333) and config
./test-regression.py

# Test against specific pool and config
./test-regression.py 192.168.1.100:3333 /path/to/ckpool.conf

# Set log directory
LOG_DIR=/var/log/ckpool ./test-regression.py
```

## Test Coverage

The regression test suite validates:

1. **Pool Connection** - Basic network connectivity
2. **Basic Subscription** - Stratum protocol subscription
3. **Empty User Agent** - Empty UA handling (should work when no whitelist)
4. **User Agent String** - Normal UA handling
5. **Sub-1 Difficulty Support** - Critical fork feature validation
6. **Donation Configuration** - Donation system setup
7. **Log File Writing** - Log file creation and content
8. **User Data Directory** - User data persistence structure
9. **Bitcoind Connectivity** - Remote bitcoind connection
10. **Pool Mode Detection** - Solo vs pool mode
11. **Miner Activity** - Recent activity detection
12. **Service Status** - Systemd service status
13. **Configuration File Validity** - Config syntax and variable names
14. **User Agent Whitelist Logic** - Whitelist configuration validation
15. **Share Submission** - Subscription and authorization flow
16. **Difficulty Configuration Validation** - Difficulty value validation
17. **User Data Persistence** - User data file structure and validity

## What These Tests Cover

These tests validate functionality that **cannot** be tested by unit tests:

- **Network Communication**: Real Stratum protocol interactions
- **Multi-Process Communication**: Generator, stratifier, connector coordination
- **File I/O**: Log file writing, user data persistence
- **Bitcoind Integration**: Real RPC calls and connectivity
- **Service Management**: Systemd integration
- **Real-Time Behavior**: Live pool operation
- **Configuration Parsing**: Real config file handling

## Unit Tests vs Integration Tests

- **Unit Tests** (`../unit/`): Test individual functions in isolation
- **Integration Tests** (`integration/`): Test full system with real components

Both are important for comprehensive test coverage.

