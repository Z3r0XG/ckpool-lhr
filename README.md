# CKPOOL-LHR

A fork of CKPOOL featuring sub-"1" difficulty support for low hash rate miners
(ESP32 devices and others), along with additional enhancements for solo mining.

Ultra low overhead, scalable, multi-process, multi-threaded Bitcoin mining
pool software in C for Linux.

## Key Features

- Sub-"1" difficulty support for low hash rate miners
- Bitcoind cookie authentication
- User agent whitelisting
- Configurable user data cleanup
- Configurable donation address

## Acknowledgment

This software is a fork of CKPOOL by Con Kolivas. The original project is
provided free of charge under the GPLv3 license. We honor and acknowledge
Con Kolivas's foundational work that made this fork possible.

**Original project:** https://bitbucket.org/ckolivas/ckpool

## License

GNU Public license V3. See included COPYING for details.

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
- **Password**: Anything (e.g., "x")

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
- Values: Up to 38 characters
- Default: None
- Example: `"btcsig" : "/solo mined/"`

**"serverurl"** : Server bindings for miner connections. **OPTIONAL**
- Type: Array of strings
- Values: "IP:port" or "hostname:port"
- Default: All interfaces on port 3333
- Note: Ports below 1024 usually require privileged access
- Example: `"serverurl" : ["192.168.1.100:3333", "127.0.0.1:3333"]`

**"mindiff"** : Minimum difficulty for vardiff. **OPTIONAL**
- Type: Double
- Values: Any positive number
- Default: 1.0
- Note: Supports sub-"1" values (e.g., 0.0005) for low hash rate miners.
- Example: `"mindiff" : 0.0005`

**"startdiff"** : Starting difficulty for new clients. **OPTIONAL**
- Type: Double
- Values: Any positive number
- Default: 42.0
- Note: Can be set below 1 for low hash rate miners.
- Example: `"startdiff" : 0.0005`

**"maxdiff"** : Maximum difficulty for vardiff. **OPTIONAL**
- Type: Integer64
- Values: Positive integer, or 0 for no maximum
- Default: 0 (no maximum)

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
- Default: 0 (no limit)

**"dropidle"** : Drop idle clients after this many seconds. **OPTIONAL**
- Type: Integer
- Default: 0 (disabled)
- Note: 1 hour (3600) is generous for low hash rate miners.

**"user_cleanup_days"** : Clean up inactive user data after this many days. **OPTIONAL**
- Type: Integer
- Default: 0 (never cleanup)

**"useragent"** : Allowed user agent strings (whitelist). **OPTIONAL**
- Type: Array of strings
- Default: None (all allowed)
- Note: Prefix matching. Empty user agents rejected if whitelist configured.
- Example: `"useragent" : ["cpuminer", "cgminer"]`

---

## Notes

> [!WARNING]
> JSON is strict with formatting. Do not put a comma after the last field.

> [!NOTE]
> You can mine with a pruned blockchain, though it may add latency.

> [!NOTE]
> Mining on testnet may create cascading solved blocks when difficulty is 1.
> This is normal behavior optimized for mainnet where block solving is rare.
