#ifndef DIELECTRICSURFACE_HH
#define DIELECTRICSURFACE_HH

#include "G4OpticalSurface.hh"

class DielectricSurface {
public:
    DielectricSurface();
    ~DielectricSurface();
    G4OpticalSurface* GetSurface() const { return fSurface; }

private:
    G4OpticalSurface* fSurface;
};

#endif