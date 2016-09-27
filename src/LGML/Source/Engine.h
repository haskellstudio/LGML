/*
 ==============================================================================

 Engine.h
 Created: 2 Apr 2016 11:03:21am
 Author:  Martin Hermant

 ==============================================================================
 */

#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED



#include "ControlManager.h"
#include "NodeManager.h"
#include "PresetManager.h"
#include "FastMapper.h"
#include "AudioFucker.h"
#include "TimeManager.h"
#include "RuleManager.h"
#include "VSTManager.h"

class Engine:public FileBasedDocument{
public:
    Engine();
    ~Engine();

    // Audio
    AudioProcessorPlayer graphPlayer;

    void createNewGraph();
    void clear();
    void initAudio();
    void closeAudio();


    void suspendAudio(bool);

    void parseCommandline(const String & );

    //==============================================================================
    // see EngineFileDocument.cpp

    //  inherited from FileBasedDocument
    String getDocumentTitle()override ;
    Result loadDocument (const File& file)override;
    Result saveDocument (const File& file)override;
    File getLastDocumentOpened() override;
    void setLastDocumentOpened (const File& file) override;

    //    #if JUCE_MODAL_LOOPS_PERMITTED
    //     File getSuggestedSaveAsFile (const File& defaultFile)override;
    //    #endif

    // our Saving methods
    var getJSONData();
    void loadJSONData(var data);

    bool checkFileVersion(DynamicObject * metaData);
    int versionStringToInt(const String &version);
    String getMinimumRequiredFileVersion();

    void  stimulateAudio(bool);
    ScopedPointer<AudioFucker> stimulator;

    class MultipleAudioSettingsHandler : public ChangeListener,public Timer{
    public:
        MultipleAudioSettingsHandler():oldSettingsId("oldAudioSettings"){}
        Identifier oldSettingsId;
        void changeListenerCallback(ChangeBroadcaster * )override;
        void saveCurrent();
        String getConfigName();
        void load();
        String lastConfigName;
        void timerCallback()override;

    };
    MultipleAudioSettingsHandler audioSettingsHandler;


    
};


static String lastFileListKey = "recentNodeGraphFiles";


#endif  // ENGINE_H_INCLUDED
