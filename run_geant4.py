"""
This script contains everything to run Geant4 and produce figures relating to neutron production
in a proton therapy scenario.
Tested in Geant4 v11.3 built with multithreading and data sets installed.
The Geant4 output file contains:
    - Information about each individual neutron produced (momentum, origin position and kinetic energy)
    - Neutron production from proton inelastic processes (Kinetic energy of the proton when it underwent
    nuclear inelastic process and how many neutrons it produced)
    - 3D histogram of neutron production
To run this python script, you have the following args:
    -- e: beam energy in MeV (default 150 MeV)
    -- n: number of protons in the beam (default 1E6)
    -- c: number of cpu threads to use for the run (required, default 4)
    -- p: phantom geometry: water, slab or insert (default water)
    -- l: list of physics lists to simulate (default QGSP_BIC)

Examples:
    min args: <python_executable> run_geant4.py -c 10
    for use of all args: <python_executable> run_geant4.py -e 150 -n 1E6 -c 10 -p water -l QGSP_BIC FTFP_BERT
"""
import subprocess
import uproot4
import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np
import pandas as pd
import os
import argparse

# ----------- Definitions of directories and files -----------------
project_dir = Path(__file__).parent
output_dir = project_dir / "Output"

build_dir = project_dir / "build"
geant_bin = build_dir / "ProtonTherapy"
macro_file = project_dir / "run0.mac"

# ------------------------ Functions -------------------------------

def ensure_geant_built():
    """Ensure the Geant4 code is compiled. If not, configure and build."""
    if geant_bin.exists():
        print("✔ Found compiled Geant4 executable.")
        return

    print("⚠ No compiled executable found. Building now...")
    build_dir.mkdir(exist_ok=True)

    # Run CMake
    cmake_cmd = ["cmake", ".."]
    print("→ Running CMake...")
    subprocess.run(cmake_cmd, cwd=build_dir, check=True)

    # Compile
    print("→ Running Make...")
    subprocess.run(["make"], cwd=build_dir, check=True)

    if not geant_bin.exists():
        raise RuntimeError("Compilation finished but executable not found.")

    print("✔ Geant4 project successfully built.")


def write_macro(root_filename,
                energy, n_primaries,
                phantom_type):
    """Writes a macro file with the desired name and contents.
    Filename args: name of a macro file, name of output root file
    Simulation args:
        - root_filename: name of output root file from geant4
        - energy: of the proton beam
        - n_primaries: number of protons in the beam
        - phantom_type: "water", "slab" or "insert", according to the geometries explored for the SDE paper
    """
    filename = project_dir / "run0.mac"
    n_primaries = int(n_primaries)
    macro_content = f"""
/phantom/select {phantom_type}
/run/initialize

# Source geometry:
/gps/particle proton
/gps/position -50 0 0 cm

/gps/pos/type Beam
# Incident surface is yz plane:
/gps/pos/rot1 0 1 0
/gps/pos/rot2 0 0 1

/gps/pos/shape Circle
/gps/pos/radius 0.5 mm
/gps/pos/sigma_r 0

/gps/ang/rot1 0 0 1
/gps/ang/rot2 0 1 0
/gps/ang/type beam1d
/gps/ang/sigma_r 0 deg

# Energy is gaussian profile
/gps/ene/type Gauss
/gps/ene/mono {energy} MeV
/gps/ene/sigma 0.0001 MeV

/output/rootFileName {output_dir}/{root_filename}.root

/run/beamOn {n_primaries}
"""
    filename.write_text(macro_content)
    print("✔ Macro file generated for the run.")

def run_geant(root_filename,
              energy, n_primaries,
              phantom_type,
              n_proc, phys_list):
    """Generates a macro file run0.mac and runs it, saves root output with the desired file name.
    Args: Same as the ones needed for the macro file function, plus:
     - n_proc: Number of cpu threads to use for the run
     - phys_list: Name of the physics list to use, e.g. "QGSP_BIC", "FTFP_BERT", "QGSP_INCLXX"
    """
    # Ensure Geant4 is built or build it if not
    ensure_geant_built()

    # Create the macro file
    write_macro(root_filename, energy, n_primaries, phantom_type)

    # Running make command copies the newly generated macro file into the build directory
    subprocess.run(["make"], cwd=build_dir, check=True)

    # Catch segfault error that has always been there, but it's not a worry
    try:
        # Run with args: macro and number of threads
        subprocess.run([str(geant_bin), "-m", str(macro_file), "-t", str(int(n_proc)), "-p", str(phys_list)],
                       check=True)
    except subprocess.CalledProcessError:
        print("✔ Geant4 finished, ignoring usual cleanup segfault.")

