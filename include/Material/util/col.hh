#ifndef MY_COL_H
#define MY_COL_H 1

#include "G4Transform3D.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

namespace col {
  
  enum Axis { X, Y, Z };
  G4Transform3D Transform(int axis, double angle, double dx, double dy, double dz) {
    G4RotationMatrix rot;
    if( axis==Axis::X ) rot.rotateX( angle );
    if( axis==Axis::Y ) rot.rotateY( angle );
    if( axis==Axis::Z ) rot.rotateZ( angle );
    return G4Transform3D( rot, G4ThreeVector( dx, dy, dz ) );
  }

  enum class ColID {
    White,
    Gray,
    Grey,
    Black,
    Brown,
    Red,
    Green,
    Blue,
    Cyan,
    Magenta,
    Yellow,
    DarkGrey,
  };

  struct RGB { double R; double G; double B; };
  static std::map<ColID, RGB> defaultRGB = {
    { ColID::White,    { 1.0, 1.0, 1.0 } },
    { ColID::Gray,     { 0.5, 0.5, 0.5 } },
    { ColID::Grey,     { 0.5, 0.5, 0.5 } },
    { ColID::Black,    { 0.0, 0.0, 0.0 } },
    { ColID::Brown,    { 0.45,0.25,0.0 } },
    { ColID::Red,      { 1.0, 0.0, 0.0 } },
    { ColID::Green,    { 0.0, 1.0, 0.0 } },
    { ColID::Blue,     { 0.0, 0.0, 1.0 } },
    { ColID::Cyan,     { 0.0, 1.0, 1.0 } },
    { ColID::Magenta,  { 1.0, 0.0, 1.0 } },
    { ColID::Yellow ,  { 1.0, 1.0, 0.0 } },
    { ColID::DarkGrey, { 0.25,0.25,0.25} },
  };

  G4Colour Color(ColID ID, double Opacity=1) {
    return G4Colour( 
        defaultRGB[ID].R,
        defaultRGB[ID].G,
        defaultRGB[ID].B,
        Opacity );
  }
};


#endif
