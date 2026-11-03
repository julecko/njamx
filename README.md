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

## Packaging (`-p`)

`./build.sh --release -p` / `.\build.bat --release -p` builds a
distributable package after building:

- **Linux**: a `.deb` (via CPack) whose postinstall script grants
  `CAP_NET_RAW` (and, if you say yes when asked, `CAP_NET_ADMIN`) to the
  installed binary with `setcap`, so it doesn't need `sudo` on every
  run. Needs `cpack`.
- **Windows**: an NSIS installer (`packaging/windows/installer.nsi`,
  built directly with `makensis` - needs NSIS installed, not CPack's
  NSIS generator, since that can't do what's described below). It:
  - Lets you choose "install for me only" or "install for all users"
    (the latter needs admin and elevates automatically), followed by
    the normal browsable destination-folder page.
  - Bundles and launches Npcap's own installer interactively (not
    silently - Npcap's free license reserves unattended install for
    its paid OEM tier).
  - Asks whether to add njamx's folder to `PATH`, and if so, for your
    account only or for all users (the latter only offered when
    installing for all users). Removed again on uninstall.