def read_root(root_filename, to_json=True):
    """
    Reads individual neutron information from root output file into Python, saves json file if requested.
    """
    root_path = root_filename if root_filename.endswith('.root') else f"{root_filename}.root"
    with uproot4.open(output_dir / root_path) as f:
        tree = f["NeutronInfo"]
        df = tree.arrays(['OriginPosX', 'OriginPosY', 'OriginPosZ', 'MomentumX', 'MomentumY', 'MomentumZ', 'eKin'], library="pd")
        if to_json:
            json_path = output_dir / (os.path.splitext(root_path)[0] + ".json")
            df.to_json(json_path)
        return df

def calculate_neutron_multiplicities(root_filename,
                               ep_bins=np.linspace(0, 180, 91),
                               save_nu_bar=False,
                               ):
    """
    From a root output file, extract the neutron multiplicities per proton energy, print total and
    proton-inelastic-induced neutron production yields. Computes the mean, standard deviation and number
    of neutrons per proton energy bin. We call the mean number of neutrons produced per proton energy bin "nu bar".
    Args:
        root_filename: name of the root output file, without the .root extension
        ep_bins: energy bins to use for the histogram (default: 91 bins from 0 to 180 MeV)
        save_nu_bar: whether to save the mean, standard deviation and number of neutrons per proton energy bin
    Returns: a pandas dataframe with the mean neutron multiplicities, standard deviation and count per proton energy bin
    """
    root_path = root_filename if root_filename.endswith('.root') else f"{root_filename}.root"
    f = uproot4.open(output_dir / root_path)
    tree = f["protonInelasticData"]
    df = tree.arrays(['protonEkin', 'NeutronMultiplicity'], library="pd")
    # Print some info
    ntot_interactions = len(df)
    ntot_neutrons = df['NeutronMultiplicity'].sum()
    print(f"Total yield: {ntot_neutrons / 1E6:.3f} neutrons per primary proton.")
    print(f"Interaction yield: {ntot_neutrons / ntot_interactions:.3f} neutrons per inelastic interaction.")

    # Bin energy data
    df['Ep_bin'] = pd.cut(df['protonEkin'], bins=ep_bins)
    # Let nu bar be the mean neutron multiplicity in each bin.
    # Only look at the multiplicity and compute mean, std and n counts per bin
    nu_bar = df.groupby('Ep_bin', observed=True)['NeutronMultiplicity'].agg(
        nu_mean='mean',
        nu_std='std',
        nu_count='count'
    )
    # Compute standard error of the mean
    nu_bar[('nu_stderr')] = nu_bar['nu_std'] / np.sqrt(nu_bar['nu_count'])

    if save_nu_bar:
        nu_bar.to_csv(output_dir / f"{root_filename}_neutron_yield_per_inelastic.csv")
    return nu_bar


