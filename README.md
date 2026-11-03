# Project

An ARP scanner/jammer with an msfconsole-style CLI. Builds and runs on
both Linux and Windows; packet capture/injection goes through
[libpcap](https://www.tcpdump.org/) on Linux and
[Npcap](https://npcap.com/) (its Windows-API-compatible counterpart) on
Windows.

## Building

### Linux

Requires `libpcap-dev`, `pkg-config`, CMake and a C compiler.

```sh
sudo apt install build-essential cmake pkg-config libpcap-dev   # Debian/Ubuntu
./build.sh -d          # Debug build
./build.sh --release
```

Capturing/injecting raw frames needs `CAP_NET_RAW`, so run the built
binary as root (or `setcap cap_net_raw+ep` on the executable).

### Windows

Requires Visual Studio (with the C++ build tools) and CMake.

1. Install the [Npcap runtime](https://npcap.com/#download) (needed to
   run the program, not just build it).
2. Run `build.bat -d` (or `--release`). On first configure, CMake
   downloads the Npcap SDK (headers + import libs) into
   `third_party/npcap-sdk` automatically. If that download fails (e.g.
   no network access from the build machine), download the SDK zip
   yourself from the link above and extract it there, or point
   `-DNPCAP_SDK_DIR=<path>` at an existing copy.

Npcap restricts raw adapter access to Administrators by default, so run
the built `njamx.exe` from an elevated terminal.

### Interface names

`-i`/`--interface` takes the interface's OS name: `eth0`/`wlan0` on
Linux, and the adapter's friendly name as shown in "Network
Connections" (e.g. `Ethernet`, `Wi-Fi`) on Windows.

### MAC spoofing (`-m`/`--mac-change`)

On Linux this flips the interface down, sets the new hardware address,
and brings it back up (root required).

On Windows there's no direct equivalent, so it writes the adapter's
`NetworkAddress` registry override and cycles the adapter with `netsh`
to apply it. This requires Administrator privileges and a NIC driver
that honors the override (most Ethernet/Wi-Fi drivers do, some don't).
