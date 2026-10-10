#include "PluginEditor.h"
namespace {
const auto bg=juce::Colour(0xff090d13), panel=juce::Colour(0xff141a24), stroke=juce::Colour(0xff283241), accent=juce::Colour(0xff69e9d5), muted=juce::Colour(0xff8795a8), white=juce::Colour(0xfff1f5fa);
void sectionText(juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area) {
 g.setColour(muted); g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold))); g.drawText(text,area,juce::Justification::centredLeft);
}
void panelBox(juce::Graphics& g, juce::Rectangle<float> r) {
 g.setColour(panel);g.fillRoundedRectangle(r,13.f);g.setColour(stroke);g.drawRoundedRectangle(r,13.f,1.f);
}
}
void BassforgeEditor::ForgeKeyboard::drawWhiteNote(int note, juce::Graphics& g, juce::Rectangle<float> area, bool down, bool over, juce::Colour, juce::Colour) {
 g.setColour(down?accent:(over?juce::Colour(0xffd7e4e8):juce::Colour(0xffc1cbd5)));
 g.fillRoundedRectangle(area.reduced(1.f,1.f),3.f);
 g.setColour(juce::Colour(0xff657383));g.drawRoundedRectangle(area.reduced(1.f,1.f),3.f,0.7f);
 if(note%12==0){g.setColour(juce::Colour(0xff405060));g.setFont(juce::Font(juce::FontOptions(10.f,juce::Font::bold)));g.drawText(juce::MidiMessage::getMidiNoteName(note,true,true,3),area.removeFromBottom(17.f),juce::Justification::centred);}
}
void BassforgeEditor::ForgeKeyboard::drawBlackNote(int, juce::Graphics& g, juce::Rectangle<float> area, bool down, bool over, juce::Colour) {
 g.setColour(down?accent:(over?juce::Colour(0xff43566a):juce::Colour(0xff202b39)));
 g.fillRoundedRectangle(area.reduced(.5f,0.f),3.f);g.setColour(juce::Colour(0xff070b10));g.drawRoundedRectangle(area.reduced(.5f,0.f),3.f,1.f);
}
BassforgeEditor::ForgeLookAndFeel::ForgeLookAndFeel(){
 setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff1b2a38));
 setColour(juce::ComboBox::outlineColourId,stroke);
 setColour(juce::ComboBox::textColourId,juce::Colours::white);
 setColour(juce::PopupMenu::backgroundColourId,panel);
 setColour(juce::PopupMenu::textColourId,juce::Colours::white);
 setColour(juce::TextButton::textColourOffId,juce::Colours::white);
 setColour(juce::Slider::textBoxTextColourId,muted);
 setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
 setColour(juce::ToggleButton::textColourId,muted);
}
void BassforgeEditor::ForgeLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float pos,float start,float end,juce::Slider&){
 const auto cx=x+width*.5f,cy=y+height*.47f,r=juce::jmin(width,height)*.31f;
 g.setColour(juce::Colour(0xff0b1119));g.fillEllipse(cx-r,cy-r,r*2,r*2);
 g.setColour(juce::Colour(0xff334151));g.drawEllipse(cx-r,cy-r,r*2,r*2,1.4f);
 juce::Path track;track.addCentredArc(cx,cy,r+6,r+6,0,start,end,true);
 g.setColour(juce::Colour(0xff344353));g.strokePath(track,juce::PathStrokeType(3.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 juce::Path active;active.addCentredArc(cx,cy,r+6,r+6,0,start,start+pos*(end-start),true);
 g.setColour(accent);g.strokePath(active,juce::PathStrokeType(3.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 const auto a=start+pos*(end-start);g.setColour(white);g.drawLine(cx,cy,cx+std::sin(a)*r*.75f,cy-std::cos(a)*r*.75f,2.5f);
 g.setColour(juce::Colour(0xff111b27));g.fillEllipse(cx-3.f,cy-3.f,6.f,6.f);
}
void BassforgeEditor::ForgeLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down){
 auto r=b.getLocalBounds().toFloat().reduced(.5f);auto selected=b.getToggleState();
 g.setColour(selected?juce::Colour(0xff17453f):(down?juce::Colour(0xff2a4c58):over?juce::Colour(0xff253949):juce::Colour(0xff1a2937)));
 g.fillRoundedRectangle(r,7.f);g.setColour(selected?accent:stroke);g.drawRoundedRectangle(r,7.f,1.f);
}

BassforgeEditor::BassforgeEditor(BassforgeProcessor& p):AudioProcessorEditor(&p),processor(p){
 setLookAndFeel(&theme);
 setSize(1120,740);
 setResizable(true,true);setResizeLimits(1000,680,1600,1050);
 synthTab.onClick=[this]{showFx=false;updatePage();};fxTab.onClick=[this]{showFx=true;updatePage();};addAndMakeVisible(synthTab);addAndMakeVisible(fxTab);
 title.setText("BASSFORGE",juce::dontSendNotification);title.setFont(juce::Font(juce::FontOptions(34.f,juce::Font::bold)));title.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(title);
 subtitle.setText("NEURO BASS ENGINE     /     v0.5  PRESET LAB",juce::dontSendNotification);subtitle.setColour(juce::Label::textColourId,muted);addAndMakeVisible(subtitle);
 for(int i=0;i<30;++i)presets.addItem(factoryPatches[(size_t)i].name,i+1);
 presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
 presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);};addAndMakeVisible(presets);
 addAndMakeVisible(savePreset);addAndMakeVisible(loadPreset);addAndMakeVisible(loadPack);
 savePreset.onClick=[this]{
  chooser=std::make_unique<juce::FileChooser>("Save BASSFORGE preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("My Bass.bfpreset"),"*.bfpreset");
  chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser& c){auto f=c.getResult();if(f==juce::File{})return;f=f.withFileExtension("bfpreset");auto json=juce::JSON::toString(processor.exportPreset(f.getFileNameWithoutExtension()),true);if(!f.replaceWithText(json))juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Save failed",f.getFullPathName());});
 };
 loadPreset.onClick=[this]{
  chooser=std::make_unique<juce::FileChooser>("Load BASSFORGE preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.bfpreset");
  chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c){auto f=c.getResult();if(f!=juce::File{})loadPresetFile(f);});
 };
 loadPack.onClick=[this]{
  chooser=std::make_unique<juce::FileChooser>("Load BASSFORGE pack ZIP",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.zip");
  chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c){
   auto f=c.getResult();if(f==juce::File{})return;
   juce::ZipFile zip(f);juce::StringArray names;
   for(int i=0;i<zip.getNumEntries();++i)if(auto* entry=zip.getEntry(i))if(entry->filename.endsWithIgnoreCase(".bfpreset") && !entry->filename.contains("..") && entry->uncompressedSize<1000000)names.add(entry->filename);
   if(names.isEmpty()){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Empty pack","No valid .bfpreset files found.");return;}
   juce::PopupMenu menu;for(int i=0;i<names.size();++i)menu.addItem(i+1,names[i]);
   menu.showMenuAsync(juce::PopupMenu::Options(),[this,f,names](int selection){if(selection<=0)return;juce::ZipFile archive(f);auto stream=std::unique_ptr<juce::InputStream>(archive.createStreamForEntry(names[selection-1]));if(!stream)return;auto json=juce::JSON::parse(stream->readEntireStreamAsString());juce::String error;if(!processor.importPreset(json,error)){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Preset error",error);return;}refreshControls();});
  });
 };
 const char* ids[]={"drive","fm","detune","cutoff","movement","release","tone","width","sub","output","punch","rasp"};
 for(int i=0;i<12;++i){auto& s=knobs[(size_t)i];s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,21);s.setColour(juce::Slider::rotarySliderFillColourId,accent);s.setColour(juce::Slider::thumbColourId,juce::Colours::white);addAndMakeVisible(s);auto& l=labels[(size_t)i];l.setText(juce::String(ids[i]).toUpperCase(),juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);l.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(l);attachments[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.params,ids[i],s);}
 shape.addItemList({"Saw","Horn","Metal"},1);addAndMakeVisible(shape);shapeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.params,"shape",shape);
 keyboard.setAvailableRange(24,84);keyboard.setKeyWidth(22.f);keyboard.setScrollButtonsVisible(false);keyboard.setVelocity(0.85f,false);addAndMakeVisible(keyboard);
 bypassAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.params,"fxbypass",bypass);addAndMakeVisible(bypass);
 for(int i=0;i<7;++i){
  auto id="fx"+juce::String(i);
  addAndMakeVisible(fxLabels[(size_t)i]);addAndMakeVisible(fxEnabled[(size_t)i]);
  addAndMakeVisible(fxMix[(size_t)i]);addAndMakeVisible(fxAmount[(size_t)i]);
  auto setup=[](juce::Slider& sl){sl.setSliderStyle(juce::Slider::LinearHorizontal);sl.setTextBoxStyle(juce::Slider::TextBoxRight,false,55,20);};
  setup(fxMix[(size_t)i]);setup(fxAmount[(size_t)i]);
  fxEnabledAttach[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.params,id+"on",fxEnabled[(size_t)i]);
  fxMixAttach[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.params,id+"mix",fxMix[(size_t)i]);
  fxAmountAttach[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.params,id+"amt",fxAmount[(size_t)i]);
  fxUp[(size_t)i].setButtonText("UP");fxDown[(size_t)i].setButtonText("DOWN");
  fxUp[(size_t)i].onClick=[this,i]{for(int j=0;j<7;++j)if(processor.getFxAt(j)==i){processor.moveFx(j,-1);break;}refreshFxLabels();};
  fxDown[(size_t)i].onClick=[this,i]{for(int j=0;j<7;++j)if(processor.getFxAt(j)==i){processor.moveFx(j,1);break;}refreshFxLabels();};
  fxMix[(size_t)i].setColour(juce::Slider::trackColourId,accent);fxAmount[(size_t)i].setColour(juce::Slider::trackColourId,accent);
  addAndMakeVisible(fxUp[(size_t)i]);addAndMakeVisible(fxDown[(size_t)i]);
 }
 refreshFxLabels();updatePage();
}
void BassforgeEditor::loadPresetFile(const juce::File& file){
 if(file.getSize()>1000000){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Invalid preset","File too large.");return;}
 auto data=juce::JSON::parse(file.loadFileAsString());juce::String error;
 if(!processor.importPreset(data,error)){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Preset error",error);return;}
 refreshControls();
}
void BassforgeEditor::refreshControls(){
 presets.setSelectedId(0,juce::dontSendNotification);
 refreshFxLabels();
}
void BassforgeEditor::refreshFxLabels(){
 for(int i=0;i<7;++i){int fx=i;fxLabels[(size_t)i].setText(juce::String([this,i]{for(int j=0;j<7;++j)if(processor.getFxAt(j)==i)return j+1;return i+1;}())+"  "+BassforgeProcessor::fxNames[fx],juce::dontSendNotification);}
}
void BassforgeEditor::updatePage(){
 for(auto& x:knobs)x.setVisible(!showFx);for(auto& x:labels)x.setVisible(!showFx);shape.setVisible(!showFx);
 for(int i=0;i<7;++i){fxLabels[(size_t)i].setVisible(showFx);fxEnabled[(size_t)i].setVisible(showFx);fxMix[(size_t)i].setVisible(showFx);fxAmount[(size_t)i].setVisible(showFx);fxUp[(size_t)i].setVisible(showFx);fxDown[(size_t)i].setVisible(showFx);}
 bypass.setVisible(showFx);shape.setVisible(!showFx);synthTab.setToggleState(!showFx,juce::dontSendNotification);fxTab.setToggleState(showFx,juce::dontSendNotification);repaint();
}
void BassforgeEditor::paint(juce::Graphics& g){
 const float w=(float)getWidth(),h=(float)getHeight();
 g.fillAll(bg);
 juce::ColourGradient header(juce::Colour(0xff1a2634),0.f,0.f,bg,w,118.f,false);
 g.setGradientFill(header);g.fillRect(0,0,getWidth(),112);
 g.setColour(accent);g.fillRoundedRectangle(27.f,28.f,4.f,43.f,2.f);
 g.setColour(stroke);g.drawLine(24.f,112.f,w-24.f,112.f,1.f);
 panelBox(g,{24.f,168.f,w-48.f,h-292.f});
 if(!showFx){
  auto left=juce::Rectangle<float>(42.f,188.f,(w-104.f)*.5f-9.f,h-338.f);
  auto right=juce::Rectangle<float>(left.getRight()+18.f,188.f,(w-104.f)*.5f-9.f,h-338.f);
  g.setColour(juce::Colour(0xff192330));g.fillRoundedRectangle(left,10.f);g.fillRoundedRectangle(right,10.f);
  g.setColour(stroke);g.drawRoundedRectangle(left,10.f,1.f);g.drawRoundedRectangle(right,10.f,1.f);
  sectionText(g,"01   /   OSCILLATOR & MODULATION",{(int)left.getX()+19,198,(int)left.getWidth()-38,24});
  sectionText(g,"02   /   DRIVE & OUTPUT",{(int)right.getX()+19,198,(int)right.getWidth()-38,24});
 } else {
  sectionText(g,"SIGNAL CHAIN     /     ENABLE • MIX • CHARACTER • ORDER",{45,180,getWidth()-90,24});
  const float y0=223.f,row=(h-365.f)/7.f;
  for(int i=0;i<7;++i){auto r=juce::Rectangle<float>(43.f,y0+i*row,w-86.f,row-7.f);g.setColour(i%2?juce::Colour(0xff1a2532):juce::Colour(0xff18212d));g.fillRoundedRectangle(r,7.f);}
 }
 panelBox(g,{24.f,h-110.f,w-48.f,91.f});
 sectionText(g,"PLAY   /   MIDI KEYBOARD",{43,(int)h-105,210,19});
 g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(11.f)));g.drawText("CLICK TO AUDITION  •  MIDI INPUT SUPPORTED",getWidth()-345,getHeight()-105,302,19,juce::Justification::centredRight);
}
void BassforgeEditor::resized(){
 const int w=getWidth(),h=getHeight();
 title.setBounds(43,21,350,45);subtitle.setBounds(45,68,420,22);
 presets.setBounds(w-405,21,380,36);
 savePreset.setBounds(w-405,67,120,31);loadPreset.setBounds(w-278,67,120,31);loadPack.setBounds(w-151,67,126,31);
 synthTab.setBounds(26,124,139,32);fxTab.setBounds(174,124,139,32);
 shape.setBounds(w-220,127,194,29);bypass.setBounds(w-230,127,204,29);
 // Six sound controls in each of two sections. Fixed semantic grouping, responsive widths.
 const float groupW=((float)w-122.f)*.5f-9.f;
 const float cell=groupW/3.f;
 const float available=(float)h-338.f;
 const float row=available*.5f;
 for(int i=0;i<12;++i){
  const int section=i/6, local=i%6, col=local%3, r=local/3;
  const float baseX=section==0?52.f:70.f+groupW;
  const float x=baseX+col*cell,y=235.f+r*row;
  knobs[(size_t)i].setBounds((int)(x+5),(int)y,(int)(cell-10),juce::jmin(112,(int)row-31));
  labels[(size_t)i].setBounds((int)(x+5),(int)(y+juce::jmin(112,(int)row-31)),(int)(cell-10),22);
 }
 const float fxTop=223.f,fxRow=((float)h-365.f)/7.f;
 for(int i=0;i<7;++i){int y=(int)(fxTop+i*fxRow+4.f);
  fxLabels[(size_t)i].setBounds(55,y,175,31);fxEnabled[(size_t)i].setBounds(239,y+2,32,28);
  fxMix[(size_t)i].setBounds(283,y,juce::jmax(155,(w-1010)/2+205),31);
  fxAmount[(size_t)i].setBounds(w-415,y,243,31);
  fxUp[(size_t)i].setBounds(w-161,y,58,30);fxDown[(size_t)i].setBounds(w-96,y,62,30);
 }
 keyboard.setBounds(43,h-82,w-86,58);
}
BassforgeEditor::~BassforgeEditor(){setLookAndFeel(nullptr);}
juce::AudioProcessorEditor* BassforgeProcessor::createEditor(){return new BassforgeEditor(*this);}
