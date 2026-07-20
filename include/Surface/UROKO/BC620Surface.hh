#ifndef BC620_HH  // ※ハイフンが使えないためアンダースコアに変更
#define BC620_HH

#include "G4OpticalSurface.hh"


class BC620Surface { // ※クラス名もハイフンなしで定義
public:
    BC620Surface();
    ~BC620Surface();

    G4OpticalSurface* GetSurface() const { return fSurface; }

private:
    G4OpticalSurface* fSurface;
};

#endif