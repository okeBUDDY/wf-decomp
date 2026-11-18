# Warning Forever 1.08 decompilation

Byte-matching decompilation of `wf.exe` (Warning Forever 1.08, Hikoza T. Ohkubo, 2005).
Work in progress; see the sections below as they are filled in.

## Setup (short)
- Linux with Wine 9 (32-bit) and Python 3 (`capstone`, `pefile`).
- Toolchains are not committed. Point `WF_VC71` at a VS.NET 2003 `Vc7` directory
  (cl 13.10.3077, link 7.10.3077, with `PlatformSDK/`) and `WF_VC70` at a VS.NET 2002
  `Vc7` directory (cl 13.00.9466). Defaults: `/root/wf/tools/vc71/Vc7`, `/root/wf/tools/vc70`.
- Put the original `wf.exe` into `orig/` (SHA-1 in `config/wf.sha1`).