def get_neutron_distributions(filenames, labels):
    """
    Plots production depth and kinetic energy distributions for neutrons in the given filenames.
    Args:
        - filenames: list of root filenames to read e.g. ["test1", "test2", "test3"]
        - labels: list of labels to use for the plots, with same size as filenames list.
        e.g. ["QGSP_BIC", "FTFP_BERT", "QGSP_INCLXX"] if the difference between files is the physics list
        or e.g. ["150 MeV", "180 MeV"] if the difference is the beam energy.
    """
    fig, ax = plt.subplots(2, 2, figsize=(10, 8))
    for f, l in zip(filenames, labels):
        df = read_root(f, to_json=True)
        depth = df['OriginPosX'] + 500 # Adding 500 mm as 1m3 phantom is placed at origin
        energy = df['eKin']
        theta = np.arctan2(np.sqrt(df['MomentumY']**2 + df['MomentumZ']**2), df['MomentumX'])
        phi = np.arctan2(df['MomentumZ'], df['MomentumY'])

        plot_configs = [
            (ax[0, 0], depth, 'Depth [mm]', 'linear'),
            (ax[0, 1], energy, '$E_n$ [MeV]', 'log'),
            (ax[1, 0], theta, r'$\theta$ [rad]', 'linear'),
            (ax[1, 1], phi, r'$\phi$ [rad]', 'linear'),
        ]
        for a, data, xlabel, yscale in plot_configs:
            a.hist(data, bins=200, label=l, histtype='step')
            a.set_xlabel(xlabel)
            a.set_ylabel('Neutron counts')
            a.set_yscale(yscale)
    for a in ax.flat:
        a.legend()
    plt.tight_layout()
    plt.show()
    return fig

def get_neutron_multiplicities(filenames, labels,
                               ep_bins=np.linspace(0, 180, 91)):
    """
    Plots the data obtained from get_neutron_multiplicities function for the filenames given.
    Args:
        - filenames: list of root filenames to read e.g. ["test1", "test2", "test3"]
        - labels: list of labels to use for the plots, with same size as filenames list.
        e.g. ["QGSP_BIC", "FTFP_BERT", "QGSP_INCLXX"] if the difference between files is the physics list
        or e.g. ["150 MeV", "180 MeV"] if the difference is the beam energy.
    """
    fig = plt.figure()
    for f, l in zip(filenames, labels):
        nu_bar = calculate_neutron_multiplicities(f, ep_bins)
        x = nu_bar.index.map(lambda x: x.mid) # Get energy bin midpoint
        plt.errorbar(x, nu_bar['nu_mean'], yerr=nu_bar['nu_stderr'], label=l, capsize=2, ms=4, linestyle='None', marker='o')
    plt.xlabel('Proton kinetic energy $E_p$ [MeV]')
    plt.ylabel(r'$\bar{\nu}(E_p)$ [n / protonInelastic interaction]')
    plt.title(r'Mean neutron multiplicity per proton kinetic energy', fontweight='bold')
    plt.legend()
    plt.grid()
    plt.show()
    return fig

def main():
    parser = argparse.ArgumentParser(description='Run a proton beam simulation in Geant4 and produce figures relating to neutron production')
    parser.add_argument("-e", "--energy", type=float, default=150, help="Beam energy in MeV")
    parser.add_argument("-n", "--n_primaries", type=str, default="1E6", help="Number of primaries (e.g. 1E6)")
    parser.add_argument("-c", "--n_proc", type=int, default=4, required=True, help="Number of cores")
    parser.add_argument("-p", "--phantom_type", type=str, default="water", help="Phantom type (default: water)")
    parser.add_argument("-l", "--lists", nargs="+", default=["QGSP_BIC"],
                        help="Physics lists to run")
    args = parser.parse_args()

    energy = args.energy
    n_primaries = args.n_primaries
    n_proc = args.n_proc
    phantom_type = args.phantom_type
    lists = args.lists

    # For several simulation runs of different physics lists:
    files = []
    for l in lists:
        rf= f"G4_{n_primaries}_{str(energy)}MeV_{l}"
        files.append(rf)
        if not os.path.exists(output_dir / f"{rf}.root"):
            run_geant(
                root_filename=f"G4_{n_primaries}_{str(energy)}MeV_{l}",
                energy=energy,
                n_primaries=float(n_primaries),
                n_proc=n_proc,
                phantom_type=phantom_type,
                phys_list=l
            )
        else:
            print(f"Skipping simulation: {rf}.root already exists.")

    f1 = get_neutron_distributions(files, lists)
    f2 = get_neutron_multiplicities(files, lists, ep_bins=np.linspace(0, energy, int(energy/2 + 1)))

    f1.savefig(output_dir / f"G4_{n_primaries}_{str(energy)}MeV_neutronDistributions.png")
    f2.savefig(output_dir / f"G4_{n_primaries}_{str(energy)}MeV_neutronMultiplicities.png")

if __name__ == "__main__":
    main()