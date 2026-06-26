#ifndef AIRMAT_HH
#define AIRMAT_HH

#include "G4Material.hh"

class AirMat {
public:
    AirMat();
    ~AirMat();

    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial;
};

#endif