/*
 ==============================================================================

 This file was auto-generated by the Introjucer!

 It contains the basic startup code for a Juce application.

 ==============================================================================
 */


#pragma warning( disable : 4244 )

#include "MainComponent.h"
#include "Engine.h"
#include "StringUtil.h"

MainContentComponent* createMainContentComponent(Engine* e);

//==============================================================================
class LGMLApplication : public JUCEApplication
{
public:
  //==============================================================================
  LGMLApplication() {}

  ApplicationCommandManager commandManager;
  ScopedPointer<ApplicationProperties> appProperties;
  AudioDeviceManager deviceManager;
  UndoManager undoManager;

  ScopedPointer<Engine> engine;


  const String getApplicationName() override       { return ProjectInfo::projectName; }
  const String getApplicationVersion() override    { return ProjectInfo::versionString; }
  bool moreThanOneInstanceAllowed() override       { return false; }

  //==============================================================================
  void initialise (const String& commandLine) override
  {
    // This method is where you should put your application's initialisation code..

    PropertiesFile::Options options;
    options.applicationName     = "LGML";
    options.filenameSuffix      = "settings";
    options.osxLibrarySubFolder = "Preferences";

    appProperties = new ApplicationProperties();
    appProperties->setStorageParameters (options);

	


	Process::setPriority (Process::HighPriority);

    engine = new Engine();
#if LGML_UNIT_TESTS

    UnitTestRunner tstRunner;
    CommandLineElements commandLineElements = StringUtil::parseCommandLine(commandLine);
    if(CommandLineElement elem = commandLineElements.getCommandLineElement("t","")){
      Array<UnitTest*> allTests = UnitTest::getAllTests();
      Array<UnitTest*> testsToRun ;
      for(auto &tName:elem.args){
        bool found = false;
        for(auto & t:allTests){
          if(tName==t->getName()){
            testsToRun.add(t);
            found = true;
            break;
          }
        }
        if(!found){
          DBG("no tests found for : "+tName);
        }
      }
      tstRunner.runTests(testsToRun);
    }
    else{
      tstRunner.runAllTests();
    }
    quit();
#else


    mainWindow = new MainWindow (getApplicationName(),engine);

    engine->parseCommandline(commandLine);

#endif

  }

  void shutdown() override
  {
    // Add your application's shutdown code here..


    mainWindow = nullptr; // (deletes our window)
    engine = nullptr;
  }

  //==============================================================================
  void systemRequestedQuit() override
  {
    // This is called when the app is being asked to quit: you can ignore this
    // request and let the app carry on running, or call quit() to allow the app to close.
    quit();
  }

  void anotherInstanceStarted (const String& commandLine) override
  {
    // When another instance of the app is launched while this one is running,
    // this method is invoked, and the commandLine parameter tells you what
    // the other instance's command-line arguments were.

    DBG("Another instance started !");
    engine->parseCommandline(commandLine);


  }

  //==============================================================================
  /*
   This class implements the desktop window that contains an instance of
   our MainContentComponent class.
   */
  class MainWindow    : public DocumentWindow, public Timer
  {
  public:
    MainWindow (String name,Engine * e)  : DocumentWindow (name,
                                                           Colours::lightgrey,
                                                           DocumentWindow::allButtons)
    {
      startTimer(1000);

      setUsingNativeTitleBar (true);
      MainContentComponent * mainComponent = createMainContentComponent(e);
      setContentOwned (mainComponent, true);
      setResizable (true, true);


	  int tx = getAppProperties().getCommonSettings(true)->getIntValue("windowX");
	  int ty = getAppProperties().getCommonSettings(true)->getIntValue("windowY");
	  int tw = getAppProperties().getCommonSettings(true)->getIntValue("windowWidth");
	  int th = getAppProperties().getCommonSettings(true)->getIntValue("windowHeight");
	  bool fs = getAppProperties().getCommonSettings(true)->getBoolValue("fullscreen",true);

	 
      setBounds (jmax<int>(tx,20), jmax<int>(ty,20), jmax<int>(tw,100), jmax<int>(th,100));
	  setFullScreen(fs);

      setVisible (true);

#if ! JUCE_MAC
      setMenuBar(mainComponent);
#endif
    }

    void closeButtonPressed() override
    {
      // This is called when the user tries to close this window. Here, we'll just
      // ask the app to quit when this happens, but you can change this to do
      // whatever you need.

      //@martin added but commented for testing (relou behavior)
      int result = AlertWindow::showYesNoCancelBox(AlertWindow::QuestionIcon, "Save document", "Do you want to save the document before quitting ?");

       if (result == 0)  return; //prevent exit
       
	   if (result == 1)
	   {
		   juce::FileBasedDocument::SaveResult sr = ((LGMLApplication *)LGMLApplication::getInstance())->engine->save(true, true);
		   switch (sr)
		   {
		   case juce::FileBasedDocument::SaveResult::userCancelledSave:
		   case juce::FileBasedDocument::SaveResult::failedToWriteToFile:
				   return;

         case FileBasedDocument::SaveResult::savedOk:
           break;
		   }
	   }

	   var boundsVar = var(new DynamicObject());
	   Rectangle<int> r = getScreenBounds();

	   getAppProperties().getCommonSettings(true)->setValue("windowX",r.getPosition().x);
	   getAppProperties().getCommonSettings(true)->setValue("windowY", r.getPosition().y); 
	   getAppProperties().getCommonSettings(true)->setValue("windowWidth", r.getWidth()); 
	   getAppProperties().getCommonSettings(true)->setValue("windowHeight", r.getHeight());
	   getAppProperties().getCommonSettings(true)->setValue("fullscreen", isFullScreen());
	   getAppProperties().getCommonSettings(true)->saveIfNeeded();


       JUCEApplication::getInstance()->systemRequestedQuit();

    }

    void timerCallback() override;

    /* Note: Be careful if you override any DocumentWindow methods - the base
     class uses a lot of them, so by overriding you might break its functionality.
     It's best to do all your work in your content component instead, but if
     you really have to override any DocumentWindow methods, make sure your
     subclass also calls the superclass's method.
     */

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
  };

private:
		ScopedPointer<MainWindow> mainWindow;

};


static LGMLApplication& getApp()                 { return *dynamic_cast<LGMLApplication*>(JUCEApplication::getInstance()); }
ApplicationCommandManager& getCommandManager()      { return getApp().commandManager; }
ApplicationProperties& getAppProperties()           { return *getApp().appProperties; }
AudioDeviceManager & getAudioDeviceManager()        { return getApp().deviceManager;}
UndoManager & getAppUndoManager()                      { return getApp().undoManager;}
Engine & getEngine()                              { return *getApp().engine;}
bool  isEngineLoadingFile()                            {return getEngine().isLoadingFile;}
//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (LGMLApplication)

void LGMLApplication::MainWindow::timerCallback()
{
  File loadedFile = getApp().engine->getFile();
  String loadedName = "";
  if(loadedFile.existsAsFile()){
    loadedName =  loadedFile.getFileNameWithoutExtension() + " : ";
  }
  setName(loadedName+"LGML "+ String(ProjectInfo::versionString)+String(" (CPU : ")+String((int)(getAudioDeviceManager().getCpuUsage() * 100))+String("%)"));
}
