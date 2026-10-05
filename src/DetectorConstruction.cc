#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4VisAttributes.hh"
#include "G4Trd.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "G4Element.hh"
#include "G4SystemOfUnits.hh"

#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// Geometry parameters: edit these to match your bar and photodetector
// ---------------------------------------------------------------------------
namespace
{
  const G4double kBarLength    = 96.0 * mm;  // along x
  const G4double kBarWidth     =   4.3 * mm;  // along y
  const G4double kBarThickness =   4.3 * mm;  // along z (muons cross this)

  const G4double kSensorThickness = 0.5 * mm;

  // Readout: true = light guide + SiPM at both ends,
  //          false = left end only (x < 0); the right end face is then
  //          covered by the same diffuse wrapping as the other bar faces.
  const G4bool   kReadBothEnds    = false;

  // Light guide LG-PYR-V3: truncated square pyramid in K9 glass
  // between each bar end and its SiPM. Set to false to glue the SiPMs
  // directly on the bar ends (previous setup).
  const G4bool   kUseLightGuide   = false;
  const G4double kGuideEntrance   = 4.3 * mm;   // large face, scintillator side (side length)
  const G4double kGuideExit       = 1.0 * mm;   // small face, SiPM side (side length)
  const G4double kGuideLength     = 4.95 * mm;  // along the optical axis

  // Light-guide side walls:
  //   false = bare polished glass (light kept in by total internal reflection)
  //   true  = mirror-coated walls (e.g. aluminium / ESR film glued on)
  const G4bool   kGuideMirrorWalls      = false;
  const G4double kGuideMirrorReflectivity = 0.95;

  // SiPM active area (square side). With the light guide it matches the exit face.
  const G4double kSensorSizeNoGuide = 1.0 * mm;

  const G4double kWrapReflectivity = 0.95;     // Teflon/Tyvek-like diffuse wrapping

  // Plastic scintillator (SP101 by OST optics)
  const G4double kLightYield     = 10000. / MeV;
  const G4double kDecayTime      = 2.4 * ns;
  const G4double kBulkAttLength  = 210. * cm;
  const G4double kRefIndexPlastic = 1.58;
  const G4double kBirksConstant  = 0.126 * mm / MeV;

  // Optical coupling / SiPM window
  const G4double kRefIndexSensor = 1.55;

  // Refractive index of K9 glass (Chinese equivalent of Schott N-BK7)
  // from the N-BK7 Sellmeier equation, wavelength in micrometres.
  G4double RefractiveIndexK9(G4double wavelengthMicrometre)
  {
    const G4double B1 = 1.03961212;
    const G4double B2 = 0.231792344;
    const G4double B3 = 1.01046945;
    const G4double C1 = 0.00600069867;  // um^2
    const G4double C2 = 0.0200179144;   // um^2
    const G4double C3 = 103.560653;     // um^2

    const G4double l2 = wavelengthMicrometre * wavelengthMicrometre;
    const G4double n2 = 1.0 + B1 * l2 / (l2 - C1) + B2 * l2 / (l2 - C2) + B3 * l2 / (l2 - C3);
    return std::sqrt(n2);
  }

  // Bulk absorption length of K9 (~N-BK7) from typical internal transmittance,
  // linearly interpolated in wavelength (nm). Works for any wavelength grid.
  G4double AbsorptionLengthK9(G4double wavelengthNm)
  {
    const std::vector<G4double> lambda = {375., 400., 420., 460., 500., 550.};
    const std::vector<G4double> length = {1.4, 3.3, 3.5, 4.0, 5.0, 5.0};  // metres

    if (wavelengthNm <= lambda.front()) {
      return length.front() * m;
    }
    if (wavelengthNm >= lambda.back()) {
      return length.back() * m;
    }
    for (std::size_t i = 1; i < lambda.size(); ++i) {
      if (wavelengthNm <= lambda[i]) {
        const G4double t = (wavelengthNm - lambda[i - 1]) / (lambda[i] - lambda[i - 1]);
        return (length[i - 1] + t * (length[i] - length[i - 1])) * m;
      }
    }
    return length.back() * m;
  }
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  auto* nist = G4NistManager::Instance();

