# Geant4 Neutron Production in a Cubic Phantom

This repository contains a Geant4 simulation used to study secondary neutron production from monoenergetic proton beams 
in a water phantom, used to benchmark stochastic differential equation (SDE) models of neutron transport in proton therapy.

This simulation is part of the MaThRad collaboration work on stochastic differential equation (SDE) models for proton therapy radiation transport.

---

## Overview

- The setup consists of a **1 × 1 × 1 m³ water phantom** irradiated by a **monoenergetic proton beam**.
- The Geant4 output file contains:
    - Information about each individual neutron produced (momentum, origin position, and kinetic energy)
    - Neutron production from proton inelastic processes (the kinetic energy of the proton when it underwent a nuclear inelastic process and how many neutrons it produced)
    - A 3D histogram of neutron production
- The geometry can be set to one of 3 different phantoms: **water, slab** or **insert**, set via UI commands in the macro file.
- A Python pipeline (`run_geant4.py`) wraps the Geant4 executable, runs simulations across one or more physics lists, and produces neutron distribution and multiplicity plots from the resulting ROOT files.
---

## Physics Lists

Physics lists no longer need to be hardcoded in `ProtonTherapy.cc`. 
Instead, they are passed at runtime via the Python script (see below), which selects the physics list using the 
Geant4 executable's command-line arguments. The default physics list is `QGSP_BIC`.

## Simulation Macros

Each macro modifies the phantom geometry using the command `/phantom/select <geometry>` at the start.
The possible macro configurations are the same as the proton Geant4 SDE model: water, slab, and insert.

| Phantom type            | Description                                       |
|-------------------------|---------------------------------------------------|
| Water                   | Pure water phantom                                |
| Slab                    | 2 cm water + 1 cm bone + 2 cm lung + water        |
| Insert                  | 3 cm water + 2 cm bone at top half only + water   |
---

## Running the Simulation

Ensure your Geant4 environment is sourced correctly before running:

```bash
source /path/to/geant4-install/bin/geant4.sh
```

Simulations are run through the Python script `run_geant4.py`. On first run, it automatically configures and compiles the project (creating the `build/` directory and running `cmake`/`make`) if no compiled executable is found, then runs the simulation for each requested physics list and generates neutron distribution and multiplicity plots.

```bash
python run_geant4.py -c <n_cores> [-e <energy>] [-n <n_primaries>] [-p <phantom>] [-l <physics_lists>]
```

**Arguments:**

| Flag | Description                                              | Default     | Required |
|------|-----------------------------------------------------------|-------------|----------|
| `-e` | Beam energy in MeV                                         | `150`       | No       |
| `-n` | Number of protons in the beam                               | `1E6`       | No       |
| `-c` | Number of CPU threads to use for the run                    | —           | **Yes**  |
| `-p` | Phantom geometry: `water`, `slab`, or `insert`               | `water`     | No       |
| `-l` | Physics list(s) to simulate (space-separated, no commas)     | `QGSP_BIC`  | No       |

**Examples:**

```bash
# Minimum required args
python run_geant4.py -c 10
 
# Specifying all args, including multiple physics lists
python run_geant4.py -e 150 -n 1E6 -c 10 -p water -l QGSP_BIC FTFP_BERT
```

Each run will generate an output `.root` file containing per-neutron data and the 3D production histogram, written to the `Output/` directory by default (configurable in the macro file). The Python script then produces neutron distribution and multiplicity figures from these `.root` files, also saved to `Output/`.

For visualisation only, run the Geant4 executable directly without a macro file:
```bash
./ProtonTherapy
```
Then, set the desired geometry with the command `/phantom/select <geometry>` in the Geant4 idle session and run `/control/execute refresh_vis.mac` to visualise the updated phantom.
 
---

## Notes

- This code was developed and tested using **Geant4 version 11.3.0**. Detailed instructions on how to install Geant4 and its prerequisites can be found in https://geant4.web.cern.ch/download/11.3.0.html.
- The Python script `run_geant4.py` is written in Python 3.13, and requires the `matplotlib`, `numpy`, `uproot4` and `pandas` packages.