#ifndef MGOMAT_HH
#define MGOMAT_HH

#include "G4Material.hh"

class MgOMat {
public:
    MgOMat();
    ~MgOMat();

    // どこからでもこのマテリアルを呼び出せるようにする関数
    G4Material* GetMaterial() const { return fMaterial; }

private:
    G4Material* fMaterial; // 完成したMgOを保持するポインタ
};

#endif