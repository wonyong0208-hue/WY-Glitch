#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour bg (0xff0a101b), panel (0xff131d2c), muted (0xff9babc0);
const std::array<juce::Colour,4> accents {juce::Colour(0xff47e6db), juce::Colour(0xff70a9ff), juce::Colour(0xffd38bfa), juce::Colour(0xffffac72)};
const char* modeNames[] = {"LIQUID", "CRYSTAL", "ORGANIC", "PLASMA"};
const char* knobNames[] = {"GLITCH", "SPEED", "CHAOS", "MIX"};
const char* paramNames[] = {"glitch", "speed", "chaos", "mix"};
}

void WYGlitchAudioProcessorEditor::Style::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& slider)
{
    const auto* editor = dynamic_cast<WYGlitchAudioProcessorEditor*>(slider.getParentComponent());
    int index = 0;
    if (editor) for (int i=0; i<4; ++i) if (&editor->knobs[(size_t)i] == &slider) index=i;
    auto colour = accents[(size_t)index];
    auto r = juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(15.0f);
    auto c = r.getCentre();
    float radius = juce::jmin(r.getWidth(),r.getHeight())*0.44f;
    auto angle = start + pos*(end-start);
    g.setColour(colour.withAlpha(0.08f)); g.fillEllipse(c.x-radius-11,c.y-radius-11,2*(radius+11),2*(radius+11));
    juce::Path track,active;
    track.addCentredArc(c.x,c.y,radius,radius,0,start,end,true);
    active.addCentredArc(c.x,c.y,radius,radius,0,start,angle,true);
    g.setColour(juce::Colour(0xff304057)); g.strokePath(track,juce::PathStrokeType(10.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(colour.withAlpha(0.22f)); g.strokePath(active,juce::PathStrokeType(16.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(colour); g.strokePath(active,juce::PathStrokeType(8.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff34465a),c.x-radius,c.y-radius,juce::Colour(0xff101a29),c.x+radius,c.y+radius,false));
    g.fillEllipse(c.x-radius*.79f,c.y-radius*.79f,radius*1.58f,radius*1.58f);
    g.setColour(juce::Colour(0xff61768d)); g.drawEllipse(c.x-radius*.79f,c.y-radius*.79f,radius*1.58f,radius*1.58f,1.0f);
    juce::Point<float> tip(c.x+std::sin(angle)*radius*.64f,c.y-std::cos(angle)*radius*.64f);
    g.setColour(juce::Colours::white); g.drawLine(c.x,c.y,tip.x,tip.y,3.5f);
}

WYGlitchAudioProcessorEditor::WYGlitchAudioProcessorEditor(WYGlitchAudioProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&style);
    for (int i=0;i<4;++i)
    {
        auto& button=modes[(size_t)i];
        button.setButtonText(modeNames[i]);
        button.setColour(juce::TextButton::textColourOffId,juce::Colours::white);
        button.onClick=[this,i]{ if(auto* param=processor.parameters.getParameter("mode")) { param->beginChangeGesture(); param->setValueNotifyingHost((float)i/3.0f); param->endChangeGesture(); } updateModes(); };
        addAndMakeVisible(button);
        auto& knob=knobs[(size_t)i];
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
        knob.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        knob.setRange(0,100,1);
        addAndMakeVisible(knob);
        auto& caption=captions[(size_t)i];
        caption.setText(knobNames[i],juce::dontSendNotification);
        caption.setJustificationType(juce::Justification::centred);
        caption.setColour(juce::Label::textColourId,juce::Colour(0xffe4edfa));
        caption.setFont(juce::Font(juce::FontOptions(19.0f,juce::Font::bold)));
        addAndMakeVisible(caption);
        auto& value=values[(size_t)i];
        value.setJustificationType(juce::Justification::centred);
        value.setColour(juce::Label::textColourId,muted);
        value.setFont(juce::Font(juce::FontOptions(16.0f)));
        addAndMakeVisible(value);
        knob.onValueChange=[this,i]{ values[(size_t)i].setText(juce::String((int)knobs[(size_t)i].getValue())+"%",juce::dontSendNotification); };
        attachments[(size_t)i]=std::make_unique<SliderAttachment>(processor.parameters,paramNames[i],knob);
        knob.onValueChange();
        modeSelector.addItem(modeNames[i],i+1);
    }
    modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,"mode",modeSelector);
    modeSelector.onChange=[this]{updateModes();};
    presets.addItem("Crystal Shatter",1);
    presets.addItem("Liquid Drift",2);
    presets.addItem("Organic Mutation",3);
    presets.addItem("Plasma Burst",4);
    presets.setSelectedId(1,juce::dontSendNotification);
    presets.onChange=[this]{loadPreset(presets.getSelectedId());};
    presets.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff172336));
    presets.setColour(juce::ComboBox::textColourId,juce::Colours::white);
    presets.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff455c77));
    addAndMakeVisible(presets);
    syncLabel.setText("HOST BPM SYNC  /  SPEED SELECTS DIVISION",juce::dontSendNotification);
    syncLabel.setColour(juce::Label::textColourId,juce::Colour(0xff75e6e3));
    syncLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    addAndMakeVisible(syncLabel);
    updateModes();
    setSize(920,600);
    startTimerHz(25);
}

WYGlitchAudioProcessorEditor::~WYGlitchAudioProcessorEditor()
{
    stopTimer();
    modeAttachment.reset();
    for(auto& a:attachments)a.reset();
    setLookAndFeel(nullptr);
}

