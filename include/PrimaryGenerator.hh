#ifndef PRIMARYGENERATOR_HH
#define PRIMARYGENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4RandomDirection.hh"
#include "G4IonTable.hh"

class G4ParticleGun;
class PrimaryGeneratorMessenger; // 前方宣言

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
    public:
     PrimaryGenerator();
     ~PrimaryGenerator();

     virtual void GeneratePrimaries(G4Event *);

     void SetSourceType(G4String type) { fSourceType = type; } // ソースタイプを設定するメソッド

    private:
        G4ParticleGun *fParticleGun;
        PrimaryGeneratorMessenger *fMessenger; // メッセンジャーのポインタ
        G4String fSourceType; // ソースタイプを保持するメンバ変数
};


#endif