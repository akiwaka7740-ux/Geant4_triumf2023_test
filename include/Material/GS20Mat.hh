#ifndef GS20MAT_HH
#define GS20MAT_HH

#include "G4Material.hh"

class GS20Mat {
public:
    GS20Mat();
    ~GS20Mat();

    // どこからでもこのマテリアルを呼び出せるようにする関数
    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial; // 完成したGS20を保持するポインタ
};

#endif