# CKPOOL-LHR Frequently Asked Questions

## Overview

CKPOOL-LHR is a fork of CKPOOL by Con Kolivas, featuring sub-"1" difficulty support for low hash rate miners (ESP32 devices and others). It is an ultra low overhead, massively scalable, multi-process, multi-threaded, modular Bitcoin mining pool, proxy, passthrough, and library written in C for Linux.

The software can operate in multiple deployment modes: as a standalone pool, a proxy to upstream pools, a passthrough node, a redirector, or as a library for integration into other software.

---

## File Structure and Purpose

### Core Source Files

#### `src/ckpool.c` and `src/ckpool.h`
Main entry point and core pool management. Handles command-line argument parsing, configuration loading, process initialization, and coordination between the three main processes (generator, stratifier, connector). Manages logging, signal handling, and process lifecycle. The header file defines the main `ckpool_instance` structure that holds all pool state and configuration.

#### `src/generator.c` and `src/generator.h`
Generator process responsible for communication with bitcoind. Fetches block templates using `getblocktemplate` RPC, manages bitcoind connections with failover support, validates addresses and transactions, submits solved blocks, and handles ZMQ block notifications. In proxy/passthrough modes, manages connections to upstream pools and forwards shares.

#### `src/stratifier.c` and `src/stratifier.h`
Stratifier process handles all Stratum protocol communication with miners. Manages client connections, subscription, authorization, work distribution, share validation, difficulty adjustment (vardiff), and block solving detection. Maintains workbases (block templates) and distributes work to connected miners. Handles both direct miner connections and remote node connections.

#### `src/connector.c` and `src/connector.h`
Connector process manages network I/O for all client connections. Handles TCP socket listening, accepting new connections, reading/writing data, and event-driven communication using epoll. Routes messages between network clients and the stratifier process via Unix domain sockets. Manages connection lifecycle and socket buffer optimization.

#### `src/bitcoin.c` and `src/bitcoin.h`
Bitcoin protocol implementation. Provides functions for JSON-RPC communication with bitcoind: `getblocktemplate`, `submitblock`, `getblockhash`, `getblockcount`, `validateaddress`, `validate_txn`, and related operations. Handles connection management, request/response parsing, and error handling for bitcoind communication.

#### `src/libckpool.c` and `src/libckpool.h`
Core library providing shared utilities used across all components. Includes memory management, threading primitives (mutexes, rwlocks, condition variables), Unix socket communication, JSON parsing helpers, time utilities, difficulty calculations, SHA-256 hashing, address encoding/decoding, and network socket operations. Can be used independently as a library.

#### `src/sha2.c` and `src/sha2.h`
SHA-256 hashing implementation (FIPS 180-2 compliant). Used for share validation and block header hashing. Provides optimized hash functions required for Bitcoin mining operations.

#### `src/notifier.c`
Standalone utility that can be run with bitcoind's `-blocknotify` option to notify ckpool of block changes when ZMQ is not available. Connects to the stratifier process via Unix socket and sends update messages.

#### `src/ckpmsg.c`
Command-line utility for sending messages to running ckpool instances via Unix sockets. Allows querying statistics, sending commands, and interacting with pool processes for debugging and monitoring.

### Configuration Files

#### `ckpool.conf.example`
Example configuration file for pool mode. Copy to `ckpool.conf` and customize.
Contains bitcoind connection settings, pool address, difficulty settings, server
bindings, and all pool-specific options.

#### `ckproxy.conf.example`
Example configuration file for proxy mode. Copy to `ckproxy.conf` and customize.
Contains upstream pool connection settings and proxy-specific options.

#### `ckpassthrough.conf.example`
Example configuration file for passthrough mode. Copy to `ckpassthrough.conf` and
customize. Contains upstream pool settings and passthrough-specific options.

#### `ckredirector.conf.example`
Example configuration file for redirector mode. Copy to `ckredirector.conf` and
customize. Contains upstream pool settings and redirect URL configuration.

