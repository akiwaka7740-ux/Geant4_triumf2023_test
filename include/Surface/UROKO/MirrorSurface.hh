#ifndef MirrorSURFACE1_HH
#define MirrorSURFACE1_HH

#include "G4OpticalSurface.hh"

class MirrorSurface{
public:
    MirrorSurface();
    ~MirrorSurface();
    G4OpticalSurface* GetSurface() const { return fSurface; }

private:
    G4OpticalSurface* fSurface;
};

#endif