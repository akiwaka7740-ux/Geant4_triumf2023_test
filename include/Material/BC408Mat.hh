#ifndef BC408MAT_HH
#define BC408MAT_HH

#include "G4Material.hh"

class BC408Mat {
public:
    BC408Mat();
    ~BC408Mat();

    // 完成したBC408マテリアルを返す関数
    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial;
};

#endif