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
//
//
/// \file src/RunAction.cc
/// \brief Implementation of the RunAction class

#include "RunAction.hh"
#include "G4Timer.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4ProcessManager.hh"
#include "G4Proton.hh"
#include "G4DynamicParticle.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4HadronicProcessStore.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::RunAction()
    : G4UserRunAction(),
      fRootOutputFileName("SimOutput.root") {
    fMessenger = new G4GenericMessenger(this, "/output/", "Output control");
    fMessenger->DeclareProperty("rootFileName", fRootOutputFileName, "Set root output file name");

    timer = new G4Timer();
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetFileName(fRootOutputFileName);
    analysisManager->SetH1Activation(true);
    analysisManager->SetH3Activation(true);
    analysisManager->SetNtupleMerging(true);

    analysisManager->CreateH3("NeutronCount", "Neutron creation positions", 100, -100, 100,
                              100, -100, 100, 100, -100, 100,
                              "mm", "mm", "mm");


    // If you want to record the proton inelastic cross section in Oxygen, uncomment this
    // analysisManager->CreateH1("protonInelasticXS_O", "Proton inelastic cross section in O", 200, 0, 200 / MeV);

    // Secondary neutron characterisation, filled in TrackingAction using primary beam
    analysisManager->CreateNtuple("NeutronInfo", "Neutron information per-event");
    analysisManager->CreateNtupleDColumn("OriginPosX");
    analysisManager->CreateNtupleDColumn("OriginPosY");
    analysisManager->CreateNtupleDColumn("OriginPosZ");
    analysisManager->CreateNtupleDColumn("MomentumX");
    analysisManager->CreateNtupleDColumn("MomentumY");
    analysisManager->CreateNtupleDColumn("MomentumZ");
    analysisManager->CreateNtupleDColumn("eKin");
    analysisManager->FinishNtuple();

    // Number of neutrons per inelastic interaction, filled in SteppingAction
    analysisManager->CreateNtuple("protonInelasticData", "Proton inelastic data");
    analysisManager->CreateNtupleDColumn("protonEkin");
    analysisManager->CreateNtupleIColumn("NeutronMultiplicity");
    analysisManager->FinishNtuple();
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::~RunAction() {
    delete G4AnalysisManager::Instance();
    delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run * /*run*/) {
    if (isMaster)
        timer->Start();
    // Get analysis manager
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->OpenFile(fRootOutputFileName);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::EndOfRunAction(const G4Run *run) {
    auto analysisManager = G4AnalysisManager::Instance();
    if (isMaster) {
        timer->Stop();
        G4cout << "Simulation time: " << timer->GetRealElapsed() << " seconds." << G4endl;

        // If you want to record the proton inelastic cross section in Oxygen, uncomment this:
        // G4HadronicProcessStore *store = G4HadronicProcessStore::Instance();
        // const G4Element* elm = G4NistManager::Instance()->FindOrBuildElement("O");
        // const G4Material* mat = G4NistManager::Instance()->FindOrBuildMaterial("G4_O");
        //
        // const G4int nBins = 200;
        // const G4double Emin = 0.0 * MeV;
        // const G4double Emax = 200.0 * MeV;
        // const G4double dE = (Emax - Emin) / nBins;
        // const G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("proton");
        // for (G4int i = 0; i < nBins; ++i) {
        //     G4double Ekin = Emin + (i + 0.5) * dE;
        //     G4double xs = store->GetInelasticCrossSectionPerAtom(particle, Ekin, elm, mat);
        //     analysisManager->FillH1(2, Ekin / MeV, xs/barn);
        // }
    }

    analysisManager->Write();
    analysisManager->CloseFile();
}