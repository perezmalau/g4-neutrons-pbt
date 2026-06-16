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
/// \file TrackingAction.cc
/// \brief Implementation of the TrackingAction class

#include "TrackingAction.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TrackingAction::TrackingAction() : G4UserTrackingAction()
{}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TrackingAction::~TrackingAction()
= default;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PreUserTrackingAction(const G4Track* track)
{
	auto particle = track->GetParticleDefinition()->GetParticleName();
	auto parentID = track->GetParentID();

	auto analysisManager = G4AnalysisManager::Instance();
	if (particle=="neutron" and parentID == 1) {
		fInitEnergy = track->GetKineticEnergy();
		const G4ThreeVector& creationPos = track->GetPosition();
		fInitPosX = creationPos.x();
		fInitPosY = creationPos.y();
		fInitPosZ = creationPos.z();
		auto momX = track->GetMomentumDirection().x();
		auto momY = track->GetMomentumDirection().y();
		auto momZ = track->GetMomentumDirection().z();

		analysisManager->FillNtupleDColumn(0, 0, fInitPosX);
		analysisManager->FillNtupleDColumn(0, 1, fInitPosY);
		analysisManager->FillNtupleDColumn(0, 2, fInitPosZ);
		analysisManager->FillNtupleDColumn(0, 3, momX);
		analysisManager->FillNtupleDColumn(0, 4, momY);
		analysisManager->FillNtupleDColumn(0, 5, momZ);
		analysisManager->FillNtupleDColumn(0, 6, fInitEnergy);
		analysisManager->AddNtupleRow(0);

		analysisManager->FillH3(0, fInitPosX, fInitPosY, fInitPosZ, 1);
	}
	// else {
	// 	if (parentID > 1) {
	// 		fpTrackingManager->GetTrack()->SetTrackStatus(fStopAndKill);
	// 	}
	// }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track * track)
{}