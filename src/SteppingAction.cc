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
/// \file SteppingAction.cc
/// \brief Implementation of the SteppingAction class

#include "SteppingAction.hh"
#include "TrackingAction.hh"
#include "G4Step.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

SteppingAction::SteppingAction(TrackingAction* trackingAction)
  : G4UserSteppingAction(),
    fTrackingAction(trackingAction) {}


SteppingAction::~SteppingAction() = default;


void SteppingAction::UserSteppingAction(const G4Step* step)
{
    auto analysisManager = G4AnalysisManager::Instance();

    // Record neutron yields from protonInelastic
    auto proc = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
    if (proc == "protonInelastic") {
        G4int nNeutrons = 0;
        const std::vector<const G4Track *> *secondaries = step->GetSecondaryInCurrentStep();
        // How many neutrons were produced from this process?
        for (auto track: *secondaries) {
            G4String particle = track->GetParticleDefinition()->GetParticleName();
            if (particle == "neutron") {
                nNeutrons++;
            }
        }
        // Retrieve the energy of the proton when it underwent inelastic process and number of neutrons produced
        auto ekin = step->GetPreStepPoint()->GetKineticEnergy();
        analysisManager->FillNtupleDColumn(1, 0, ekin);
        analysisManager->FillNtupleIColumn(1, 1, nNeutrons);
        analysisManager->AddNtupleRow(1);
    }

}