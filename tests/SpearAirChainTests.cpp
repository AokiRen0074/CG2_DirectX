#include "Application/Character/SpearAdventure.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
void Check(bool ok,const char* message) {if(!ok) throw std::runtime_error(message);}
SpearAdventure Arena() {
 SpearAdventure a;a.StartBossPractice();a.bossState=SpearAdventure::BossState::Recovery;a.bossTimer=1000;
 a.bossX=a.bossTargetX=124;a.bossY=a.bossTargetY=-.4f;a.invincibility=10000;
 a.player.x=103;a.player.y=-.4f;a.player.grounded=false;a.player.charges=1;
 for(int i=0;i<3;++i) a.minions[size_t(i)]={107.5f+float(i)*6,-.4f,0,0,420,-1};
 return a;
}
int main() {try {
 auto a=Arena();
 for(int dash=0;dash<4;++dash) {
  Check(a.player.charges>0,"A kill must fund the next dash");
  for(int f=0;f<11;++f) a.Tick(1,0,f==0,true);
  Check(!a.player.grounded && a.hp==6,"Route must stay airborne without damage");
 }
 Check(a.chain==3 && a.maximumChain==3 && a.airStrikeSerial==1,"Three kills must lead into a powered boss strike");
 Check(a.bossHp==64 && a.airPower==0,"Recovery strike must deal 2+6 and consume stored power");
 const int chargedHp=a.bossHp;
 a.player.x=121;a.player.dashFrames=0;a.Tick(1,0,true,true);
 Check(chargedHp-a.bossHp==2,"No repeat bonus without new kills");
 // The actual generated recovery route, not just a hand placed test arrangement.
 for(bool second : {false,true}) {
  a=Arena();a.secondPhase=second;a.minions={};a.player.x=103;a.player.y=-.8f;
  a.bossX=a.bossTargetX=120;a.bossY=a.bossTargetY=4;
  a.EnterBossRecovery(1000);
  for(int dash=0;dash<7 && a.airStrikeSerial==0;++dash) {
   const SpearAdventure::Minion* next=nullptr;float distance=100000;
   for(const auto& node:a.minions)if(node.life>0) {
    float d=(node.x-a.player.x)*(node.x-a.player.x)+(node.y-a.player.y)*(node.y-a.player.y);
    if(d<distance){distance=d;next=&node;}
   }
   float dx=(next?next->x:a.bossX)-a.player.x,dy=(next?next->y:a.bossY)-a.player.y;
   Check(a.player.charges>0,"Generated route must sustain the approach");
   for(int f=0;f<11;++f)a.Tick(dx,dy,f==0,true);
   for(int f=0;f<4;++f)a.Tick(0,0,false,true);
  }
  Check(a.airStrikeSerial==1 && a.maximumChain>=2 && !a.player.grounded,"Generated boss route must lead into an airborne powered hit");
 }
 // An actual kill creates a finite aiming window; no input never triggers a dash.
 a=Arena();for(int f=0;f<11;++f)a.Tick(1,0,f==0,true);
 const float y=a.player.y;const int id=a.player.attackId;
 for(int f=0;f<10;++f)a.Tick(0,0,false,true);
 Check(a.player.y>=y && a.player.attackId==id && a.player.charges==1,"Refill must buy aiming time, not an automatic attack");
 a.Tick(0,1,true,true);Check(a.player.attackId==id+1 && a.player.refillFloatFrames==0,"Input must immediately cancel the aiming window");
 a=Arena();for(int f=0;f<22;++f)a.Tick(1,0,f%11==0,true);
 Check(a.airPower==2,"Two kills prime a finisher");
 a.invincibility=0;a.Hurt();Check(a.chain==0 && a.airPower==0,"Damage breaks the aerial chain");
 a=Arena();for(int f=0;f<22;++f)a.Tick(1,0,f%11==0,true);
 a.player.x=115;a.player.y=-4;a.ResolveSolidPlatforms(115,1,false);
 Check(a.player.grounded && a.chain==0 && a.airPower==0 && a.player.charges==3,"Landing resets the chain but restores ordinary charges");
 // Do not teleport live route targets when a boss attack ends.
 a=Arena();const auto live=a.minions[1];a.minions[0].life=0;a.EnterBossRecovery(100);
 Check(a.minions[0].life==420 && a.minions[1].x==live.x,"Recovery replenishes only empty route slots");
 // The authored gap works from a normal grounded jump with pauses to aim.
 std::ifstream file("Resources/Stages/tutorial-stage.json");nlohmann::json j;file>>j;
 a.RestartStage(SpearStage::Parse(j));for(auto& e:a.enemies)if(e.x<31)e.alive=false;
 a.player.x=30;a.player.y=-3;a.player.grounded=true;a.checkpoint=2;
 for(int f=0;f<18;++f)a.Tick(0,0,f==0,true);
 for(int dash=0;dash<7;++dash) {
  Check(a.player.charges>0,"Authored gap must fund every dash");
  for(int f=0;f<11;++f)a.Tick(1,0,f==0,true);
  if(dash<5)for(int f=0;f<4;++f)a.Tick(0,0,false,true);
 }
 Check(a.maximumChain>=5 && a.player.x>69.75f && a.deaths==0,"Normal jump must reach five kills and the far bank");
 for(int f=0;f<80;++f)a.Tick(0,0,false,true);
 Check(a.player.grounded && a.chain==0,"Chain finishes on the far platform");
 a.enemies[3].alive=false;a.Respawn();Check(!a.enemies[3].alive && a.player.x==72,"Completed crossing must retain the far-bank checkpoint");
 std::cout<<"PASS: airborne refill loop, aiming grace, damage/landing breaks, powered boss cash-in, no bonus reuse, recovery routes and authored five-kill crossing.\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
