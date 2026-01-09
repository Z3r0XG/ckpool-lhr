# CKPOOL-LHR

A fork of CKPool featuring sub-"1" difficulty support for low hash rate miners
(ESP32 devices and others), along with additional enhancements for solo mining.

Ultra low overhead, scalable, multi-process, multi-threaded Bitcoin mining
pool software in C for Linux.

## Key Features

- Sub-"1" difficulty support for low hash rate miners
- Bitcoind cookie authentication
- User agent whitelisting
- User agent reporting (pool.status and user pages)
- Network difficulty monitoring (pool.status)
- Worker connection timestamp tracking
- Configurable donation address

## Acknowledgment

This software is a fork of CKPool by Con Kolivas. The original project is
provided free of charge under the GPLv3 license. We honor and acknowledge
Con Kolivas's foundational work that made this fork possible.

**Original project:** https://bitbucket.org/ckolivas/ckpool

## Design

**Architecture:**

- Multiprocess + multithreaded design to scale to massive deployments and capitalize on modern multicore/multithread CPU designs
- Minimal memory overhead and low-level hand-coded architecture for maximum efficiency
- Ultra-reliable Unix sockets for inter-process communication
- Modular code design to streamline further development
- Event-driven communication preventing slow clients from delaying low-latency ones

**Features:**

- Bitcoind communication with multiple failover to local or remote locations
- Virtually seamless restarts for upgrades through socket handover
- Configurable custom coinbase signature
- Configurable instant starting and minimum difficulty (including sub-1.0 for low hashrate miners)
- Rapid vardiff adjustment with stable unlimited maximum difficulty handling
- New work generation on block changes incorporate full bitcoind transaction set without delay
- Stratum messaging system to running clients
- Accurate pool and per-client statistics with user agent tracking
- Multiple named instances can run concurrently on the same machine

## License

GNU Public license V3. See included COPYING for details.

---

# Building

Building ckpool-lhr requires basic build tools and yasm on any Linux installation. ZMQ notification support (recommended) requires the zmq devel library installed.

### Building with ZMQ (recommended)

```bash
sudo apt-get install build-essential yasm libzmq3-dev
./configure
make
```

### Basic build (without ZMQ)

```bash
sudo apt-get install build-essential yasm
./configure
make
```

### Building from git

Requires additional autotools:

```bash
git clone https://github.com/Z3r0XG/ckpool-lhr.git
cd ckpool-lhr
sudo apt-get install build-essential yasm autoconf automake libtool libzmq3-dev pkgconf
./autogen.sh
./configure
make
```

### Binaries

Binaries will be built in the `src/` subdirectory:

- **ckpool** - The main pool backend
- **ckpmsg** - Application for passing messages to ckpool
- **notifier** - Application for bitcoind's `-blocknotify` to notify ckpool of block changes

### Installation

Installation is **not required** and ckpool can be run directly from the build directory, but it can be installed system-wide with:

```bash
sudo make install
```

---

# Solo Mode

## Use Case

Solo mode allows miners to mine directly to their own Bitcoin address without
pooling with others. Each miner specifies their Bitcoin address as their
username, and any blocks found are paid directly to that address.

**Ideal for:**
- Local solo mining operations
- Miners who want direct block rewards
- Testing and development
- Lottery-style mining to personal addresses

## How Solo Mode Works

In solo mode, ckpool-lhr connects to a Bitcoin daemon (bitcoind) to receive
block templates. Miners connect and provide their Bitcoin address as their
username. When a block is found, the reward goes directly to the miner's
specified address.

**Workflow**: Bitcoind provides block template → Pool generates work → Miner
connects with Bitcoin address as username → Miner submits shares → Block found →
Reward sent directly to miner's address.

---

## Required Options

### Command Line

**`-B` or `--btcsolo`**: **REQUIRED** to start ckpool-lhr in solo mode.

Example:
```bash
src/ckpool -B
```

### All Command Line Options

**`-B | --btcsolo`**
- Start ckpool in solo mode (required for solo mining)

**`-c CONFIG | --config CONFIG`**
- Specify path to configuration file (default: ckpool.conf)

**`-D | --daemonise`**
- Run ckpool as a daemon (background process)

**`-g GROUP | --group GROUP`**
- Run as specified group

**`-H | --handover`**
- Handover mode: Take over sockets from old instance, then shut it down
- Implies -k (killold)

**`-h | --help`**
- Display help message with all options

**`-k | --killold`**
- Send shutdown message to old instance with same name

**`-L | --log-shares`**
- Log shares to disk (can generate large logs)

**`-l LOGLEVEL | --loglevel LOGLEVEL`**
- Set log level (0=emergency to 7=debug, default: 5=notice)

**`-n NAME | --name NAME`**
- Set process name (default: ckpool, cknode, etc.)

**`-N | --node`**
- Run as a passthrough node (combines proxy + passthrough)
- Cannot be combined with other proxy modes

**`-P | --passthrough`**
- Passthrough proxy mode (relays to upstream pool)
- Cannot be combined with other proxy modes