#### `cknode.conf.example`
Example configuration file for node mode. Copy to `cknode.conf` and customize.
Contains bitcoind connection settings and upstream pool settings for hybrid node
operation.

### Build System Files

#### `configure.ac` and `configure`
Autotools configuration files for build system setup. Detects system capabilities, libraries, and generates `config.h` and Makefiles.

#### `Makefile.am` and `Makefile.in`
Automake files defining build rules, source files, and compilation targets.

#### `autogen.sh`
Script to regenerate autotools files from source. Required when building from git repository.

#### `compile`, `config.guess`, `config.sub`, `depcomp`, `install-sh`, `ltmain.sh`, `missing`
Autotools helper scripts for cross-platform compilation and installation.

### Documentation Files

#### `README`
Primary documentation covering design philosophy, features, building instructions, running options, and configuration parameters.

#### `README-SOLO`
Instructions for setting up and running ckpool in solo mining mode.

#### `AUTHORS`
List of project authors and contributors with their contact information and Bitcoin addresses.

#### `COPYING`
GNU General Public License version 3 text.

#### `ChangeLog`
Version history and release notes.

### Third-Party Components

#### `src/jansson-2.14/`
Embedded JSON parsing library (Jansson). Used throughout ckpool for JSON-RPC communication with bitcoind, Stratum protocol messages, and configuration file parsing.

#### `src/uthash.h` and `src/utlist.h`
Header-only hash table and linked list libraries. Used for efficient data structures throughout the codebase.

#### `src/sha256_code_release/`
Additional SHA-256 implementation code (if present).

---

## Workflow and Interactions

### Miner to Pool Communication (Stratum Protocol)

1. **Connection Establishment**
   - Miner connects via TCP to the pool's listening socket (default port 3333)
   - Connector process accepts the connection and creates a client instance
   - Connection is registered with the stratifier process via Unix socket

2. **Subscription**
   - Miner sends `mining.subscribe` message
   - Stratifier responds with subscription result containing:
     - Extranonce1 (unique per worker)
     - Extranonce2 size
     - Subscription ID

3. **Authorization**
   - Miner sends `mining.authorize` with username and password
   - Username typically identifies the worker (e.g., "address.workername")
   - In BTCSOLO mode, username must be a valid Bitcoin address
   - Stratifier validates and responds with authorization result

4. **Work Distribution**
   - Stratifier sends `mining.notify` messages containing:
     - Job ID
     - Previous block hash
     - Coinbase transaction parts (coinb1, coinb2)
     - Merkle branch hashes
     - Block version
     - Network difficulty target (nbits)
     - Current time (ntime)
   - Work is generated by the generator process from bitcoind's `getblocktemplate`
   - Each workbase is assigned a unique ID and distributed to all authorized miners

5. **Share Submission**
   - Miner sends `mining.submit` with:
     - Username
     - Job ID
     - Extranonce2
     - Time (ntime)
     - Nonce
   - Stratifier validates the share:
     - Checks job ID validity and staleness
     - Reconstructs block header
     - Performs double SHA-256 hash
     - Compares hash against target difficulty
     - Checks for duplicate shares
   - Valid shares are accepted and statistics updated
   - Invalid shares are rejected with error codes

6. **Difficulty Adjustment (Vardiff)**
   - Pool monitors share submission rate per miner
   - Difficulty is automatically adjusted to target optimal share submission frequency
   - Adjustments are sent via `mining.set_difficulty` messages
   - Minimum, starting, and maximum difficulty are configurable

7. **Block Solving**
   - When a share meets the network difficulty target, it represents a solved block
   - Stratifier detects block solution and notifies generator process
   - Generator submits the block to bitcoind via `submitblock` RPC
   - Block reward is distributed according to pool mode (pool mode vs. BTCSOLO mode)

### Pool to Bitcoind Communication (JSON-RPC)

