#ifndef VACUUMMAT_HH
#define VACUUMMAT_HH

#include "G4Material.hh"

class VacuumMat {
public:
    VacuumMat();
    ~VacuumMat();

    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial;
};

#endif