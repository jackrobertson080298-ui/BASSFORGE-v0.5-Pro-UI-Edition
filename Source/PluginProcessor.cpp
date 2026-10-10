#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
const std::array<Patch,30> factoryPatches = {{
 {"01 Reactor",.65f,.28f,.17f,.70f,.25f,.27f,.72f,.65f,0},
 {"02 Iron Foghorn",.80f,.38f,.06f,.43f,.18f,.58f,.34f,.23f,1},
 {"03 Voltage Reese",.56f,.14f,.46f,.56f,.37f,.60f,.52f,.94f,0},
 {"04 FM Punisher",.88f,.83f,.10f,.78f,.20f,.13f,.83f,.35f,2},
 {"05 Skyline",.43f,.17f,.29f,.84f,.11f,.47f,.92f,.83f,0},
 {"06 Neuro Pulse",.78f,.64f,.16f,.62f,.91f,.30f,.73f,.52f,2},
 {"07 Sub Destroyer",.91f,.18f,.08f,.35f,.17f,.44f,.22f,.16f,1},
 {"08 Razor Edge",.85f,.53f,.11f,.91f,.48f,.18f,.99f,.41f,2},
 {"09 Dark Matter",.73f,.48f,.34f,.39f,.62f,.82f,.42f,.80f,1},
 {"10 Drop Hammer",.95f,.39f,.04f,.65f,.13f,.09f,.68f,.20f,2},
 {"11 Titan Horn",.72f,.32f,.12f,.52f,.23f,.64f,.47f,.38f,1},
 {"12 Saw Riot",.79f,.12f,.31f,.93f,.34f,.31f,.96f,.79f,0},
 {"13 Metallic Bite",.84f,.92f,.05f,.82f,.44f,.16f,.89f,.29f,2},
 {"14 Dirty Donk",.90f,.73f,.03f,.68f,.10f,.12f,.65f,.14f,1},
 {"15 Wide Collision",.61f,.27f,.49f,.77f,.39f,.53f,.78f,1.f,0},
 {"16 Ghost Reese",.48f,.22f,.38f,.36f,.58f,.73f,.29f,.89f,0},
 {"17 Shredder",.98f,.71f,.15f,.96f,.69f,.22f,.95f,.59f,2},
 {"18 Rumble",.74f,.11f,.22f,.25f,.32f,.69f,.16f,.45f,1},
 {"19 Ascension",.55f,.33f,.25f,.88f,.75f,.46f,.86f,.88f,0},
 {"20 Final Impact",.99f,.59f,.08f,.73f,.26f,.11f,.77f,.30f,2},
 {"21 Punisher XL",.95f,.88f,.18f,.87f,.32f,.24f,.91f,.68f,2,.54f,.60f},
 {"22 Punisher Stab",.93f,.91f,.05f,.91f,.12f,.06f,.95f,.22f,2,.96f,.48f},
 {"23 Punisher Reese",.81f,.79f,.39f,.59f,.59f,.53f,.68f,.90f,2,.27f,.61f},
 {"24 Punisher Scream",.98f,.99f,.08f,.99f,.77f,.20f,1.f,.55f,2,.64f,.98f},
 {"25 Punisher Foghorn",.92f,.70f,.14f,.53f,.28f,.79f,.52f,.50f,1,.37f,.58f},
 {"26 Punisher Laser",.91f,.95f,.02f,.97f,.85f,.08f,.98f,.28f,2,.90f,.83f},
 {"27 Punisher Wobble",.86f,.84f,.28f,.66f,.98f,.36f,.76f,.71f,2,.36f,.73f},
 {"28 Punisher Crusher",1.f,.90f,.06f,.72f,.38f,.14f,.87f,.34f,2,.87f,.92f},
 {"29 Punisher Shimmer",.76f,.71f,.31f,.93f,.48f,.55f,.99f,.97f,0,.29f,.65f},
 {"30 Punisher Final Boss",1.f,1.f,.15f,.84f,.69f,.31f,.94f,.80f,2,1.f,1.f}
}};
const char* BassforgeProcessor::fxNames[BassforgeProcessor::fxCount]={"DISTORTION","FILTER","DYNAMICS","EQ","CHORUS","DELAY","REVERB"};
juce::AudioProcessorValueTreeState::ParameterLayout BassforgeProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 for(auto id:{"drive","fm","detune","cutoff","movement","release","tone","width","sub","output","punch","rasp"})
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},juce::String(id).toUpperCase(),0.f,1.f,(juce::String(id)=="sub"?.65f:juce::String(id)=="output"?.65f:.5f)));
 for(int i=0;i<fxCount;++i){
  auto prefix="fx"+juce::String(i);
  p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{prefix+"on",1},prefix+" ENABLED",i<4));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{prefix+"mix",1},prefix+" MIX",0.f,1.f,i<4?0.65f:0.20f));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{prefix+"amt",1},prefix+" AMOUNT",0.f,1.f,0.5f));
 }
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"fxbypass",1},"FX BYPASS",false));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"shape",1},"SHAPE",juce::StringArray{"Saw","Horn","Metal"},0));
 return p;
}
BassforgeProcessor::BassforgeProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),params(*this,nullptr,"PARAMS",layout()){for(int i=0;i<fxCount;++i)fxOrder[(size_t)i].store(i);setCurrentProgram(0);}
void BassforgeProcessor::prepareToPlay(double sr,int){
 sampleRateHz=sr;for(auto& v:voices)v=Voice{};
 for(int ch=0;ch<2;++ch){delayBuffer[ch].assign((size_t)(sr*2.1),0.f);reverbBuffer[ch].assign((size_t)(sr*.65),0.f);chorusBuffer[ch].assign((size_t)(sr*.09),0.f);}
 delayPos=reverbPos=chorusPos=0;fxClock=0.0;lpFx.fill(0);hpFx.fill(0);eqLow.fill(0);eqHigh.fill(0);compEnv.fill(0);revLp.fill(0);
}
void BassforgeProcessor::moveFx(int slot,int delta){
 const int other=slot+delta;if(slot<0||slot>=fxCount||other<0||other>=fxCount)return;
 int a=fxOrder[(size_t)slot].load(),b=fxOrder[(size_t)other].load();fxOrder[(size_t)slot].store(b);fxOrder[(size_t)other].store(a);
}
float BassforgeProcessor::processFx(int effect,float x,int ch,float amount,float time,float rate){
 const float pi=juce::MathConstants<float>::pi;
 auto smooth=[rate](float hz){return 1.f-std::exp(-2.f*juce::MathConstants<float>::pi*hz/rate);};
 switch(effect){
 case 0: return std::tanh(x*(1.f+amount*20.f)) / std::tanh(1.f+amount*20.f);
 case 1: {float hz=130.f+amount*14000.f;lpFx[ch]+=smooth(hz)*(x-lpFx[ch]);return lpFx[ch];}
 case 2: { // two-band upward saturation / gentle dynamic control
   float e=compEnv[ch];e+=(std::abs(x)-e)*(std::abs(x)>e?0.015f:0.0008f);compEnv[ch]=e;
   float gain=1.f/(1.f+amount*3.5f*e);return std::tanh(x*gain*(1.f+amount*2.f));
 }
 case 3: {eqLow[ch]+=smooth(210.f)*(x-eqLow[ch]);eqHigh[ch]+=smooth(3000.f)*(x-eqHigh[ch]);return x+amount*(0.55f*eqLow[ch]+0.65f*(x-eqHigh[ch])-.25f*(eqHigh[ch]-eqLow[ch]));}
 case 4: {auto& b=chorusBuffer[ch];if(b.empty())return x;int len=(int)b.size();int d=(int)((0.011f+0.006f*std::sin(2*pi*(0.18f+amount*.9f)*time+ch*1.5f))*rate);int ix=(chorusPos-d+len*2)%len;float y=b[(size_t)ix];b[(size_t)chorusPos]=x;return y;}
 case 5: {auto& b=delayBuffer[ch];if(b.empty())return x;int len=(int)b.size();int d=juce::jlimit(1,len-1,(int)((.12f+amount*.55f)*rate));int ix=(delayPos-d+len*2)%len;float y=b[(size_t)ix];b[(size_t)delayPos]=x+0.38f*y;return y;}
 case 6: {auto& b=reverbBuffer[ch];if(b.empty())return x;int len=(int)b.size();int d=juce::jlimit(1,len-1,(int)((.071f+ch*.017f+amount*.19f)*rate));int ix=(reverbPos-d+len*2)%len;float y=b[(size_t)ix];revLp[ch]+=.22f*(y-revLp[ch]);b[(size_t)reverbPos]=x+revLp[ch]*(0.45f+amount*.35f);return y;}
 }
 return x;
}
void BassforgeProcessor::noteOn(int note,float vel){Voice* v=nullptr;for(auto& a:voices)if(!a.active){v=&a;break;}if(!v)v=&voices[0];*v=Voice{};v->active=true;v->note=note;v->velocity=vel;}
void BassforgeProcessor::noteOff(int note){for(auto& v:voices)if(v.active&&v.note==note)v.releasing=true;}
void BassforgeProcessor::processBlock(juce::AudioBuffer<float>& out,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;out.clear();
 auto get=[this](const char* id){return params.getRawParameterValue(id)->load();};
 const float drive=get("drive"),fm=get("fm"),detune=get("detune"),cutoff=get("cutoff"),movement=get("movement"),release=get("release"),tone=get("tone"),width=get("width"),sub=get("sub"),output=get("output");
 const float punch=get("punch"), rasp=get("rasp");
 const int shape=(int)get("shape");
 auto wave=[](double phase,int type){const double s=std::sin(phase*juce::MathConstants<double>::twoPi);if(type==0)return 2.0*(phase-std::floor(phase))-1.0;if(type==1)return std::sin(phase*juce::MathConstants<double>::twoPi)+.35*std::sin(phase*juce::MathConstants<double>::twoPi*2.0);return std::sin(phase*juce::MathConstants<double>::twoPi)+.6*std::sin(phase*juce::MathConstants<double>::twoPi*3.0);};
 juce::MidiBuffer::Iterator it(midi);juce::MidiMessage msg;int eventPos=0;bool has=it.getNextEvent(msg,eventPos);
 for(int i=0;i<out.getNumSamples();++i){while(has&&eventPos<=i){if(msg.isNoteOn())noteOn(msg.getNoteNumber(),msg.getFloatVelocity());else if(msg.isNoteOff())noteOff(msg.getNoteNumber());else if(msg.isAllNotesOff()||msg.isAllSoundOff())for(auto& v:voices)v.releasing=true;has=it.getNextEvent(msg,eventPos);}
 double l=0,r=0;
 for(auto& v:voices){if(!v.active)continue;
 const double freq=juce::MidiMessage::getMidiNoteInHertz(v.note);
 const double delta=freq/sampleRateHz;
 const double mod=std::sin(juce::MathConstants<double>::twoPi*v.lfo);
 const double fmWave=std::sin(juce::MathConstants<double>::twoPi*v.phase2);
 const double ph=v.phase+fm*.7*fmWave + rasp*.22*std::sin(juce::MathConstants<double>::twoPi*(v.phase2*3.0));
 const double det=1.0+.018*detune;
 const double a=wave(ph,shape),b=wave(v.phase2,shape);
 const double raw=.62*a+.38*b;
 const double folded=raw + rasp*.5*std::sin(raw*12.0);
 const double dirty=std::tanh(folded*(1.+drive*16.));
 const double cleanSub=std::sin(juce::MathConstants<double>::twoPi*v.subPhase)*sub*.75;
 const double bright=dirty*(.28+tone*.72) + rasp*.20*(dirty-v.lpL);
 const double targetHz=juce::jlimit(80.0,16000.0,100.+cutoff*10500.*(1.+.65*movement*mod));
 const double alpha=1.-std::exp(-juce::MathConstants<double>::twoPi*targetHz/sampleRateHz);
 const double stereo=width*.013;
 const double leftIn=bright*(1.-stereo*mod)+cleanSub;
 const double rightIn=bright*(1.+stereo*mod)+cleanSub;
 v.lpL+=(float)(alpha*(leftIn-v.lpL));v.lpR+=(float)(alpha*(rightIn-v.lpR));
 const float attack=1.f/(float)(sampleRateHz*.004);
 if(!v.releasing)v.env=juce::jmin(1.f,v.env+attack);
 else v.env*=std::exp(-1.f/(float)(sampleRateHz*(.025+release*1.5)));
 if(v.env<.0001f&&v.releasing){v.active=false;continue;}
 const double transient=1.0+punch*1.8*std::exp(-v.age*55.0);
 const double gain=v.env*v.velocity*transient;
 l+=v.lpL*gain;r+=v.lpR*gain;
 v.age+=1.0/sampleRateHz;
 v.phase+=delta;v.phase-=std::floor(v.phase);
 v.phase2+=delta*det;v.phase2-=std::floor(v.phase2);
 v.subPhase+=delta*.5;v.subPhase-=std::floor(v.subPhase);
 v.lfo+=(.15+movement*12.)/sampleRateHz;v.lfo-=std::floor(v.lfo);
 }
 const float g=.12f+output*.45f;
 float stereoOut[2]={(float)std::tanh(l*g),(float)std::tanh(r*g)};
 const bool bypass=params.getRawParameterValue("fxbypass")->load()>0.5f;
 if(!bypass){
  for(int slot=0;slot<fxCount;++slot){
   int fx=fxOrder[(size_t)slot].load();fx=juce::jlimit(0,fxCount-1,fx);
   auto prefix="fx"+juce::String(fx);
   if(params.getRawParameterValue(prefix+"on")->load()<0.5f)continue;
   float mix=params.getRawParameterValue(prefix+"mix")->load();
   float amt=params.getRawParameterValue(prefix+"amt")->load();
   for(int ch=0;ch<2;++ch){float dry=stereoOut[ch];float wet=processFx(fx,dry,ch,amt,(float)fxClock, (float)sampleRateHz);stereoOut[ch]=dry+(wet-dry)*mix;}
  }
 }
 for(int ch=0;ch<2;++ch)out.setSample(ch,i,std::tanh(stereoOut[ch]));
 if(!delayBuffer[0].empty())delayPos=(delayPos+1)%(int)delayBuffer[0].size();
 if(!reverbBuffer[0].empty())reverbPos=(reverbPos+1)%(int)reverbBuffer[0].size();
 if(!chorusBuffer[0].empty())chorusPos=(chorusPos+1)%(int)chorusBuffer[0].size();
 fxClock+=1.0/sampleRateHz;
 }
}
void BassforgeProcessor::setCurrentProgram(int i){if(!juce::isPositiveAndBelow(i,30))return;currentProgram=i;const auto& p=factoryPatches[(size_t)i];const std::pair<const char*,float> values[]={{"drive",p.drive},{"fm",p.fm},{"detune",p.detune},{"cutoff",p.cutoff},{"movement",p.movement},{"release",p.release},{"tone",p.tone},{"width",p.width},{"punch",p.punch},{"rasp",p.rasp}};for(auto& v:values)if(auto* param=params.getParameter(v.first))param->setValueNotifyingHost(v.second);if(auto* param=params.getParameter("shape"))param->setValueNotifyingHost(p.shape/2.f);}
const juce::String BassforgeProcessor::getProgramName(int i){return juce::isPositiveAndBelow(i,30)?factoryPatches[(size_t)i].name:"";}
void BassforgeProcessor::getStateInformation(juce::MemoryBlock& b){auto tree=params.copyState();tree.setProperty("program",currentProgram,nullptr);for(int i=0;i<fxCount;++i)tree.setProperty("order"+juce::String(i),fxOrder[(size_t)i].load(),nullptr);std::unique_ptr<juce::XmlElement> xml(tree.createXml());copyXmlToBinary(*xml,b);}
void BassforgeProcessor::setStateInformation(const void* data,int bytes){std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,bytes));if(xml&&xml->hasTagName(params.state.getType())){auto tree=juce::ValueTree::fromXml(*xml);currentProgram=(int)tree.getProperty("program",0);for(int i=0;i<fxCount;++i)fxOrder[(size_t)i].store(juce::jlimit(0,fxCount-1,(int)tree.getProperty("order"+juce::String(i),i)));params.replaceState(tree);}}

