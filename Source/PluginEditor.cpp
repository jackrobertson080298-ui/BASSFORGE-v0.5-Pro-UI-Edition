#include "PluginEditor.h"
namespace {
const auto bg=juce::Colour(0xff0a1019), panel=juce::Colour(0xff141f2b), stroke=juce::Colour(0xff263647), accent=juce::Colour(0xff41e2ca), muted=juce::Colour(0xff91a6b7);
void sectionText(juce::Graphics& g,const juce::String& text,int x,int y){g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));g.drawText(text,x,y,250,18,juce::Justification::centredLeft);}
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
 const auto cx=x+width*0.5f,cy=y+height*0.46f,r=juce::jmin(width,height)*0.34f;
 g.setColour(juce::Colour(0xff0b141d));g.fillEllipse(cx-r,cy-r,r*2,r*2);
 g.setColour(stroke);g.drawEllipse(cx-r,cy-r,r*2,r*2,1.5f);
 juce::Path track;track.addCentredArc(cx,cy,r+5,r+5,0,start,end,true);
 g.setColour(juce::Colour(0xff304554));g.strokePath(track,juce::PathStrokeType(3.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 juce::Path active;active.addCentredArc(cx,cy,r+5,r+5,0,start,start+pos*(end-start),true);
 g.setColour(accent);g.strokePath(active,juce::PathStrokeType(3.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 const auto a=start+pos*(end-start);juce::Point<float> tip(cx+std::sin(a)*r*.72f,cy-std::cos(a)*r*.72f);
 g.setColour(juce::Colours::white);g.drawLine(cx,cy,tip.x,tip.y,2.2f);
}
void BassforgeEditor::ForgeLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down){
 auto r=b.getLocalBounds().toFloat().reduced(.5f);auto selected=b.getToggleState();
 g.setColour(selected?juce::Colour(0xff17453f):(down?juce::Colour(0xff2a4c58):over?juce::Colour(0xff253949):juce::Colour(0xff1a2937)));
 g.fillRoundedRectangle(r,7.f);g.setColour(selected?accent:stroke);g.drawRoundedRectangle(r,7.f,1.f);
}

BassforgeEditor::BassforgeEditor(BassforgeProcessor& p):AudioProcessorEditor(&p),processor(p){
 setLookAndFeel(&theme);
 setSize(940,760);
 setResizable(true,true);setResizeLimits(940,760,1500,1100);
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
   menu.showMenuAsync(juce::PopupMenu::Options(),[this,f,names](int selection){if(selection<=0)return;juce::ZipFile archive(f);int entryIndex=-1;for(int i=0;i<archive.getNumEntries();++i){auto* entry=archive.getEntry(i);if(entry!=nullptr && entry->filename==names[selection-1]){entryIndex=i;break;}}if(entryIndex<0)return;auto stream=std::unique_ptr<juce::InputStream>(archive.createStreamForEntry(entryIndex));if(!stream)return;auto json=juce::JSON::parse(stream->readEntireStreamAsString());juce::String error;if(!processor.importPreset(json,error)){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Preset error",error);return;}refreshControls();});
  });
 };
 const char* ids[]={"drive","fm","detune","cutoff","movement","release","tone","width","sub","output","punch","rasp"};
 for(int i=0;i<12;++i){auto& s=knobs[(size_t)i];s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,19);s.setColour(juce::Slider::rotarySliderFillColourId,accent);s.setColour(juce::Slider::thumbColourId,juce::Colours::white);addAndMakeVisible(s);auto& l=labels[(size_t)i];l.setText(juce::String(ids[i]).toUpperCase(),juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);l.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(l);attachments[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.params,ids[i],s);}
 shape.addItemList({"Saw","Horn","Metal"},1);addAndMakeVisible(shape);shapeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.params,"shape",shape);
 keyboard.setAvailableRange(24,96);addAndMakeVisible(keyboard);
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
 bypass.setVisible(showFx);repaint();
}
void BassforgeEditor::paint(juce::Graphics& g){g.fillAll(juce::Colour(0xff101821));g.setColour(juce::Colour(0xff253544));g.fillRoundedRectangle(16.f,140.f,818.f,450.f,12.f);g.setColour(accent);g.fillRect(20,90,810,2);}
void BassforgeEditor::resized(){
 const auto w=getWidth(),h=getHeight();
 title.setBounds(40,14,330,46);subtitle.setBounds(41,61,365,23);
 presets.setBounds(w-370,20,346,35);
 savePreset.setBounds(w-370,67,108,29);loadPreset.setBounds(w-255,67,108,29);loadPack.setBounds(w-140,67,116,29);
 synthTab.setBounds(24,121,122,34);fxTab.setBounds(155,121,122,34);
 shape.setBounds(w-216,125,192,27);bypass.setBounds(w-216,125,192,27);
 const float innerW=(float)w-74.f,cellW=innerW/6.f;
 const float contentH=(float)h-275.f;
 for(int i=0;i<12;++i){int col=i%6,row=i/6;float x=37.f+col*cellW,y=220.f+row*((contentH-90.f)/2.f);
  knobs[(size_t)i].setBounds((int)(x+6),(int)(y+10),(int)(cellW-20),98);
  labels[(size_t)i].setBounds((int)(x+5),(int)(y+110),(int)(cellW-18),22);
 }
 const float rowH=(contentH-65.f)/7.f;
 for(int i=0;i<7;++i){int y=(int)(213.f+i*rowH+7.f);
  fxLabels[(size_t)i].setBounds(46,y,173,29);fxEnabled[(size_t)i].setBounds(230,y+1,27,27);
  fxMix[(size_t)i].setBounds(285,y,juce::jmax(110,(w-940)/2+225),29);
  fxAmount[(size_t)i].setBounds(w-390,y,225,29);
  fxUp[(size_t)i].setBounds(w-151,y,57,29);fxDown[(size_t)i].setBounds(w-87,y,58,29);
 }
 keyboard.setBounds(24,h-86,w-48,65);
}
BassforgeEditor::~BassforgeEditor(){setLookAndFeel(nullptr);}
juce::AudioProcessorEditor* BassforgeProcessor::createEditor(){return new BassforgeEditor(*this);}
