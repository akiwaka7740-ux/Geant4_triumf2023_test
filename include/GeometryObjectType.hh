#ifndef GEOMETRY_OBJECT_TYPE_HH
#define GEOMETRY_OBJECT_TYPE_HH

#include "globals.hh"

enum class GeometryObjectType : G4int {
    Unknown     = -1,
    World       = 0,

    LiGlass     = 1,
    UROKO       = 10,
    HILE        = 20,
    HPGe        = 30,
    BetaPlastic = 40,
    Magnet      = 50,
    Frame       = 60,
    Floor       = 70,
    Shield      = 80,
    Chamber     = 90,
    Stopper     = 100
};

//enum classは暗黙の型変換が許されないため、明示的に型変換する関数があると便利?
constexpr G4int ObjectBaseId(
    GeometryObjectType type
)
{
    return static_cast<G4int>(type);
}

#endif
