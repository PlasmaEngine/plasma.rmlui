# plasma.rmlui

In-game user interface built from HTML and CSS-like documents, powered by
[RmlUi](https://github.com/mikke89/RmlUi). Moved out of the engine tree to be built and distributed
as a package.

Published as [`plasma.rmlui`](https://plasmaengine.github.io/PlasmaPackages/index.plPackageIndex) in
the Plasma package registry.

## Layout

```
Package.plPackage       the manifest - identity, compatibility, dependencies, components
RmlUi.plPluginBundle    legacy descriptor, kept until the engine reads manifests directly
Source/
  RmlUiPlugin/          runtime               -> plRmlUiPlugin
  EnginePluginRmlUi/    editor engine process -> plEnginePluginRmlUi
  EditorPluginRmlUi/    editor                -> plEditorPluginRmlUi  (RmlUi asset type)
ThirdParty/
  RmlUi/                upstream RmlUi, its SVG plugin, lunasvg and FreeType
Configs/                build rules for building against an installed engine SDK
Branding/               icon and banner
```

RmlCore, RmlDebugger, RmlSvg and lunasvg are all compiled into `plRmlUiPlugin`. The one
redistributable is FreeType, which ships in the runtime component with `FTL.TXT` beside it as
`freetype-LICENSE.txt` — FreeType is dual-licensed under the FreeType License and GPLv2, and the FTL
permits redistribution with attribution.

The bundle used to also list `RmlCore.dll` in `PackageDependencies`. No such file has ever existed
in `Binaries/` — RmlCore is statically linked — so that entry was dropped rather than carried
forward.

## Dependencies

- `plasma.assets ^1.0` — `plEditorPluginRmlUi` defines an asset document type, so it builds on
  `EditorPluginAssets` and must load after it. `plasma.assets` is a built-in that ships with the
  engine, but it is declared like anything else so the store can reason about it.

## Building

Requires an engine SDK — a built engine checkout — named by `PLASMA_SDK_ROOT`:

```bash
PLASMA_SDK_ROOT=/path/to/engine plasmabuild build EditorPluginRmlUi
```

Each of the three targets builds separately. Output lands in `Bin/Windows_x64_Development/`.

`RmlUiPlugin` also reads shader headers from the SDK's `Data/Base`, so the SDK must be a real engine
checkout rather than a headers-only drop.

## A note on the engine's copy

`Code/ThirdParty/RmlUi` stays in the engine even though this package vendors its own copy, because
`GameComponentsPlugin` — a template default that stays in the engine — includes FreeType headers
from that path. Removing it there would break a plugin that has nothing to do with RmlUi.

## Publishing a new version

Staging and publishing are driven from the engine and registry repositories:

```bash
# engine repo - stage and validate, emitting machine-readable output
python Utilities/PackageInspector.py <staged-package-dir> --json <out>.json

# registry repo - upload blobs and record the release
python tools/publish_package.py <out>.json
```

Versions are immutable. A mistake is fixed by publishing a new patch version, never by replacing an
existing one — every lock file that already trusts a version must keep resolving to the same bytes.