  // -------------------------------------------------------------------------
  // Optical photon energy grid built from wavelengths (nm), in increasing energy
  // -------------------------------------------------------------------------
  const G4double hc = 1239.84193 * eV * nm;

  // Wavelengths in nm, ordered from long to short so that photon energies increase
  // (Geant4 requires increasing energies in every property vector).
  std::vector<G4double> wavelengths = {520., 510., 500., 490., 480., 470., 460., 455., 450., 445., 440., 435.,
                                       430., 426., 423., 420., 418., 416., 414., 412., 410., 405., 400.};
  // SP101 emission spectrum (relative light output, peak = 1 at 423 nm),
  // digitised from the supplier plot; same order as wavelengths.
  std::vector<G4double> emission = {0.068, 0.085, 0.105, 0.146, 0.185, 0.264, 0.358, 0.473, 0.580, 0.654, 0.702, 0.730,
                                    0.832, 0.947, 1.000, 0.953, 0.804, 0.612, 0.396, 0.270, 0.200, 0.125, 0.072};

  std::vector<G4double> energies;
  for (std::size_t i = 0; i < wavelengths.size(); ++i) {
    energies.push_back(hc / (wavelengths[i] * nm));
  }

  const std::size_t nPoints = energies.size();
  std::vector<G4double> rindexPlastic(nPoints, kRefIndexPlastic);
  std::vector<G4double> absPlastic(nPoints, kBulkAttLength);
  std::vector<G4double> rindexAir(nPoints, 1.0);
  std::vector<G4double> rindexSensor(nPoints, kRefIndexSensor);
  std::vector<G4double> reflectivity(nPoints, kWrapReflectivity);
  std::vector<G4double> guideMirrorReflectivity(nPoints, kGuideMirrorReflectivity);

  // K9 refractive index (Sellmeier) and bulk absorption length.
  std::vector<G4double> rindexK9;
  std::vector<G4double> absK9;
  for (std::size_t i = 0; i < nPoints; ++i) {
    rindexK9.push_back(RefractiveIndexK9(wavelengths[i] / 1000.));
    absK9.push_back(AbsorptionLengthK9(wavelengths[i]));
  }

  // -------------------------------------------------------------------------
  // Materials
  // -------------------------------------------------------------------------
  G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
  auto* airMPT = new G4MaterialPropertiesTable();
  airMPT->AddProperty("RINDEX", energies, rindexAir);
  air->SetMaterialPropertiesTable(airMPT);

  G4Material* plastic = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
  auto* plasticMPT = new G4MaterialPropertiesTable();
  plasticMPT->AddProperty("RINDEX", energies, rindexPlastic);
  plasticMPT->AddProperty("ABSLENGTH", energies, absPlastic);
  plasticMPT->AddProperty("SCINTILLATIONCOMPONENT1", energies, emission);
  plasticMPT->AddConstProperty("SCINTILLATIONYIELD", kLightYield);
  plasticMPT->AddConstProperty("RESOLUTIONSCALE", 1.0);
  plasticMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", kDecayTime);
  plasticMPT->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
  plastic->SetMaterialPropertiesTable(plasticMPT);
  plastic->GetIonisation()->SetBirksConstant(kBirksConstant);

  // K9 glass (~N-BK7) defined by oxide mass fractions, density 2.51 g/cm3.
  // Typical BK7 composition: SiO2 69.1 %, B2O3 10.8 %, Na2O 10.4 %, K2O 6.3 %, BaO 3.1 %
  // (the 0.4 % As2O3 fining agent is neglected and the rest renormalised).
  G4Element* elBa = nist->FindOrBuildElement("Ba");
  G4Element* elO  = nist->FindOrBuildElement("O");
  auto* bariumOxide = new G4Material("BariumOxide", 5.72 * g / cm3, 2);
  bariumOxide->AddElement(elBa, 1);
  bariumOxide->AddElement(elO, 1);

