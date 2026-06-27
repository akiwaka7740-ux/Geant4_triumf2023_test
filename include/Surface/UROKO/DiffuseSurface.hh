#ifndef DIFFUSESURFACE_HH
#define DIFFUSESURFACE_HH

#include "G4OpticalSurface.hh"

class DiffuseSurface {
public:
    DiffuseSurface();
    ~DiffuseSurface();
    G4OpticalSurface* GetSurface() const { return fSurface; }

private:
    G4OpticalSurface* fSurface;
};

#endif