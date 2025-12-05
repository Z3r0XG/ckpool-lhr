# CKPOOL-LHR Installation Scripts

## install-ckpool-solo.sh

Automated installer for CKPOOL-LHR in solo mining mode with Bitcoin Core.

### Requirements

- Ubuntu/Debian or Fedora/CentOS/RHEL
- Root/sudo access
- Minimum 10GB disk space (pruned) or 700GB+ (full chain)
- Internet connection

> [!IMPORTANT]
> Full blockchain sync can take several days. Pruned mode is faster but has some limitations.

### Usage

```bash
sudo ./scripts/install-ckpool-solo.sh
```

The script will prompt for:
- **Service user**: User account to run services (default: current user)
- **Disk space**: Pruned or full blockchain
- **Assumevalid**: Block hash to speed up sync
- **Donation**: Optional 0.5% donation to the fork maintainer
- **Coinbase signature**: Optional signature in mined blocks

### What Gets Installed

| Component | Location |
|-----------|----------|
| Bitcoin Core | `/usr/local/bin/bitcoind`, `/usr/local/bin/bitcoin-cli` |
| CKPOOL-LHR | `/usr/local/bin/ckpool` (source at `/opt/ckpool`) |
| ckpool config | `/etc/ckpool/ckpool.conf` |
| ckpool logs | `/var/log/ckpool/` |
| Bitcoin data | `~/.bitcoin/` |

### Systemd Services

Two services are created and enabled:

**bitcoind.service**
```ini
[Unit]
Description=Bitcoin Daemon
After=network.target

[Service]
User=<service_user>
ExecStart=/usr/local/bin/bitcoind -conf=<datadir>/bitcoin.conf -datadir=<datadir> -printtoconsole
Restart=always

[Install]
WantedBy=multi-user.target
```

**ckpool.service**
```ini
[Unit]
Description=CKPool-LHR Solo
After=bitcoind.service

[Service]
User=<service_user>
ExecStart=/bin/bash -c '/usr/local/bin/wait-for-bitcoind-sync.sh && exec /usr/local/bin/ckpool -B -q -c /etc/ckpool/ckpool.conf'
StandardOutput=journal
StandardError=journal
Restart=always

[Install]
WantedBy=multi-user.target
```

### Managing Services

```bash
# Check status
systemctl status bitcoind
systemctl status ckpool

# View logs
journalctl -u bitcoind -f
journalctl -u ckpool -f

# Restart services
systemctl restart bitcoind
systemctl restart ckpool

# Stop services
systemctl stop ckpool
systemctl stop bitcoind
```

### Monitoring

```bash
# ckpool logs
tail -f /var/log/ckpool/ckpool.log

# Bitcoin Core logs
tail -f ~/.bitcoin/debug.log

# Sync progress (while syncing)
journalctl -u ckpool -f
```

### Configuration Files

**Bitcoin Core** (`~/.bitcoin/bitcoin.conf`):
- RPC authentication (auto-generated)
- ZMQ block notifications enabled
- Pruning (if configured)

**CKPool** (`/etc/ckpool/ckpool.conf`):
- Bitcoind connection details
- Starting difficulty: 10000
- Solo mode enabled via `-B` flag in service

### Post-Installation

1. Wait for blockchain sync (can take days for full chain)
2. Monitor sync: `journalctl -u ckpool -f`
3. Once synced, connect miners:
   - URL: `stratum+tcp://<server-ip>:3333`
   - Username: Your Bitcoin address (optionally with `.workername`, e.g., `bc1q...abc.worker1`)
   - Password: `x`

### Uninstalling

> [!CAUTION]
> Removing blockchain data (`~/.bitcoin`) will require a full re-sync if you reinstall.

```bash
# Stop and disable services
sudo systemctl stop ckpool bitcoind
sudo systemctl disable ckpool bitcoind

# Remove service files
sudo rm /etc/systemd/system/ckpool.service
sudo rm /etc/systemd/system/bitcoind.service
sudo systemctl daemon-reload

# Remove installed files
sudo rm -rf /opt/ckpool /etc/ckpool /var/log/ckpool
sudo rm /usr/local/bin/wait-for-bitcoind-sync.sh

# Optionally remove Bitcoin Core
sudo rm /usr/local/bin/bitcoin*

# Optionally remove blockchain data
rm -rf ~/.bitcoin
```

### Troubleshooting

> [!TIP]
> **ckpool not starting?**
> - Check if blockchain is synced: `bitcoin-cli getblockchaininfo`
> - ckpool waits for sync before starting

> [!TIP]
> **Connection refused on port 3333?**
> - Verify ckpool is running: `systemctl status ckpool`
> - Check firewall: `sudo ufw allow 3333/tcp`

> [!TIP]
> **RPC errors?**
> - Verify bitcoind is running: `systemctl status bitcoind`
> - Check RPC credentials match between configs
