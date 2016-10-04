/*
 ==============================================================================

 MainComponent.cpp
 Created: 25 Mar 2016 6:11:02pm
 Author:  Martin Hermant

 ==============================================================================
 */

#include "MainComponent.h"


#include "NodeManagerUI.h"

// (This function is called by the app startup code to create our main component)
MainContentComponent* createMainContentComponent(Engine * e)
{
  return new MainContentComponent(e);
}


MainContentComponent::MainContentComponent(Engine * e):
engine(e)
{
engine->addEngineListener(this);
  
  setLookAndFeel(lookAndFeelOO = new LookAndFeelOO);

  addAndMakeVisible(&ShapeShifterManager::getInstance()->mainContainer);

  ShapeShifterManager::getInstance()->loadLastSessionLayoutFile();

  (&getCommandManager())->registerAllCommandsForTarget (this);
  (&getCommandManager())-> setFirstCommandTarget(this);

  (&getCommandManager())->getKeyMappings()->resetToDefaultMappings();
  //    (&getCommandManager())->getKeyMappings()->restoreFromXml (lastSavedKeyMappingsXML);
  addKeyListener ((&getCommandManager())->getKeyMappings());
#if JUCE_MAC
  setMacMainMenu (this,nullptr,"");
#else
  //setMenu (this); //done in Main.cpp as it's a method of DocumentWindow
#endif


  e->createNewGraph();
  e->setChangedFlag(false);

  //e->initAudio();

  setSize((int)(getParentMonitorArea().getWidth()*.9f), (int)(getParentMonitorArea().getHeight()*.6f));
  setWantsKeyboardFocus(true);
//  grabKeyboardFocus();
}



MainContentComponent::~MainContentComponent(){

#if JUCE_MAC
  setMacMainMenu (nullptr);
#else
  //setMenuBar (nullptr);

#endif
  //    LookAndFeelOO::deleteInstance();

  //DBG("Clear inspector");
  engine->removeEngineListener(this);
  ShapeShifterManager::deleteInstance();
}

void MainContentComponent::focusGained(FocusChangeType cause){
  ShapeShifterManager * sm = ShapeShifterManager::getInstanceWithoutCreating();
  if(sm){
    ShapeShifterContent * nm = ShapeShifterManager::getInstance()->getContentForName(NodeManagerPanel);
    if(nm){
      nm->grabKeyboardFocus();
    }
  }
}


void MainContentComponent::resized()
{
  Rectangle<int> r = getLocalBounds();
  //timeManagerUI->setBounds(r.removeFromTop(25));
  //DBG("Resized in main component :" << getLocalBounds().toString());

  ShapeShifterManager::getInstance()->mainContainer.setBounds(r);
}

void MainContentComponent::showAudioSettings()
{
  AudioDeviceSelectorComponent audioSettingsComp(getAudioDeviceManager(),
                                                 0, 256,
                                                 0, 256,
                                                 true,true, false,false);
  audioSettingsComp.setSize (500, 450);

  DialogWindow::LaunchOptions o;
  o.content.setNonOwned (&audioSettingsComp);
  o.dialogTitle                   = "Audio Settings";
  o.componentToCentreAround       = this;
  o.dialogBackgroundColour        = Colours::azure;
  o.escapeKeyTriggersCloseButton  = true;
  o.useNativeTitleBar             = false;
  o.resizable                     = false;


  o.runModal();

  engine->audioSettingsHandler.saveCurrent();


}

void MainContentComponent::paintOverChildren(Graphics & g) {
  if(engine->isLoadingFile){
    g.setColour(Colours::black.withAlpha(0.4f));
    g.fillAll();

    int period = 4000.0;
    float time = (Time::currentTimeMillis()%period)*1.0/period;
    Point<int> center = getLocalBounds().getCentre();
    int numPoints = 10;
    float radius = 300;
    float pSize =40;
    float aStep = 2*float_Pi/numPoints ;
    for(int i = 0 ; i < numPoints ; i++){
      g.setColour(Colours::white.withAlpha(float(1.0+cos((time+i*0.25/numPoints)*2*float_Pi))/2.0f));
      g.fillEllipse(center.x + radius*cos(i*aStep),center.y+radius*sin(i*aStep),pSize,pSize);

    }

    
  }
};
void MainContentComponent::startLoadFile(){
  
  if(NodeManagerUIViewport * vp = (NodeManagerUIViewport*)ShapeShifterManager::getInstance()->getContentForName(NodeManagerPanel)){
    if(NodeContainerViewer *v = vp->nmui->currentViewer){
      v->clear();
    }
  }
  startTimerHz(30);
  repaint();
}

void MainContentComponent::fileProgress(float percent, int state){
  // not implemented
};
void MainContentComponent::endLoadFile(){
  stopTimer();
  repaint();
};
void MainContentComponent::timerCallback(){
//  if(!engine->isLoadingFile){
//    stopTimer();
//  }

  repaint();
}
