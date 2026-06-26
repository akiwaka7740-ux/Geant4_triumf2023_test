#ifndef PMTGLASSMAT_HH
#define PMTGLASSMAT_HH

#include "G4Material.hh"

class PMTGlassMat {
public:
    PMTGlassMat();
    ~PMTGlassMat();

    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial;
};

#endif