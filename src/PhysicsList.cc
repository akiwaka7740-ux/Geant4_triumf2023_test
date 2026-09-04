#include "PhysicsList.hh"

#include "G4RadioactiveDecayPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4OpticalParameters.hh"

PhysicsList::PhysicsList(): FTFP_BERT_HP() //親クラスのコンストラクを呼ぶ
{
    RegisterPhysics(new G4RadioactiveDecayPhysics());
    //RegisterPhysics(new G4DecayPhysics()); <-- これはFTFP_BERT_HPに含まれているので、重複して登録するとエラーになる
    ReplacePhysics(new G4EmLivermorePhysics()); //すでに登録されているEMstandardと交換する
    RegisterPhysics(new G4OpticalPhysics());

    G4OpticalParameters::Instance()->SetScintFiniteRiseTime(true); //Scintillatorの立ち上がり時間を考慮する
}

PhysicsList::~PhysicsList()
{

}