**`-p | --proxy`**
- Standard proxy mode
- Cannot be combined with other proxy modes

**`-q | --quiet`**
- Disable warnings and non-critical messages

**`-R | --redirector`**
- Redirector mode (minimal proxy that redirects clients)
- Cannot be combined with other proxy modes

**`-s SOCKDIR | --sockdir SOCKDIR`**
- Directory for unix domain sockets

**`-t | --trusted`**
- Trusted remote mode (cannot be combined with proxy modes)

**`-u | --userproxy`**
- User-based proxy mode
- Cannot be combined with other proxy modes

> [!NOTE]
> Most users only need `-B` for solo mining. Other options are for advanced configurations.

> [!WARNING]
> Proxy modes (`-p`, `-P`, `-N`, `-R`, `-u`) are mutually exclusive and cannot be combined with solo mode (`-B`).

### Configuration File

**"btcd"** : Bitcoind connection configuration. **REQUIRED**
- Type: Array of objects
- Each entry: url (string, required), auth (string, required), pass (string, required),
  cookie (string, optional), notify (boolean, optional)
- Default: localhost:8332 with user "user" and password "pass" if not specified
- Note: Cookie-based authentication can be used as an alternative to auth/pass by
  specifying the cookie file path.
- Example:
```json
"btcd" : [
    {
        "url" : "127.0.0.1:8332",
        "auth" : "username",
        "pass" : "password",
        "notify" : true
    }
]
```

---

## Quick Start

### Option 1: Automated Installation

> [!TIP]
> **Recommended for most users.** This script handles all dependencies and configuration.

Run the installation script as root/sudo:
```bash
sudo ./scripts/install-ckpool-solo.sh
```

This automatically installs Bitcoin Core and ckpool-lhr, sets up systemd
services, and configures everything for solo mining.

### Option 2: Manual Setup

#### 1. Configure Bitcoin Daemon

Edit `bitcoin.conf` to enable RPC:
```
server=1
rpcuser=username
rpcpassword=password
rpcallowip=127.0.0.1
rpcbind=127.0.0.1
```

Enable ZMQ block notifications (recommended):
```
zmqpubhashblock=tcp://127.0.0.1:28332
```

Or use the notifier script as an alternative:
```
blocknotify=/path/to/ckpool/src/notifier
```

#### 2. Create Configuration File

Copy the example and customize:
```bash
cp ckpool.conf.example ckpool.conf
```

Edit `ckpool.conf` with required settings:
```json
{
"btcd" : [
    {
        "url" : "127.0.0.1:8332",
        "auth" : "username",
        "pass" : "password",
        "notify" : true
    }
],
"logdir" : "logs"
}
```

#### 3. Start ckpool-lhr in Solo Mode

```bash
src/ckpool -B
```

#### 4. Configure Your Miners

Point mining hardware to the pool:
- **URL**: `192.168.1.100:3333` (pool IP address, default port 3333)
- **Username**: Your Bitcoin address, optionally with a worker name (e.g., `bc1q8qkesw5kyplv7hdxyseqls5m78w5tqdfd40lf5.worker1`)
- **Password** (optional parameters):
    - Default: any value (e.g., `x`)
    - Supports comma-separated parameters (e.g., `x, diff=200, f=9`)
    - `diff`: Suggest difficulty after authorization. Format: `diff=X` where `X` is numeric (e.g., `diff=0.001`). Clamped to pool `mindiff`.

Any valid Bitcoin address works as the username. Append `.workername` to track multiple miners separately.

---

## Optional Configuration Options

All configuration options relevant to solo mode are listed below. The `btcaddress`
option is ignored in solo mode (miners provide their own address as username).

**"donaddress"** : Bitcoin address for donation payments. **OPTIONAL**
- Type: String
- Values: Any valid Bitcoin address
- Default: bc1q8qkesw5kyplv7hdxyseqls5m78w5tqdfd40lf5
- Note: Only used when "donation" is configured and greater than 0.
- Example: `"donaddress" : "bc1q..."`

**"donation"** : Percentage of block reward to donate. **OPTIONAL**
- Type: Double
- Values: 0.0 to 99.9
- Default: 0 (disabled)
- Note: Values below 0.1 are treated as 0 to avoid dust-sized donations.
- Example: `"donation" : 0.5`

**"btcsig"** : Signature to include in coinbase of mined blocks. **OPTIONAL**
- Type: String
- Values: Up to 38 bytes
- Default: None
- Example: `"btcsig" : "/solo mined/"`
- Note: Truncated to 38 bytes at load time (bytes, not characters); multibyte UTF-8 uses multiple bytes. If truncated, a warning is logged.
- To check: `echo -n "<sig>" | wc -c` (bytes)

**"serverurl"** : Server bindings for miner connections. **OPTIONAL**
- Type: Array of strings
- Values: "IP:port" or "hostname:port"
- Default: All interfaces on port 3333
- Note: Ports below 1024 usually require privileged access
- Example: `"serverurl" : ["192.168.1.100:3333", "127.0.0.1:3333"]`