void WYGlitchAudioProcessorEditor::loadPreset(int id)
{
    if(id<1 || id>4)return;
    const float preset[4][4]={{55,65,40,80},{38,32,25,85},{65,55,80,90},{85,90,70,95}};
    const int mode[]={1,0,2,3};
    if(auto* p=processor.parameters.getParameter("mode")) {p->beginChangeGesture();p->setValueNotifyingHost((float)mode[id-1]/3.0f);p->endChangeGesture();}
    for(int i=0;i<4;++i)knobs[(size_t)i].setValue(preset[id-1][i],juce::sendNotificationSync);
    updateModes();
}

void WYGlitchAudioProcessorEditor::updateModes()
{
    int selected=juce::jlimit(0,3,modeSelector.getSelectedItemIndex());
    for(int i=0;i<4;++i)
    {
        modes[(size_t)i].setColour(juce::TextButton::buttonColourId,i==selected?accents[(size_t)i].withAlpha(.65f):juce::Colour(0xff1b283a));
        modes[(size_t)i].setColour(juce::TextButton::textColourOffId,i==selected?juce::Colours::white:muted);
    }
    repaint();
}

void WYGlitchAudioProcessorEditor::timerCallback(){phase+=0.065f;repaint(juce::Rectangle<int>(28,200,864,165));}

void WYGlitchAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto bounds=getLocalBounds().toFloat().reduced(5.0f);
    g.setColour(juce::Colour(0xff354760));g.drawRoundedRectangle(bounds,22.0f,1.5f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff51dbe6),35,0,juce::Colour(0xffd78bf8),365,0,false));
    g.setFont(juce::Font(juce::FontOptions(37.0f,juce::Font::bold)));
    g.drawText("WY / GLITCH",38,19,355,48,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(12.5f)));
    g.drawText("SOUND DESTRUCTION ENGINE",42,66,370,22,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff29364b));g.drawHorizontalLine(102,30.0f,890.0f);
    for(int i=0;i<4;++i)
    {
        auto r=juce::Rectangle<float>(30.0f+i*219.0f,116.0f,205.0f,78.0f);
        const bool active=i==juce::jlimit(0,3,modeSelector.getSelectedItemIndex());
        g.setGradientFill(juce::ColourGradient(accents[(size_t)i].withAlpha(active?.38f:.16f),r.getX(),r.getY(),panel,r.getRight(),r.getBottom(),false));
        g.fillRoundedRectangle(r,13.0f);
        g.setColour(accents[(size_t)i].withAlpha(active?1.0f:.38f));g.drawRoundedRectangle(r,13.0f,active?2.0f:1.0f);
        // Animated crystalline decorative accents, one palette per texture.
        for(int k=0;k<5;++k){float xx=r.getX()+10+k*41.0f;float yy=r.getY()+9+std::sin(k*2.1f+i)*7.0f;
            g.setColour(accents[(size_t)i].withAlpha(.12f));g.drawLine(xx,yy,xx+28,yy+20,1.0f);}
    }
    auto wave=juce::Rectangle<float>(30,208,860,157);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff102537),wave.getX(),wave.getY(),juce::Colour(0xff1b1534),wave.getRight(),wave.getBottom(),false));
    g.fillRoundedRectangle(wave,15.0f);
    g.setColour(juce::Colour(0xff40516e));g.drawRoundedRectangle(wave,15.0f,1.0f);
    g.setColour(juce::Colour(0xff38506b));g.drawHorizontalLine(286,44.0f,875.0f);
    // Decorative, animated glitch waveform. Not an input-signal analyzer.
    const float intensity=processor.parameters.getRawParameterValue("glitch")->load()/100.0f;
    const float chaos=processor.parameters.getRawParameterValue("chaos")->load()/100.0f;
    const int mode=juce::jlimit(0,3,(int)processor.parameters.getRawParameterValue("mode")->load());
    juce::Path wavePath;
    for(int i=0;i<=430;++i)
    {
        float t=(float)i/430.0f;
        float env=std::pow(juce::jmax(0.0f,std::sin(juce::MathConstants<float>::pi*t)),.8f);
        float spikes=std::sin(t*(mode==1?170.0f:mode==3?245.0f:95.0f)+phase)*std::sin(t*63.0f-phase*.4f);
        float glitch=std::sin(t*800.0f+std::floor(t*33.0f)*chaos*7.0f);
        float yy=286.0f+env*(spikes*(18.0f+intensity*40.0f)+glitch*chaos*14.0f);
        float xx=44.0f+t*832.0f;
        if(i==0)wavePath.startNewSubPath(xx,yy);else wavePath.lineTo(xx,yy);
    }
    g.setColour(accents[(size_t)mode].withAlpha(.23f));g.strokePath(wavePath,juce::PathStrokeType(7.0f));
    g.setColour(accents[(size_t)mode]);g.strokePath(wavePath,juce::PathStrokeType(1.8f));
    for(int i=1;i<4;++i){g.setColour(juce::Colour(0xff26374c));g.drawVerticalLine(i*220+20,386.0f,540.0f);}
    g.setColour(juce::Colour(0xff344b64));g.drawHorizontalLine(546,34.0f,887.0f);
    g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("4 TEXTURES  /  4 CONTROLS",640,558,250,20,juce::Justification::centredRight);
}

void WYGlitchAudioProcessorEditor::resized()
{
    presets.setBounds(480,34,350,39);
    for(int i=0;i<4;++i)
    {
        modes[(size_t)i].setBounds(36+i*219,140,193,41);
        const int x=34+i*220;
        knobs[(size_t)i].setBounds(x+19,373,153,128);
        captions[(size_t)i].setBounds(x+12,497,170,25);
        values[(size_t)i].setBounds(x+12,520,170,24);
    }
    syncLabel.setBounds(38,555,480,23);
}
