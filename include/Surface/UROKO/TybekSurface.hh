#ifndef TYBEK_HH
#define TYBEK_HH

#include "G4OpticalSurface.hh"

class TybekSurface {
public:
    TybekSurface();
    ~TybekSurface();

    G4OpticalSurface* GetSurface() const { return fSurface; }

private:
    G4OpticalSurface* fSurface;
};

#endif