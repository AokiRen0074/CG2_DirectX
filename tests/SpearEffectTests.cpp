#include "Application/Character/SpearEffects.h"
#include "Application/Character/SpearEditorSettings.h"
#include <cassert>
#include <iostream>
#include <limits>
int main() {
    SpearEditorSettings saved;
    saved.effects.trailFrames=73; saved.effects.trailWidth=2.1f;
    saved.effects.blue={.2f,.7f,.1f}; saved.effects.hitSparkCount=17;
    auto json=saved.Serialize();
    const auto loaded=SpearEditorSettings::Parse(json);
    assert(loaded.effects.trailFrames==73 && loaded.effects.blue==saved.effects.blue);
    assert(loaded.effects.hitSparkCount==17 && loaded.effects.trailWidth==2.1f);
    json.erase("effects"); // Existing saves remain compatible.
    assert(SpearEditorSettings::Parse(json).effects.trailFrames==16);
    for (int bad: {0, -1, 13}) {
        json=saved.Serialize(); json["effects"]["dashSparkInterval"]=bad;
        bool rejected=false;
        try { (void)SpearEditorSettings::Parse(json); } catch (const std::exception&) { rejected=true; }
        assert(rejected);
    }
    auto bad=saved.effects; bad.sparkSpeed=std::numeric_limits<float>::quiet_NaN();
    bool rejected=false;
    try { bad.Validate(); } catch (const std::exception&) { rejected=true; }
    assert(rejected);
    SpearEffects fx;
    fx.settings.sparkLife=2; fx.settings.sparkSpeed=3;
    fx.SpawnSpark(0,0,.1f,.2f,20);
    assert(fx.sparks[0].life==40 && fx.sparks[0].duration==40);
    fx.Tick(); assert(fx.sparks[0].x>.29f && fx.sparks[0].y>.59f);
    fx={}; fx.settings.trailFrames=60; fx.settings.dashSparkInterval=1;
    SpearMotion motion; motion.attackId=2;
    fx.BeginDash(motion,{0,0}); motion.x=1; fx.Record(motion);
    assert(fx.trails[0].fade==60 && fx.trails[0].duration==60 && fx.sparkCursor==1);
    for(int i=0;i<60;++i) fx.Tick();
    assert(fx.trails[0].fade==0);
    fx={}; fx.settings.hitSparkCount=24; fx.Hit(0,0,0);
    assert(fx.sparkCursor==24);
    fx={}; fx.settings.hitSparkCount=0; fx.Hit(0,0,0);
    assert(fx.sparkCursor==0);
    saved.effects.ambientEnabled=false;saved.effects.ambientDensity=1.6f;
    saved.effects.ambientBrightness=.7f;saved.effects.ambientDrift=1.2f;
    auto atmosphere=SpearEditorSettings::Parse(saved.Serialize());
    assert(!atmosphere.effects.ambientEnabled && atmosphere.effects.ambientDensity==1.6f);
    auto legacyEffects=saved.Serialize();
    for(const auto *field:{"ambientEnabled","ambientDensity","ambientBrightness","ambientDrift"}) legacyEffects["effects"].erase(field);
    atmosphere=SpearEditorSettings::Parse(legacyEffects);
    assert(atmosphere.effects.ambientEnabled && atmosphere.effects.ambientDensity==1 && atmosphere.effects.ambientBrightness==1);
    bad=saved.effects;bad.ambientDensity=3;rejected=false;
    try {bad.Validate();} catch(const std::exception&) {rejected=true;}
    assert(rejected);
    std::cout << "Effect settings and lifetime tests passed\n";
}
