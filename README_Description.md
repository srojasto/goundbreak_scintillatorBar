# scintbar — muons in a plastic scintillator bar

A minimal Geant4 (11.4.x) application that shoots muons through a wrapped plastic
scintillator bar, tracks the scintillation and Cherenkov photons, and counts the photons
detected by a SiPM glued to each end of the bar.

## Contents

- [What is simulated](#what-is-simulated)
- [Requirements](#requirements)
- [Build](#build)
- [Run](#run)
- [Outputs](#outputs)
- [Analysing the ROOT file](#analysing-the-root-file)
- [Changing the setup](#changing-the-setup)
- [Code structure](#code-structure)
- [Limitations](#limitations)

---

## What is simulated

```
                 mu- (4 GeV, from z = +15 cm, going -z)
                  |
                  v
      +--------------------------------------------------------+
  []<|          plastic scintillator bar                      |>[]
      |          100 cm (x) x 5 cm (y) x 1 cm (z)              |
      +--------------------------------------------------------+
  ^ ^           diffuse reflective wrapping (95 %)             ^ ^
  | K9 light guide 4.3 -> 1.0 mm, 4.95 mm long      light guide | SiPM (R), copy 1, x > 0
  SiPM (L), copy 0, x < 0
```

| Component | Description |
|---|---|
| World | Air box (RINDEX = 1.0) |
| Bar | `G4_PLASTIC_SC_VINYLTOLUENE`, 100 × 5 × 1 cm, centred at the origin, long axis along x |
| Scintillation (EJ-200 / BC-408-like) | 10 000 photons/MeV, emission 375–500 nm peaking at 425 nm, decay time 2.1 ns, Birks constant 0.126 mm/MeV |
| Bulk optics | Refractive index 1.58, attenuation length 380 cm |
| Wrapping | `groundfrontpainted` surface with 95 % reflectivity (diffuse, Teflon/Tyvek-like) on every bar face touching air |
| Light guides (LG-PYR-V3) | Two truncated square pyramids in K9 glass (≈ N-BK7), entrance 4.3 × 4.3 mm on the bar end, exit 1.0 × 1.0 mm, length 4.95 mm (wall angle 18.4°). RINDEX from the N-BK7 Sellmeier equation (1.535 at 375 nm, 1.521 at 500 nm), density 2.51 g/cm³. Side walls bare polished by default (TIR). Can be switched off with `kUseLightGuide = false` |
| SiPMs | Two SiPM windows (SiO₂, RINDEX = 1.55, 0.5 mm thick) at the guide exits: 1.0 × 1.0 mm with the light guide, 6 × 6 mm directly on the bar ends without it. All interfaces bar → guide → SiPM are optically coupled (no air gap) |
| Detection | A photon that enters a SiPM is counted with probability PDE = 0.40 and then killed |
| Primary | `mu-`, 4 GeV, vertical, through the bar centre |
| Physics | `FTFP_BERT` + `G4OpticalPhysics` (scintillation, Cherenkov, bulk absorption, boundary processes, Rayleigh, WLS) |

---

## Requirements

- Geant4 11.4.x built with Qt (for the GUI) and multithreading.
- CMake ≥ 3.16 and a working C++17 compiler (Apple Clang from the Command Line Tools).
- Optional: ROOT, to analyse `scintbar.root` (Geant4 writes the ROOT file itself; ROOT is not needed to run the simulation).
- Keep conda deactivated (`conda deactivate`) when building and running, so its libraries do not get mixed with the Geant4 build.

Load the Geant4 environment in each new terminal (or add it to `~/.zshrc`):

```bash
source /Users/solangel/sw/geant/geant4-v11.4.2-install/bin/geant4.sh
```

---

## Build

```bash
cd /Users/solangel/sw/geant
mkdir scintbar-build
cd scintbar-build
cmake ../scintbar -DGeant4_DIR=/Users/solangel/sw/geant/geant4-v11.4.2-install/lib/cmake/Geant4
make -j$(sysctl -n hw.ncpu)
```

CMake copies `vis.mac` and `run.mac` into the build directory. If you edit the macros in the
source directory, re-run `cmake` (or `make`) so the copies are updated, or edit the copies
in the build directory directly.

---

## Run

### Interactive (GUI)

```bash
./scintbar
```

This opens the Qt window and executes `vis.mac` (geometry, trajectories colour-coded:
muon red, electrons yellow, optical photons green). Then, in the command box:

```
/run/beamOn 1
```

Shoot only 1–2 events in the GUI: each muon produces ~10⁴ optical photons and drawing all
of them is slow.

### Batch

```bash
./scintbar run.mac
```

`run.mac` runs 1000 events on 8 threads:

```
/run/numberOfThreads 8
/run/initialize
/run/printProgress 100
/gun/particle mu-
/gun/energy 4 GeV
/gun/position 0 0 15 cm
/gun/direction 0 0 -1
/run/beamOn 1000
```

Useful variations (put them before `/run/beamOn`):

```
/gun/energy 1 GeV                  # different muon energy
/gun/position 40 0 15 cm           # hit the bar 40 cm from the centre, closer to the right SiPM
/gun/direction 0.5 0 -0.866        # inclined track (30 degrees)
/gun/particle mu+                  # positive muons
/random/setSeeds 12345 67890       # reproducible run
```

A position scan along the bar (to measure light attenuation) is a series of
`/gun/position X 0 15 cm` + `/run/beamOn N` blocks. Each `/run/beamOn` overwrites
`scintbar.root`, so for a scan add `/analysis/setFileName scintbar_x40` (with a different
name per position) before each `/run/beamOn`.

---

## Outputs

### 1. Terminal: per-event line

Printed for events 0–9 and then every 100th event:

```
Event 3 | Edep = 1.93 MeV | produced: scint = 17350, Cherenkov = 212 | detected: left = 41, right = 38
```

(Numbers are illustrative.)

| Field | Meaning |
|---|---|
| `Edep` | Total energy deposited in the bar by all charged particles in the event (muon + delta electrons), in MeV |
| `scint` | Number of optical photons created by the `Scintillation` process in the bar |
| `Cherenkov` | Number of optical photons created by the `Cerenkov` process in the bar |
| `left`, `right` | Number of photons detected by the SiPM at x < 0 (copy 0) and x > 0 (copy 1), after applying the PDE |

### 2. Terminal: run summary

Printed once by the master thread at the end of each run:

```
================ Run summary ================
 Events                     : 1000
 Mean Edep in bar           : ... MeV
 Mean photons produced      : ...
 Mean photons detected (L+R): ... +- ... (RMS)
=============================================
```

- *Mean photons produced* = scintillation + Cherenkov.
- *Mean photons detected (L+R)* is the sum of both SiPMs. The value after `+-` is the RMS
  (event-to-event spread), not the error on the mean. The error on the mean is RMS/√N.

### 3. ROOT file: `scintbar.root`

Written in the directory where you run the program. With multithreading the per-thread
ntuples are merged into this single file.

It contains one TTree, `photons`, with one entry per event:

| Branch | Type | Units | Meaning |
|---|---|---|---|
| `edep_MeV` | double | MeV | Energy deposited in the bar |
| `nScint` | int | photons | Scintillation photons produced |
| `nCerenkov` | int | photons | Cherenkov photons produced |
| `nDetLeft` | int | photons | Photons detected by the left SiPM (x < 0) |
| `nDetRight` | int | photons | Photons detected by the right SiPM (x > 0) |
| `xGun_mm` | double | mm | x position of the muon gun along the bar (identifies the scan point) |

---

## Position scan

`scan.mac` shoots muons at 1, 2, …, 9 cm from the SiPM-side (left) end of the bar, one run
per position, and writes one file per position (`scintbar_d1cm.root` … `scintbar_d9cm.root`).
Each point is executed by `scan_point.mac` through `/control/foreach`.

```bash
./scintbar scan.mac
hadd -f scan_all.root scintbar_d*cm.root
root -l 'plotScan.C("scan_all.root", 48.)'
```

`plotScan.C` prints the mean detected photons (left SiPM) with its error per position and saves
`scan_detected_vs_distance.pdf`. The second argument is the bar half-length in mm.

Edit at the top of `scan.mac`: `barHalf` (must equal `kBarLength / 2` in mm), `nEvents` (events
per point), and the list of distances in the `/control/foreach` line.

---

## Analysing the ROOT file

Quick look in an interactive ROOT session:

```bash
root -l scintbar.root
```

```cpp
photons->Print();
photons->Draw("edep_MeV");                  // energy deposit (Landau-like)
photons->Draw("nScint");                    // photons produced
photons->Draw("nDetLeft+nDetRight");        // total detected photons
photons->Draw("nDetLeft:nDetRight", "", "colz");
photons->Draw("(nDetLeft+nDetRight)/edep_MeV");  // detected photons per MeV
```

Mean number of detected photons with its error, as a small macro (`meanDetected.C`):

```cpp
void meanDetected()
{
  TFile* file = TFile::Open("scintbar.root");
  if (file == nullptr) {
    printf("Cannot open scintbar.root\n");
    return;
  }

  TTree* tree = nullptr;
  file->GetObject("photons", tree);
  if (tree == nullptr) {
    printf("Tree 'photons' not found\n");
    return;
  }

  tree->Draw("nDetLeft+nDetRight>>hDet(200,0,200)", "", "goff");
  TH1* hDet = nullptr;
  gDirectory->GetObject("hDet", hDet);

  printf("Entries           : %.0f\n", hDet->GetEntries());
  printf("Mean detected     : %.2f +- %.2f\n", hDet->GetMean(), hDet->GetMeanError());
  printf("RMS               : %.2f\n", hDet->GetRMS());
}
```

```bash
root -l -q meanDetected.C
```

Adjust the histogram range (`200,0,200`) if your detected counts are larger.

---

## Changing the setup

All geometry and material parameters are constants at the top of
`src/DetectorConstruction.cc`:

| Constant | Default | Meaning |
|---|---|---|
| `kBarLength` | 100 cm | Bar length (x) |
| `kBarWidth` | 5 cm | Bar width (y) |
| `kBarThickness` | 1 cm | Bar thickness (z), crossed by the muon |
| `kUseLightGuide` | true | Place the K9 pyramid light guides between bar and SiPMs |
| `kGuideEntrance` | 4.3 mm | Light-guide entrance (bar side) side length |
| `kGuideExit` | 1.0 mm | Light-guide exit (SiPM side) side length; also the SiPM size when the guide is used |
| `kGuideLength` | 4.95 mm | Light-guide length along the optical axis |
| `kGuideMirrorWalls` | false | false = bare polished walls (TIR); true = mirror-coated walls |
| `kGuideMirrorReflectivity` | 0.95 | Reflectivity of the mirror coating (if used) |
| `kSensorSizeNoGuide` | 6 mm | SiPM active area side when the guide is not used |
| `kSensorThickness` | 0.5 mm | SiPM window thickness |
| `kWrapReflectivity` | 0.95 | Reflectivity of the wrapping |
| `kLightYield` | 10000 / MeV | Scintillation yield |
| `kDecayTime` | 2.1 ns | Scintillation decay time |
| `kBulkAttLength` | 380 cm | Bulk attenuation length |
| `kRefIndexPlastic` | 1.58 | Refractive index of the bar |
| `kBirksConstant` | 0.126 mm/MeV | Birks quenching constant |
| `kRefIndexSensor` | 1.55 | Refractive index of the SiPM window / coupling |

The emission spectrum is the `wavelengths` / `emission` table in the same file.

The photon detection efficiency is `kPDE` (default 0.40) at the top of
`src/SteppingAction.cc`. Set it to `1.0` to count every photon that reaches a SiPM.

The default muon is set in `src/PrimaryGeneratorAction.cc`, but it is easier to change it
with `/gun/...` commands in a macro.

After editing any `.cc` file, rebuild with `make` in the build directory.

---

## Code structure

```
scintbar/
├── CMakeLists.txt
├── scintbar.cc                    main(): run manager, FTFP_BERT + optical physics, UI
├── vis.mac                        GUI visualisation settings
├── run.mac                        batch run (1000 muons, 8 threads)
├── include/  src/
│   ├── DetectorConstruction       geometry, materials, optical properties, wrapping
│   ├── PrimaryGeneratorAction     particle gun (mu-)
│   ├── ActionInitialization       registers the user actions (master and workers)
│   ├── RunAction                  ROOT ntuple definition, run summary
│   ├── EventAction                per-event counters, fills the ntuple
│   └── SteppingAction             counts produced photons, Edep, detected photons (PDE)
```

How the counting works (`SteppingAction`):

1. For every step of a charged particle inside `Bar`, the energy deposit is added to the
   event total, and every optical-photon secondary created in that step is counted as
   scintillation or Cherenkov, according to its creator process.
2. When an optical photon takes its first step inside a `Sensor` volume, it is counted for
   that SiPM (copy number 0 = left, 1 = right) with probability PDE and then killed, so it is
   never counted twice.

---

## Limitations

- The optical parameters are generic EJ-200/BC-408 datasheet values, not measurements of a specific bar.
- The PDE does not depend on wavelength; a real SiPM PDE curve is not included.
- There is no WLS fibre: the SiPMs read the bar through the light guides (or directly at the end faces).
- The light-guide entrance (4.3 × 4.3 mm) covers only ~3.7 % of the 50 × 10 mm bar end face; the rest of the end face is treated as wrapped.
- K9 is modelled with N-BK7 Sellmeier coefficients, a typical BK7 oxide composition and typical BK7 absorption; use the supplier datasheet values if available.
- The wrapping is an idealised surface with no air gap between the bar and the reflector.
- There is no SiPM or electronics response (no crosstalk, afterpulsing, dark counts, gain, or waveform). The output is the number of detected photons (≈ photoelectrons).
- Photon arrival times are not stored.
