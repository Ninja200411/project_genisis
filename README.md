# Project Genesis

Deterministisches Industrie- und Kolonieaufbauspiel auf Basis von Unreal Engine 5.

## Technischer Stand

- Engine-Zuordnung: Unreal Engine `5.8`
- Sprache: C++ mit Blueprint-Präsentationsschicht
- Entwicklungsumgebung: Visual Studio Code und MSVC Build Tools
- Architektur: getrennte Runtime-Module für Spielstart, Basistypen und Simulation
- Qualitätswerkzeuge: clang-format, clang-tidy und EditorConfig

## Module

| Modul | Verantwortung |
|---|---|
| `ProjectGenesis` | Unreal-Spielmodul und Integration |
| `GenesisCore` | stabile IDs, Basistypen und gemeinsame Verträge |
| `GenesisSimulation` | deterministische Simulationsclock und Systemausführung |

Weitere Fachmodule werden erst ergänzt, wenn die jeweiligen Epic-Tickets umgesetzt werden.

## Lokales Setup unter Windows

Voraussetzungen:

1. Unreal Engine 5.8 unter `C:\Program Files\Epic Games\UE_5.8` oder einem eigenen Pfad.
2. Visual Studio Build Tools mit Desktopentwicklung mit C++ und Windows SDK.
3. Visual Studio Code mit den empfohlenen Workspace-Erweiterungen.
4. Git LFS.

Nach dem Klonen:

```powershell
git lfs install
./Scripts/setup/generate-vscode.ps1
./Scripts/build/build-editor.ps1
```

Liegt Unreal an einem anderen Ort:

```powershell
./Scripts/setup/generate-vscode.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
./Scripts/build/build-editor.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
```

Alternativ in VS Code:

- `Terminal > Run Task > Genesis: Generate VS Code files`
- `Terminal > Run Build Task > Genesis: Build Editor`

## Projekt öffnen

Nach erfolgreichem Build `ProjectGenesis.uproject` öffnen. Falls Unreal eine andere Engine-Zuordnung verwendet, über **Switch Unreal Engine Version** die lokale UE-5.8-Installation auswählen und anschließend die Projektdateien erneut generieren.

## Wichtige Regeln

- Fachlogik gehört in C++-Module, nicht in Level Blueprints.
- Simulationszustände werden nicht durch Actor-Ticks gesteuert.
- Zustandsänderungen erfolgen später ausschließlich über Commands.
- `Binaries`, `Intermediate`, `Saved` und `DerivedDataCache` werden nicht versioniert.
- Binäre Unreal-Assets unter `Content/` werden über Git LFS verwaltet.
- Neue Module benötigen eine dokumentierte Verantwortlichkeit und gerichtete Abhängigkeiten.

## Dokumentation

Die vollständige Produkt-, Architektur- und Implementierungsdokumentation liegt im Repository `Ninja200411/wikijs` unter `de/project/genesis`.
