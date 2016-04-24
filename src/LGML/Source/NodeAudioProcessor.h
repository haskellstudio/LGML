/*
  ==============================================================================

    NodeAudioProcessor.h
    Created: 24 Apr 2016 1:32:40pm
    Author:  Martin Hermant

  ==============================================================================
*/

#ifndef NODEAUDIOPROCESSOR_H_INCLUDED
#define NODEAUDIOPROCESSOR_H_INCLUDED


#include "JuceHeader.h"

class NodeAudioProcessor : public juce::AudioProcessor,public AsyncUpdater
{
public:


    NodeAudioProcessor() :AudioProcessor(){};
    virtual ~NodeAudioProcessor(){};

    bool setPreferedNumAudioInput(int num);
    bool setPreferedNumAudioOutput(int num);


    virtual const String getName() const override { return "NodeBaseProcessor"; };

    virtual void prepareToPlay(double ,int) override{};
    virtual void releaseResources() override {};

    //bool silenceInProducesSilenceOut() const override { return false; }

    virtual AudioProcessorEditor* createEditor() override {return nullptr ;}
    virtual bool hasEditor() const override { return false; }



    // dumb overrides from JUCE AudioProcessor :  MIDI
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return "NoProgram"; }
    void changeProgramName(int, const String&) override {};
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    void numChannelsChanged()override{
        // int a = 0;
    };


    // save procedures from host
    virtual void getStateInformation(juce::MemoryBlock&) override {};
    virtual void setStateInformation(const void*, int) override {};


    virtual void processBlock(AudioBuffer<float>& buffer,MidiBuffer& midiMessages) override ;
    virtual void processBlockInternal(AudioBuffer<float>& buffer,MidiBuffer& midiMessages) = 0;


    void updateRMS(const AudioBuffer<float>& buffer);
    float alphaRMS = 0.05f;
    float rmsValue = 0.f;
    const int samplesBeforeRMSUpdate = 512;
    int curSamplesForRMSUpdate = 0;

    //Listener are called from non audio thread
    void handleAsyncUpdate() override{rmsListeners.call(&RMSListener::RMSChanged,rmsValue);}

    class  RMSListener
    {
    public:
        /** Destructor. */
        virtual ~RMSListener() {}
        virtual void RMSChanged(float ) = 0;

    };

    ListenerList<RMSListener> rmsListeners;
    void addRMSListener(RMSListener* newListener) { rmsListeners.add(newListener); }
    void removeRMSListener(RMSListener* listener) { rmsListeners.remove(listener); }

    class NodeAudioProcessorListener{
    public:
        virtual ~NodeAudioProcessorListener(){};
        virtual void numAudioInputChanged(int newNum){};
        virtual void numAudioOutputChanged(int newNum){};
    };
    ListenerList<NodeAudioProcessorListener> nodeAudioProcessorListeners;
    void addNodeAudioProcessorListener(NodeAudioProcessorListener* newListener) { nodeAudioProcessorListeners.add(newListener); }
    void removeNodeAudioProcessorListener(NodeAudioProcessorListener* listener) { nodeAudioProcessorListeners.remove(listener); }


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodeAudioProcessor)
};



#endif  // NODEAUDIOPROCESSOR_H_INCLUDED