1. **Connection Management**
   - Generator process maintains persistent connections to configured bitcoind instances
   - Multiple bitcoind instances can be configured for failover
   - Connections use JSON-RPC over HTTP
   - Authentication via RPC username/password or cookie file

2. **Block Template Retrieval**
   - Generator requests new work via `getblocktemplate` RPC call
   - Bitcoind returns:
     - Current block height
     - Previous block hash
     - Transaction set (or transaction hashes)
     - Coinbase value
     - Difficulty target (nbits)
     - Block version
     - Network rules (segwit, etc.)
   - Generator processes template and creates workbase structure
   - Workbase is sent to stratifier for distribution to miners

3. **Block Change Detection**
   - Primary method: ZMQ notifications via `zmqpubhashblock` (if configured)
   - Secondary method: `notifier` utility with bitcoind's `-blocknotify` option
   - Fallback method: Polling `getbestblockhash` at configured interval (`blockpoll`)
   - When new block detected, generator fetches new template immediately

4. **Block Submission**
   - When miner solves a block, stratifier reconstructs full block data
   - Generator submits block to bitcoind via `submitblock` RPC
   - Bitcoind validates and processes the block
   - Generator handles submission responses and errors

5. **Address and Transaction Validation**
   - Pool validates Bitcoin addresses via `validateaddress` RPC
   - Detects address type (P2PKH, P2SH, SegWit)
   - Validates transactions via `getrawtransaction` when needed
   - Ensures coinbase transactions are properly formatted

6. **Network Information**
   - Generator queries block height via `getblockcount`
   - Retrieves block hashes via `getblockhash` for validation
   - Monitors network difficulty from block templates

### Inter-Process Communication

1. **Unix Domain Sockets**
   - Main process creates Unix domain sockets in `/tmp` (configurable)
   - Three child processes communicate via these sockets:
     - Generator: Manages bitcoind communication
     - Stratifier: Manages miner communication
     - Connector: Manages network I/O

2. **Message Queues**
   - Each process uses message queues for asynchronous communication
   - Messages are queued and processed by dedicated threads
   - Prevents blocking between processes

3. **Process Coordination**
   - Main process spawns and monitors child processes
   - Handles graceful shutdown and socket handover
   - Supports seamless restarts via `-H` (handover) option

---

## Modes of Operation

### Pool Mode (Default)
**Purpose**: Operate as a standalone Bitcoin mining pool.

**Characteristics**:
- Requires local or remote bitcoind connection
- Accepts direct miner connections
- Distributes work to miners
- Validates and aggregates shares
- Submits solved blocks to bitcoind
- Distributes block rewards to miners based on shares

**Configuration**: `ckpool.conf`

**Command**: `ckpool` (no special flags)

**Use Cases**:
- Running a public or private mining pool
- Solo mining with multiple devices
- Pool mining with reward distribution

### BTCSOLO Mode
**Purpose**: Solo mining mode where each miner mines to their own Bitcoin address.

**Characteristics**:
- Username must be a valid Bitcoin address
- 100% of block reward goes to the address that solved the block
- No share aggregation or reward distribution
- Minimal pool overhead

**Configuration**: `ckpool.conf` (btcaddress optional)

**Command**: `ckpool -B` or `ckpool --btcsolo`

**Use Cases**:
- Individual miners wanting full block rewards
- Testing mining setups
- Low hash rate solo mining

### Proxy Mode
**Purpose**: Act as an intermediary between miners and an upstream pool.

**Characteristics**:
- Accepts connections from downstream miners
- Forwards all shares to upstream pool as a single user
- Appears as one worker to upstream pool regardless of downstream miner count
- Requires upstream pool to be ckpool-compatible for scalability
- Can aggregate multiple miners into one upstream connection

**Configuration**: `ckproxy.conf`

**Command**: `ckpool -p` or `ckpool --proxy` (or use `ckproxy` symlink)