  auto* glassK9 = new G4Material("K9_Glass", 2.51 * g / cm3, 5);
  glassK9->AddMaterial(nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE"), 0.6938);
  glassK9->AddMaterial(nist->FindOrBuildMaterial("G4_BORON_OXIDE"),     0.1079);
  glassK9->AddMaterial(nist->FindOrBuildMaterial("G4_SODIUM_MONOXIDE"), 0.1044);
  glassK9->AddMaterial(nist->FindOrBuildMaterial("G4_POTASSIUM_OXIDE"), 0.0631);
  glassK9->AddMaterial(bariumOxide,                                     0.0308);

  auto* k9MPT = new G4MaterialPropertiesTable();
  k9MPT->AddProperty("RINDEX", energies, rindexK9);
  k9MPT->AddProperty("ABSLENGTH", energies, absK9);
  glassK9->SetMaterialPropertiesTable(k9MPT);

  // SiPM entrance window (epoxy/silicone). Photons entering it are counted.
  G4Material* sensorWindow = nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE");
  auto* sensorMPT = new G4MaterialPropertiesTable();
  sensorMPT->AddProperty("RINDEX", energies, rindexSensor);
  sensorWindow->SetMaterialPropertiesTable(sensorMPT);

  // -------------------------------------------------------------------------
  // World
  // -------------------------------------------------------------------------
  const G4double worldHalfX = 0.5 * kBarLength + 10. * cm;
  const G4double worldHalfY = 0.5 * kBarWidth + 10. * cm;
  const G4double worldHalfZ = 0.5 * kBarThickness + 20. * cm;

  auto* worldSolid = new G4Box("World", worldHalfX, worldHalfY, worldHalfZ);
  auto* worldLogical = new G4LogicalVolume(worldSolid, air, "World");
  auto* worldPhysical = new G4PVPlacement(nullptr, G4ThreeVector(), worldLogical,
                                          "World", nullptr, false, 0, true);

  // -------------------------------------------------------------------------
  // Scintillator bar
  // -------------------------------------------------------------------------
  auto* barSolid = new G4Box("Bar", 0.5 * kBarLength, 0.5 * kBarWidth, 0.5 * kBarThickness);
  auto* barLogical = new G4LogicalVolume(barSolid, plastic, "Bar");
  auto* barPhysical = new G4PVPlacement(nullptr, G4ThreeVector(), barLogical,
                                        "Bar", worldLogical, false, 0, true);

  // -------------------------------------------------------------------------
  // Light guides (optional) and SiPMs: left end always, right end only if
  // kReadBothEnds. Copy 0 = left (x < 0), copy 1 = right (x > 0).
  //
  // G4Trd is a box whose x and y half-widths change linearly along its local z:
  //   at z = -dz : half-widths (dx1, dy1)  -> entrance face, 4.3 x 4.3 mm
  //   at z = +dz : half-widths (dx2, dy2)  -> exit face,     1.0 x 1.0 mm
  // Its local z axis is the optical axis, so it is rotated by +-90 deg about y
  // to lie along the bar (global x), with the large face touching the bar.
  // -------------------------------------------------------------------------
  G4double sensorSize = kSensorSizeNoGuide;
  G4double sensorX = 0.5 * kBarLength + 0.5 * kSensorThickness;

  G4LogicalVolume* guideLogical = nullptr;
  G4VPhysicalVolume* guidePhysical[2] = {nullptr, nullptr};

  if (kUseLightGuide) {
    sensorSize = kGuideExit;
    sensorX = 0.5 * kBarLength + kGuideLength + 0.5 * kSensorThickness;

    auto* guideSolid = new G4Trd("LightGuide",
                                 0.5 * kGuideEntrance, 0.5 * kGuideExit,   // dx1, dx2
                                 0.5 * kGuideEntrance, 0.5 * kGuideExit,   // dy1, dy2
                                 0.5 * kGuideLength);                      // dz
    guideLogical = new G4LogicalVolume(guideSolid, glassK9, "LightGuide");

    const G4double guideX = 0.5 * kBarLength + 0.5 * kGuideLength;

    // Left guide: local +z (exit) must point to global -x
    G4RotationMatrix rotLeft;
    rotLeft.rotateY(-90. * deg);
    guidePhysical[0] = new G4PVPlacement(G4Transform3D(rotLeft, G4ThreeVector(-guideX, 0., 0.)),
                                         guideLogical, "LightGuide", worldLogical, false, 0, true);

    // Right guide: local +z (exit) must point to global +x
    if (kReadBothEnds) {
      G4RotationMatrix rotRight;
      rotRight.rotateY(+90. * deg);
      guidePhysical[1] = new G4PVPlacement(G4Transform3D(rotRight, G4ThreeVector(+guideX, 0., 0.)),
                                           guideLogical, "LightGuide", worldLogical, false, 1, true);
    }
  }

  auto* sensorSolid = new G4Box("Sensor", 0.5 * kSensorThickness, 0.5 * sensorSize, 0.5 * sensorSize);
  auto* sensorLogical = new G4LogicalVolume(sensorSolid, sensorWindow, "Sensor");

  new G4PVPlacement(nullptr, G4ThreeVector(-sensorX, 0., 0.), sensorLogical,
                    "Sensor", worldLogical, false, 0, true);
  if (kReadBothEnds) {
    new G4PVPlacement(nullptr, G4ThreeVector(+sensorX, 0., 0.), sensorLogical,
                      "Sensor", worldLogical, false, 1, true);
  }

  // -------------------------------------------------------------------------
  // Wrapping: diffuse reflector on every bar face touching air.
  // Bar -> sensor boundary has no surface: Fresnel transmission (optical grease).
  // -------------------------------------------------------------------------
  auto* wrapSurface = new G4OpticalSurface("Wrapping");
  wrapSurface->SetType(dielectric_dielectric);
  wrapSurface->SetModel(unified);
  wrapSurface->SetFinish(groundfrontpainted);

  auto* wrapMPT = new G4MaterialPropertiesTable();
  wrapMPT->AddProperty("REFLECTIVITY", energies, reflectivity);
  wrapSurface->SetMaterialPropertiesTable(wrapMPT);

  new G4LogicalBorderSurface("BarWrapping", barPhysical, worldPhysical, wrapSurface);

  // Light-guide side walls (guide -> air). The bar -> guide and guide -> SiPM
  // interfaces get no surface: plain Fresnel refraction, i.e. optical grease/glue.
  if (kUseLightGuide) {
    auto* guideSurface = new G4OpticalSurface("GuideWalls");
    guideSurface->SetModel(unified);

    if (kGuideMirrorWalls) {
      guideSurface->SetType(dielectric_dielectric);
      guideSurface->SetFinish(polishedfrontpainted);
      auto* guideMPT = new G4MaterialPropertiesTable();
      guideMPT->AddProperty("REFLECTIVITY", energies, guideMirrorReflectivity);
      guideSurface->SetMaterialPropertiesTable(guideMPT);
    }
    else {
      guideSurface->SetType(dielectric_dielectric);
      guideSurface->SetFinish(polished);
    }

    new G4LogicalBorderSurface("GuideWallsLeft", guidePhysical[0], worldPhysical, guideSurface);
    if (kReadBothEnds) {
      new G4LogicalBorderSurface("GuideWallsRight", guidePhysical[1], worldPhysical, guideSurface);
    }
  }

  // -------------------------------------------------------------------------
  // Visualization
  // -------------------------------------------------------------------------
  worldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

  auto* barVis = new G4VisAttributes(G4Colour(0.3, 0.6, 1.0, 0.4));
  barVis->SetForceSolid(true);
  barLogical->SetVisAttributes(barVis);

  auto* sensorVis = new G4VisAttributes(G4Colour(1.0, 0.2, 0.2));
  sensorVis->SetForceSolid(true);
  sensorLogical->SetVisAttributes(sensorVis);

  if (kUseLightGuide) {
    auto* guideVis = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.5));
    guideVis->SetForceSolid(true);
    guideLogical->SetVisAttributes(guideVis);
  }

  return worldPhysical;
}
