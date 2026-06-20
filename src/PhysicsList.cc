#include "PhysicsList.hh"

#include "G4RadioactiveDecayPhysics.hh"
#include "G4DecayPhysics.hh"

PhysicsList::PhysicsList(): FTFP_BERT_HP() //親クラスのコンストラクを呼ぶ
{
    RegisterPhysics(new G4RadioactiveDecayPhysics());
    //RegisterPhysics(new G4DecayPhysics()); <-- これはFTFP_BERT_HPに含まれているので、重複して登録するとエラーになる
}

PhysicsList::~PhysicsList()
{

}