**"mindiff"** : Minimum difficulty for vardiff. **OPTIONAL**
- Type: Double
- Values: Any positive number
- Default: 1
- Note: Supports sub-"1" values (e.g., 0.0005) for low hash rate miners.
- Example: `"mindiff" : 1` or `"mindiff" : 0.0005`

**"startdiff"** : Starting difficulty for new clients. **OPTIONAL**
- Type: Double
- Values: Any positive number
- Default: 42
- Note: Can be set below 1 for low hash rate miners.
- Example: `"startdiff" : 42` or `"startdiff" : 0.0005`

**"maxdiff"** : Maximum difficulty for vardiff. **OPTIONAL**
- Type: Double
- Values: Any positive number, or 0 for no maximum
- Default: 0 (no maximum)
- Note: Caps vardiff adjustments to prevent difficulty from growing too high.
- Example: `"maxdiff" : 10000000` or `"maxdiff" : 0`

**"highdiff"** : Starting difficulty for high-hashrate server ports. **OPTIONAL**
- Type: Double
- Values: Any positive number
- Default: 1000000
- Note: Used when specific server ports are designated as "highdiff" ports.
- Example: `"highdiff" : 100000`

**"allow_low_diff"** : Remove minimum network difficulty floor (for regtest). **OPTIONAL**
- Type: Boolean
- Default: false
- Note: Enables block submission to blockchains with sub-1.0 network difficulty (e.g., regtest).
- Warning: For regtest testing only. Do NOT enable on mainnet or testnet.
- Example: `"allow_low_diff" : true`

**"nonce1length"** : Length of extranonce1 in bytes. **OPTIONAL**
- Type: Integer
- Values: 2-8
- Default: 4

**"nonce2length"** : Length of extranonce2 in bytes. **OPTIONAL**
- Type: Integer
- Values: 2-8
- Default: 8

**"update_interval"** : Frequency to send stratum updates. **OPTIONAL**
- Type: Integer
- Values: Seconds (positive integer)
- Default: 30

**"version_mask"** : Allowed version bits for clients to modify. **OPTIONAL**
- Type: String (hex)
- Default: "1fffe000"
- Example: `"version_mask" : "1fffe000"`

**"blockpoll"** : Frequency to poll bitcoind for new blocks. **OPTIONAL**
- Type: Integer
- Values: Milliseconds
- Default: 100
- Note: Only used if "notify" is not set on btcd entry.

**"zmqblock"** : ZMQ interface for block notifications. **OPTIONAL**
- Type: String
- Values: ZMQ URL format
- Default: "tcp://127.0.0.1:28332"
- Note: Requires bitcoind -zmqpubhashblock option.

**"logdir"** : Directory for logs. **OPTIONAL**
- Type: String
- Default: "logs"

**"maxclients"** : Maximum concurrent client connections. **OPTIONAL**
- Type: Integer
- Default: 0 (auto-calculated as 90% of system's `ulimit -n`)

**"dropidle"** : Drop idle clients after this many seconds. **OPTIONAL**
- Type: Integer
- Default: 0 (disabled)
- Note: 1 hour (3600) is generous for low hash rate miners.

**"useragent"** : Allowed user agent strings (whitelist). **OPTIONAL**
- Type: Array of strings
- Default: None (all allowed)
- Note: Prefix matching. Empty user agents rejected if whitelist configured.
- Example: `"useragent" : ["cpuminer", "cgminer"]`

**"max_pool_useragents"** : Maximum distinct normalized useragents to include in pool.status. **OPTIONAL**
- Type: Integer
- Default: 100
- Note: Controls how many aggregated UA entries (normalized string + count) are included in pool/pool.status JSON; 0 disables publishing. Individual user/worker pages show the specific useragent string for each worker.

---

## Notes

> [!NOTE]
> **Difficulty rounding:** `mindiff`, `startdiff`, `maxdiff`, and `highdiff` values >= 1 are rounded to whole numbers. Values below 1 retain full precision.

> [!WARNING]
> JSON is strict with formatting. Do not put a comma after the last field.

> [!NOTE]
> You can mine with a pruned blockchain, though it may add latency.

> [!NOTE]
> Mining on testnet may create cascading solved blocks when difficulty is 1.
> This is normal behavior optimized for mainnet where block solving is rare.

---

## Other Modes

While ckpool-lhr is optimized and documented for solo mining, it inherits all capabilities from upstream CKPool and can also operate in the following modes:

- **Proxy mode** (`-p`) - Standard proxy appearing as a single user to upstream pool
- **Passthrough mode** (`-P`) - Collates connections while retaining individual presence on master pool
- **Node mode** (`-N`) - Passthrough node with local bitcoind for block submission
- **Redirector mode** (`-R`) - Front-end filter that redirects active miners
- **Userproxy mode** (`-u`) - User-based proxy accepting username/password credentials

> [!NOTE]
> These modes are inherited from upstream CKPool but are not the primary focus of ckpool-lhr. For detailed documentation on proxy/passthrough modes, refer to the original CKPool documentation.

> [!WARNING]
> Solo mode (`-B`) cannot be combined with any proxy modes.
