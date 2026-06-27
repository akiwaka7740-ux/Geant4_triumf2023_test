#ifndef CATHODESURFACE_HH
#define CATHODESURFACE_HH

#include "G4OpticalSurface.hh"

class CathodeSurface {
public:
    CathodeSurface();
    ~CathodeSurface();
    G4OpticalSurface* GetSurface() const { return fSurface; }

private:
    G4OpticalSurface* fSurface;
};

#endif