# PS3ME 01.24 network support

This change replaces the 1.23 PCSL offline stubs with an IPv4 transport using
PSL1GHT/libnet. Applications use the network connection already configured in
PS3 system settings, whether wireless or Ethernet. No Wi-Fi password or SSID
needs to be entered in PS3ME.

## Implemented

- Asynchronous hostname lookup, nonblocking TCP connect/read/write, UDP datagrams,
  TCP listeners/accept, socket options, address/port queries and output shutdown.
- Cooperative readiness polling through the existing MIDP event queue. DNS runs
  on SDL worker threads; those workers do not call the VM or retain Java buffers.
- Unique 31-bit descriptors avoid PPU pointer truncation and recycled-socket
  wakeups. Closed connections wake pending readers/writers, including when the
  event queue was temporarily full. All sockets close when a Java session ends.
- HTTP PUT, PATCH, DELETE and OPTIONS, alongside existing GET, HEAD and POST.
- Optional API type linkage used by Discord J2ME: FileConnection,
  FileSystemRegistry and Nokia SoftNotification. **These are type contracts,
  not file access or PS3 notification implementations.** Their capability
  properties remain absent. There are no accessible JSR-75 roots or file://
  provider. This avoids verification failures in disabled application paths.

The transport currently allows 32 sockets and four outstanding DNS requests.
Reverse DNS is unsupported. It does not replace phoneME's existing SSL engine
with modern TLS, and does not claim arbitrary modern HTTPS compatibility.

## Discord J2ME 5.3.1

Reviewed upstream tag v5.3.1, commit
`b51078417dd00ff2f8b2853a9fdb98683fe3e2da`, and the supplied unmodified
`discord_jl.jar`, SHA-256
`5dc12dbe960cb0c76538a7a6b2f2ad77f96bca852273f0828f05054f13e1c5c1`.

Its default configuration uses HTTP to `146.59.80.3` and TCP gateway port 8081,
with a separate HTTP CDN port 8080. Keep the application's default proxy mode.
Its proxyless HTTPS option has not been validated. Account login, messaging,
QR login and attachment operations have not been verified on PS3 hardware.
No account credentials were used and no messages were sent during these tests.

Do not confuse the JAR's verification results with a successful Discord login.
The supplied JAR is not modified or bundled in the emulator PKG.

## Validation and limits

- Host transport tests use the actual PCSL implementation with real local TCP
  and UDP sockets and SDL threads, under AddressSanitizer and UBSan. Tests cover
  delayed reads, queue backpressure, partial writes, EOF, half-close, socket
  options/endpoints, close during blocked reads and 100 consecutive sessions.
  LeakSanitizer is disabled because the execution environment uses ptrace.
- PPU cross-compilation and linking pass. The output is checked as big-endian
  ELF64 PowerPC64 with Cell LV2 imports.
- The modified HTTP classes are compiled against the existing CLDC runtime and
  processed by the actual CLDC preverifier.
- All 99 classes of the provided Discord JAR pass CLDC preverification against
  the resulting class library. Originally six failed on missing optional APIs.
- The PKG is independently checked for version/title ID, payload equality,
  class-library CRC and correct finalized package structure. No test JARs ship.
- Real outbound socket tests to the default Discord proxy are blocked by this
  execution environment (`ENETUNREACH`). That does not establish whether the
  public proxy is currently online or offline.
- RPCS3 was downloaded and invoked, but reports Missing Firmware. Downloads of
  Sony's official firmware failed with HTTP 503. No successful RPCS3 runtime
  test or physical-console test is claimed for this build.

## Building the change from recovered 1.23 archives

Fetch the pinned SDK/phoneME sources with `tools/fetch_deps.py`. Existing 1.23
VM/MIDP archives and class library are required for this incremental route.
Use a JDK 8 compiler and a working CLDC preverifier (32-bit runner if needed).

```sh
python3 tools/build_pcsl_ppu.py
python3 tools/build_network_java.py --jdk /path/to/jdk8
python3 tools/build_application.py --reuse-properties
python3 tools/package_application.py --output build/PS3ME_01.24_Network.pkg
```

On systems needing legacy libcrypto.so.1.1 for the SDK signing tools, pass
`--crypto-libs /path/to/legacy/lib` to the packaging step. Full MIDP rebuilds
also apply and package the Java compatibility changes automatically.

With SDL 1.2 development packages installed, `make -C ps3 test-network` runs
transport tests. The optional `--discord` argument on that test binary checks
unauthenticated HTTP and TCP reachability when outbound sockets are permitted;
it does not log into an account. A failure there is not a local regression test
failure and must be recorded separately.

Two headers missing from the GitHub 1.23 snapshot (BrowserUI.hpp and
Ps3Backend.hpp) were restored from its official 1.23 release source archive so
that the current native sources compile. The 1.23 XMB heartbeat/flip-guard logic
is retained.
