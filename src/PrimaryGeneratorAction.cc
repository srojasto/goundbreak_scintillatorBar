#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  fParticleGun = new G4ParticleGun(1);

  // Defaults: 4 GeV mu- shot vertically down through the bar centre.
  // All of these can be overridden in a macro with /gun/... commands.
  auto* muon = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
  fParticleGun->SetParticleDefinition(muon);
  fParticleGun->SetParticleEnergy(4. * GeV);
  fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 15. * cm));
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., -1.));
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  fParticleGun->GeneratePrimaryVertex(event);
}
