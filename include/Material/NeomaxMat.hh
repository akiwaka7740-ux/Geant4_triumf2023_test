#ifndef NeomaxMat_h
#define NeomaxMat_h 1

#include "G4Material.hh"

class NeomaxMat {
  public:
    NeomaxMat();
    ~NeomaxMat();

    // 作成したマテリアルを取得する関数
    G4Material* GetMaterial() { return fMaterial; }

  private:
    G4Material* fMaterial;
};

#endif