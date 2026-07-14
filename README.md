# Project Genesis

Deterministisches Industrie- und Kolonieaufbauspiel auf Basis von Unreal Engine 5.

## Technischer Stand

- Engine-Zuordnung: Unreal Engine `5.8`
- Sprache: C++ mit Blueprint-Präsentationsschicht
- Entwicklungsumgebung: Visual Studio Code und MSVC Build Tools
- Architektur: getrennte Runtime-Module für Spielstart, Basistypen und Simulation
- Qualitätswerkzeuge: clang-format, clang-tidy, EditorConfig und Unreal Automation Tests
- aktueller Implementierungsschwerpunkt: Phase 0 / `TICK-GEN-001-01`

## Verbindliche Quellen

Produkt-, Architektur- und Implementierungsanforderungen werden aus `Ninja200411/wikijs` übernommen. Für die laufende Umsetzung gelten insbesondere:

- `de/project/genesis/04-implementierung/README.md`
- `de/project/genesis/04-implementierung/epics-und-tickets.md`
- `de/project/genesis/04-implementierung/github-issue-register.md`
- `de/project/genesis/04-implementierung/unreal-engine-5-projektaufbau.md`
- `de/project/genesis/04-implementierung/code-quality-und-linting.md`
- `de/project/genesis/04-implementierung/clean-code-richtlinien.md`

GitHub-Issues im Wiki-Repository definieren Scope, Akzeptanzkriterien und Reihenfolge. Implementierung und Tests in diesem Repository müssen auf eine Ticket-ID zurückführbar sein.

## Module

| Modul | Verantwortung |
|---|---|
| `ProjectGenesis` | Unreal-Spielmodul und Integration |
| `GenesisCore` | stabile IDs, Basistypen und gemeinsame Verträge |
| `GenesisSimulation` | deterministische Simulationsclock, Tick-Scheduler und Systemausführung |

Weitere Fachmodule werden erst ergänzt, wenn die jeweiligen Epic-Tickets umgesetzt werden.

## Aktueller Slice: TICK-GEN-001-01

Implementiert sind:

- feste Tickdauer als ganzzahlige Mikrosekunden,
- Trennung von Realzeit und Simulationszeit,
- Pause, Resume, Single-Step sowie `1x`, `2x` und `4x`,
- stabile Phasen `Input`, `Simulation`, `Resolution`, `Events`, `ReadModels`,
- Sortierung nach Phase, Priorität und stabiler System-ID,
- Tick- und Systembudgetmessung,
- begrenzte Tickverarbeitung pro Frame ohne Verwerfen des Rückstands,
- Automation Tests für Pause, Einzelschritt, Geschwindigkeit und Ausführungsreihenfolge.

Noch offen innerhalb des Tickets:

- State-Hash und 10.000-Tick-Determinismustest,
- persistierbarer Execution Plan für Save/Load,
- Telemetrieexport der Messwerte,
- Debug-UI für Clock und Scheduler.

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
./Scripts/test/run-automation.ps1
```

Liegt Unreal an einem anderen Ort:

```powershell
./Scripts/setup/generate-vscode.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
./Scripts/build/build-editor.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
./Scripts/test/run-automation.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
```

Gezielter Testfilter:

```powershell
./Scripts/test/run-automation.ps1 -TestFilter "Genesis.Simulation."
```

## Projekt öffnen

Nach erfolgreichem Build `ProjectGenesis.uproject` öffnen. Falls Unreal eine andere Engine-Zuordnung verwendet, über **Switch Unreal Engine Version** die lokale UE-5.8-Installation auswählen und anschließend die Projektdateien erneut generieren.

## Wichtige Regeln

- Fachlogik gehört in C++-Module, nicht in Level Blueprints.
- Simulationszustände werden nicht durch Actor-Ticks gesteuert.
- Zustandsänderungen erfolgen ausschließlich über definierte Commands, sobald `TICK-GEN-001-03` umgesetzt ist.
- Simulationselemente dürfen nicht von ungeordneten Hash- oder Collection-Iterationen abhängen.
- `Binaries`, `Intermediate`, `Saved` und `DerivedDataCache` werden nicht versioniert.
- Binäre Unreal-Assets unter `Content/` werden über Git LFS verwaltet.
- Neue Module benötigen eine dokumentierte Verantwortlichkeit und gerichtete Abhängigkeiten.
