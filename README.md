# OpenNR for ReShade

A ReShade add-on that runs NVIDIA's DLSS 5 neural rendering (NR) network on AMD Radeon cards. It is the ReShade side of
the OpenNR project, for games and emulators where the [OptiScaler route](https://github.com/MatheusFerreiraS/OpenNR-OptiScaler)
doesn't fit: D3D11 games, older APIs, 32-bit games and emulators.

The network itself comes from danielblnc's [DLSS-NR-on-AMD](https://github.com/danielblnc/DLSS-NR-on-AMD) runtime or,
on RX 9000, from mochizuki0323's [DLSSNR-AMD](https://gitlab.com/mochizuki0323/DLSSNR-AMD) Vulkan port. This project
does not reimplement it. Current release: v0.7.15.

Discord: <https://discord.gg/wYhvS3JSHM>

Ko-fi: <https://ko-fi.com/T6T213OVFE>

## What you need

| | |
|---|---|
| GPU | Radeon RX 6000, RX 7000 or RX 9000 with the HIP 7 runtime (`amdhip64_7.dll`). Current Adrenalin drivers include it on RX 7000 and RX 9000; RX 6000 needs AMD's HIP SDK 7.2. mochizuki needs an RX 9000 and no HIP. |
| Renderer | Direct3D 11 works best. Direct3D 12 works, but the OptiScaler route is better there. Direct3D 10, Vulkan and OpenGL are experimental, and so are 32-bit games (D3D8, D3D9, D3D10, D3D11, OpenGL). |
| ReShade | The build "with full add-on support", 6.x. The normal build cannot load add-ons. |
| Disk | About 150 MB for the weights. |

## Install

Get the [OpenNR Installer](https://github.com/zmodelerlover/OpenNR-Installer/releases). It finds your games, picks the
renderer, downloads ReShade, the add-on, the runtime and the weights, checks every file, and can uninstall it all.
There is a [video of the whole install](https://www.youtube.com/watch?v=L2v0b98wReQ).

Two things to know:

- The renderer detection is usually right, not always. A wrong pick installs fine and then does nothing. Each game
  card links to its PCGamingWiki page.
- For emulators, the renderer you pick inside the emulator decides whether the add-on works. The installer tells you
  which one.

## Turn it on

Press **Home** to open ReShade, go to the **Add-ons** tab and find **OpenNR**. It starts off: tick **Enabled** or press
**Ctrl+End**. **On at startup** turns it on when the game opens.

Every control has a `(?)` tooltip, and settings save to `amd-nr.ini` as you change them. The main ones:

- **Scale**: size of the network input. Smaller is faster and loses fine detail.
- **Passes**: one to three runs of the network. More passes give a stronger effect and can add grain.
- **Limit** (Debug section): how much the image may change.
- **NR runtime**: danielblnc or mochizuki (RX 9000). Applies on the next game start.

Red controls can crash the display driver. Everything else is under **More settings**.

## Runtimes

- **danielblnc**: the add-on runs 0.6.0, which the installer puts in, and still accepts 0.5.1, 0.5.0, 0.4.3, 0.4.2 and
  0.4.1. It checks the file's hash and refuses anything else.
- **mochizuki** (RX 9000): `MochizukiNrRuntime.dll` and its `dlssnr-amd\` folder, built by the OpenNR OptiScaler fork.
  The installer adds it on RX 9000 cards. The first start in each game builds the network, which can take up to a
  minute; frames go through untouched until it's ready. Its log is `mochizuki_nr.log`.

## Motion vectors

The network wants colour, depth, motion and exposure. On D3D11 games that render a velocity buffer, the add-on hands
over the game's own vectors. Everywhere else the 64-bit add-on estimates motion with AMD FidelityFX optical flow.

An optical-flow shader you already run can feed it instead: copy `effects/AMD_Neural_Feed.fx` into your ReShade
shaders, and enable it below a provider such as iMMERSE Launchpad (<https://martysmods.com>), VORT or LumeniteFX. The
**Status** column says which source is in use.

## Troubleshooting

| What you see | What to do |
|---|---|
| The add-on isn't in the Add-ons tab | Delete the `DisabledAddons=` line under `[ADDON]` in `ReShade.ini`. |
| `HIP: amdhip64_7.dll failed to load` | Install HIP 7. On RX 6000, install AMD's HIP SDK 7.2 and restart the game. |
| `dlssnr_amd_pass1.dll ... Refused.` | That file is not a build the add-on knows. Compare it with `tools/SHA256SUMS.txt`. |
| `missing:` and a path | Put that file at exactly that path. |
| The game crashes with `887A0005` | The display driver reset. Lower the Scale. |
| `off: the GPU held one network job for ... s` | danielblnc's runtime stalled. On RX 9000, switch NR runtime to mochizuki. |

To report a problem, send the panel's status line (`N frames, Z% skipped`) and both logs next to the game's `.exe`:
`amd-nr.log` and `dlssnr_on_amd.log`. Screenshots rarely help with flicker. For installer problems, use its
**Report a problem** button.

## Building

You don't need to build anything to use this.

```powershell
.\build.ps1 -Target neural       # amd-nr.addon64
.\build-x86bridge.ps1            # amd-nr.addon32 and amd-nr-host64.exe, for 32-bit games
```

Needs Visual Studio with the C++ tools and the Windows SDK. The ReShade and Dear ImGui headers are in `3rdparty/`.
`.\tools\gates.ps1 -All` builds every target and runs the checks.

## Limits

- Only D3D11 passes the game's own motion vectors. D3D12 gets the game's depth and optical flow; D3D10, Vulkan, OpenGL
  and 32-bit D3D9 get the final image only.
- No FSR upscaling, and none planned.
- Tested by one person on one card (RX 9070 XT).

## Support

[![Support this project on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/T6T213OVFE)

Nothing is locked behind a donation. The code is MIT either way.

## Credits

- [DLSS-NR-on-AMD](https://github.com/danielblnc/DLSS-NR-on-AMD) by danielblnc: the runtime and weights this add-on
  drives. Neither is reimplemented or redistributed here, and they have their own terms.
- [DLSSNR-AMD](https://gitlab.com/mochizuki0323/DLSSNR-AMD) by mochizuki0323 (MIT): the optional Vulkan runtime. The
  add-on loads it and ships none of it.
- `core/temporal/smooth.inc` and `core/temporal/motion_feed.inc` port shaders from lmxxf's
  [dlss5-on-amd-9070xt-porting](https://github.com/lmxxf/dlss5-on-amd-9070xt-porting) (MIT), notice in
  `docs/third-party/lmxxf-LICENSE.md`.
- Thanks to everyone who ran a build and sent back a log.

## License

MIT, see `LICENSE`. Headers in `3rdparty/` keep their own licences, listed in `3rdparty/reshade/NOTICE.md`.