juce::var BassforgeProcessor::exportPreset(const juce::String& name) const {
 auto* obj = new juce::DynamicObject();
 obj->setProperty("format", "bassforge-preset");
 obj->setProperty("version", 1);
 obj->setProperty("name", name);
 auto* values = new juce::DynamicObject();
 for (auto* param : getParameters()) {
  if (auto* id = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
   values->setProperty(id->paramID, param->getValue());
 }
 obj->setProperty("parameters", juce::var(values));
 juce::Array<juce::var> order;
 for (int i=0;i<fxCount;++i) order.add(fxOrder[(size_t)i].load());
 obj->setProperty("fxOrder", juce::var(order));
 return juce::var(obj);
}
bool BassforgeProcessor::importPreset(const juce::var& preset, juce::String& error) {
 auto* obj=preset.getDynamicObject();
 if (!obj || obj->getProperty("format").toString()!="bassforge-preset" || (int)obj->getProperty("version")!=1) {error="Not a supported BASSFORGE preset (format/version).";return false;}
 auto* values=obj->getProperty("parameters").getDynamicObject();
 if (!values) {error="Preset has no parameters.";return false;}
 // Validate all fields before modifying any live parameters.
 for (auto& property : values->getProperties()) {
  auto* param=params.getParameter(property.name.toString());
  if (!param || !(property.value.isDouble() || property.value.isInt() || property.value.isInt64())) {error="Invalid parameter: "+property.name.toString();return false;}
  double v=(double)property.value;
  if (!std::isfinite(v) || v<0.0 || v>1.0) {error="Parameter out of range: "+property.name.toString();return false;}
 }
 auto order=obj->getProperty("fxOrder");
 if (!order.isArray() || order.size()!=fxCount) {error="Invalid FX order.";return false;}
 std::array<bool,fxCount> seen{};
 for(int i=0;i<fxCount;++i){int v=(int)order[i];if(v<0||v>=fxCount||seen[(size_t)v]){error="FX order must be a permutation.";return false;}seen[(size_t)v]=true;}
 for (auto& property : values->getProperties())
  if (auto* param=params.getParameter(property.name.toString())) param->setValueNotifyingHost((float)(double)property.value);
 for(int i=0;i<fxCount;++i)fxOrder[(size_t)i].store((int)order[i]);
 currentProgram=0;
 return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new BassforgeProcessor();}
