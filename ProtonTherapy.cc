//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************


#include "DetectorConstruction.hh"
#include "G4PhysListFactory.hh"
#include "G4VModularPhysicsList.hh"
#include "ActionInitialization.hh"
#include "G4ScoringManager.hh"
#include "G4RunManagerFactory.hh"

#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4StepLimiterPhysics.hh"

// Custom physics list components
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4DecayPhysics.hh"
#include "G4IonBinaryCascadePhysics.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4StoppingPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

namespace {
    void PrintUsage() {
        G4cerr << " Usage: " << G4endl;
        G4cerr << " example [-m macro ] [-u UIsession] [-t nThreads]" << G4endl;
        G4cerr << "   note: -t option is available only for multi-threaded mode."
               << G4endl;
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc, char **argv) {
    // Evaluate arguments
    //
    if (argc > 9) {
        PrintUsage();
        return 1;
    }

    G4String macro;
    G4String session;
    G4String physListName = "QGSP_BIC_HP";
#ifdef G4MULTITHREADED
    G4int nThreads = 0;
#endif
    for (G4int i = 1; i < argc; i = i + 2) {
        if (G4String(argv[i]) == "-m") macro = argv[i + 1];
        else if (G4String(argv[i]) == "-u") session = argv[i + 1];
        else if (G4String(argv[i]) == "-p") physListName = argv[i + 1]; // add the physics list name
#ifdef G4MULTITHREADED
        else if (G4String(argv[i]) == "-t") {
            nThreads = G4UIcommand::ConvertToInt(argv[i + 1]);
        }
#endif
        else {
            PrintUsage();
            return 1;
        }
    }

    // Detect interactive mode (if no macro provided) and define UI session
    //
    G4UIExecutive* ui = nullptr;
    if (!macro.size()) {
        ui = new G4UIExecutive(argc, argv, session);
    }

    // Choose the Random engine
    //
    CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine());
    auto seed = (G4int) time(nullptr);
    // Possibility to set an offset to random number via environment variable "JOB_ID"
    char *jobID = getenv("JOB_ID");
    if (jobID != nullptr)
        seed += atoi(jobID) * 4852234 * G4UniformRand();
    G4cout << "Seed:" << seed << G4endl;
    G4Random::setTheSeed(seed);

    // Construct the default run manager
    //
    auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
#ifdef G4MULTITHREADED
    if (nThreads > 0) {
        runManager->SetNumberOfThreads(nThreads);
    }
#endif


    // Activate UI-command base scorer
    G4ScoringManager * scManager = G4ScoringManager::GetScoringManager();
    scManager->SetVerboseLevel(1);

    // Set mandatory initialization classes
    //
    auto *detConstruction = new DetectorConstruction();
    runManager->SetUserInitialization(detConstruction);

    // Physics list: use "-p custom" for the hand-built list, or any
    // reference name (e.g. QGSP_BIC_HP) for the factory.
    G4VModularPhysicsList* physicsList = nullptr;
    if (physListName == "custom") {
        physicsList = new G4VModularPhysicsList();
        physicsList->RegisterPhysics(new G4EmStandardPhysics_option4());
        physicsList->RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
        physicsList->RegisterPhysics(new G4DecayPhysics());
        physicsList->RegisterPhysics(new G4IonBinaryCascadePhysics());
        physicsList->RegisterPhysics(new G4HadronElasticPhysicsHP());
        physicsList->RegisterPhysics(new G4StoppingPhysics());
        physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics());
    } else {
        auto* physicsFactory = new G4PhysListFactory();
        physicsList = physicsFactory->GetReferencePhysList(physListName);
    }
    physicsList->SetVerboseLevel(0);
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    runManager->SetUserInitialization(physicsList);

    auto *actionInitialization = new ActionInitialization();
    runManager->SetUserInitialization(actionInitialization);


    // Initialize visualization
    //
    // auto visManager = new G4VisExecutive;
    // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
    auto visManager = new G4VisExecutive("Quiet");
    visManager->Initialize();

    // Get the pointer to the User Interface manager
    auto UImanager = G4UImanager::GetUIpointer();


    // Process macro or start UI session
    //
    if (macro.size()) {
        // batch mode
        G4String command = "/control/execute ";
        UImanager->ApplyCommand(command + macro);
    }
    else {
        // interactive mode : define UI session
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        if (ui->IsGUI()) {
            UImanager->ApplyCommand("/control/execute gui.mac");
        }
        ui->SessionStart();
        delete ui;
    }

    // Job termination
    // Free the store: user actions, physics_list and detector_description are
    // owned and deleted by the run manager, so they should not be deleted
    // in the main() program !

#ifdef G4VIS_USE
    delete visManager;
#endif
    delete runManager;
}
