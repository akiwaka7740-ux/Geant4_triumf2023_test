#ifndef ACRYLICMAT_HH
#define ACRYLICMAT_HH

#include "G4Material.hh"

class AcrylicMat {
public:
    AcrylicMat();
    ~AcrylicMat();

    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial;
};

#endif