**Use Cases**:
- Aggregating multiple miners behind NAT
- Reducing connection overhead for upstream pool
- Providing local proxy for remote miners

### Passthrough Mode
**Purpose**: Combine multiple miner connections into a single upstream connection while preserving individual miner identity.

**Characteristics**:
- Accepts connections from downstream miners
- Streams all miner data on single connection to upstream pool
- Each downstream miner retains individual presence on upstream pool
- Scales to millions of clients by isolating main pool from direct client communication
- Standalone mode implied

**Configuration**: `ckpassthrough.conf`

**Command**: `ckpool -P` or `ckpool --passthrough`

**Use Cases**:
- Large-scale pool deployments
- Isolating main pool infrastructure from client connections
- Reducing network overhead on main pool

### Node Mode
**Purpose**: Hybrid mode combining passthrough functionality with local bitcoind and block submission capability.

**Characteristics**:
- Behaves like passthrough but requires local bitcoind
- Can submit blocks itself in addition to forwarding to upstream
- Monitors hashrate and requires more resources than simple passthrough
- Upstream pools must specify dedicated IPs/ports for node communication

**Configuration**: `cknode.conf`

**Command**: `ckpool -N` or `ckpool --node`

**Use Cases**:
- Distributed pool architecture with local block submission
- Redundant block submission paths
- Regional pool nodes with local bitcoind

### Redirector Mode
**Purpose**: Front-end filter that redirects active miners to other pools.

**Characteristics**:
- Filters out users that never contribute shares
- Once accepted share detected from upstream, issues redirect to configured URLs
- Cycles over multiple redirect URLs if configured
- Attempts to keep clients from same IP redirecting to same pool

**Configuration**: `ckredirector.conf`

**Command**: `ckpool -R` or `ckpool --redirector`

**Use Cases**:
- Load balancing across multiple pools
- Filtering inactive miners before redirect
- Distributing miners across pool infrastructure

### Userproxy Mode
**Purpose**: Proxy mode that preserves individual miner credentials to upstream pool.

**Characteristics**:
- Accepts username/password from stratum connections
- Opens additional connections with those credentials to upstream pool
- Reconnects miners to mine with their chosen username/password upstream
- Each miner maintains separate identity on upstream pool

**Configuration**: `ckproxy.conf`

**Command**: `ckpool -u` or `ckpool --userproxy`

**Use Cases**:
- Transparent proxy preserving miner authentication
- Multi-pool aggregation with individual tracking
- Credential passthrough scenarios

### Library Mode
**Purpose**: Use libckpool as a standalone library in other software.

**Characteristics**:
- Core functionality available as linkable library
- Provides utilities for Bitcoin mining operations
- Can be integrated into custom mining software
- Independent of ckpool executable

**Use Cases**:
- Custom mining pool implementations
- Integration into existing software
- Building specialized mining tools

---

## Additional Features

### Difficulty Management
- Variable difficulty (vardiff) automatically adjusts per miner
- Configurable minimum, starting, and maximum difficulty
- Supports sub-"1" difficulty for low hash rate miners (LHR feature)
- Rapid adjustment with stable unlimited maximum difficulty handling

### Share Logging
- Optional per-share logging enabled with `-L` flag
- Logs organized by block height and workbase
- Stored in configured log directory

### Seamless Restarts
- Socket handover allows zero-downtime upgrades
- `-H` flag enables handover from existing instance
- `-k` flag kills existing instance before starting

### Multiple Instances
- Can run multiple named instances concurrently
- `-n` flag specifies instance name
- Each instance uses separate configuration and sockets

### Statistics and Monitoring
- Real-time pool and per-client statistics
- Queryable via `ckpmsg` utility
- Accurate share and hashrate tracking

### Network Features
- Event-driven communication prevents slow clients from blocking fast ones
- Configurable socket buffer sizes
- Support for multiple server bindings
- IPv4 and IPv6 support

