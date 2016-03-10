/*
 ==============================================================================
 
 LooperNode.cpp
 Created: 3 Mar 2016 10:32:16pm
 Author:  bkupe
 
 ==============================================================================
 */

#include "LooperNode.h"
#include "TimeManager.h"

#include "LooperNodeUI.h"

LooperNode::LooperNode(NodeManager * nodeManager,uint32 nodeId) :NodeBase(nodeManager,nodeId,"Looper",new Looper(this)) {
    looper = dynamic_cast<Looper*>(audioProcessor);
    addChildControllableContainer(looper);
}


LooperNode::Looper::Looper(LooperNode * looperNode):
ControllableContainer("InnerLooper"),
selectedTrack(nullptr),
looperNode(looperNode)
{
    
    recPlaySelectedTrig =   addTrigger("Rec Or Play",
                                       "Tells the selected track to wait for the next bar \
                                       and then start record or play");
    
    playSelectedTrig =      addTrigger("Play",
                                       "Tells the selected track to wait for the next bar and \
                                       then stop recording and start playing");
    stopSelectedTrig =      addTrigger("Stop",
                                       "Tells the selected track to stop ");
    
    clearSelectedTrig =     addTrigger("Clear",
                                       "Tells the selected track to clear it's content if got any");
    
    volumeSelected =        addFloatParameter("Volume",
                                              "Set the volume of the selected track",
                                              1, 0, 1);
    
    
    clearAllTrig = addTrigger("ClearAll",
                              "Tells all tracks to clear it's content if got any");
    
    stopAllTrig = addTrigger("StopAll",
                             "Tells all tracks to stop it's content if got any");
    
    skipControllableNameInAddress = true;

    setNumTracks(8);
    recPlaySelectedTrig->addTriggerListener(this);
    playSelectedTrig->addTriggerListener(this);
    clearSelectedTrig->addTriggerListener(this);
    stopSelectedTrig->addTriggerListener(this);
    clearAllTrig->addTriggerListener(this);
    stopAllTrig->addTriggerListener(this);
}

void LooperNode::Looper::processBlockInternal(AudioBuffer<float>& buffer, MidiBuffer &midiMessages){
    
    // TODO check if we can optimize copies
    // handle multiples channels
    bufferIn.makeCopyOf(buffer);
    bufferOut.setSize(bufferIn.getNumChannels(),bufferIn.getNumSamples());
    bufferOut.clear();
    for( auto & t:tracks){
        t->processBlock(buffer,midiMessages);
        
        bufferOut.addFrom(0,0,buffer,0,0,buffer.getNumSamples());
        buffer.copyFrom(0,0,bufferIn,0,0,buffer.getNumSamples());
    }
    
    buffer.makeCopyOf( bufferOut);
    
    
}



void LooperNode::Looper::addTrack(){
    Track * t = new Track(this, tracks.size());
    tracks.add(t);
    addChildControllableContainer(t);
}

void LooperNode::Looper::removeTrack( int i){
    removeChildControllableContainer(tracks[i]);
    tracks.remove(i);
}


void LooperNode::Looper::setNumTracks(int numTracks){
    int oldSize = tracks.size();
    if(numTracks>oldSize)   { for(int i = oldSize ; i< numTracks ; i++)     {addTrack();}}
    else                    {for (int i = oldSize - 1; i > numTracks; --i)  {removeTrack(i);}}
    looperListeners.call(&Looper::Listener::trackNumChanged,numTracks);
}


void LooperNode::Looper::checkIfNeedGlobalLooperStateUpdate(){
    bool needToStop = true;
    bool needToReleaseMasterTempo = true;
    for(auto & t : tracks){
        needToStop &= (t->trackState == Track::STOPPED  ||t->trackState == Track::CLEARED  ) ;
        needToReleaseMasterTempo &= (t->trackState == Track::CLEARED );
    }
    
    if (needToReleaseMasterTempo) {
        TimeManager::getInstance()->removeIfMaster(looperNode);
    }
    if (needToStop) {
        TimeManager::getInstance()->stop();
    }
}


void LooperNode::Looper::triggerTriggered(Trigger * t){
    if(selectedTrack!=nullptr){
        if(t == recPlaySelectedTrig){
            selectedTrack->recPlayTrig->trigger();
        }else if(t == playSelectedTrig){
            selectedTrack->playTrig->trigger();
        }else if(t == clearSelectedTrig){
            selectedTrack->clearTrig->trigger();
        }else if(t == stopSelectedTrig){
            selectedTrack->stopTrig->trigger();
        }
    }
    if(t == clearAllTrig){
        for(auto & t:tracks){
            t->clearTrig->trigger();
        }
    }
    if(t == stopAllTrig){
        for(auto & t:tracks){
            t->stopTrig->trigger();
        }
    }
}
void LooperNode::Looper::selectMe(Track * t){
    if(selectedTrack!=nullptr ){
        selectedTrack->setSelected(false);
    }
    selectedTrack = t;
    if(selectedTrack!=nullptr ){
        selectedTrack->setSelected(true);
    }
    
    
}



/////////
// TRACK
////////




LooperNode::Looper::Track::Track(Looper * looper, int _trackNum) :
ControllableContainer("Track " + String(_trackNum)),
parentLooper(looper),
quantizedRecordStart(0),
quantizedRecordEnd(0),
quantizedPlayStart(0),
quantizedPlayEnd(0),
streamBipBuffer(16384),// 16000 ~ 300ms and 256*64
monoLoopSample(1,44100*MAX_LOOP_LENGTH_S),
trackState(CLEARED),
internalTrackState(BUFFER_STOPPED)
{
    
    //setCustomShortName("track/" + String(_trackNum)); //can't use "/" in shortName, will use ControllableIndexedContainer for that when ready.
    
    trackNum =      addIntParameter("Track Number",
                                    "Number of tracks",
                                    _trackNum, 0, MAX_NUM_TRACKS);
    
    recPlayTrig =   addTrigger("Rec Or Play",
                               "Tells the track to wait for the next bar \
                               and then start record or play");
    
    playTrig =      addTrigger("Play",
                               "Tells the track to wait for the next bar and \
                               then stop recording and start playing");
    stopTrig =     addTrigger("Stop",
                              "Tells the track to stop ");
    
    clearTrig =     addTrigger("Clear",
                               "Tells the track to clear it's content if got any");
    
    volume =        addFloatParameter("Volume",
                                      "Set the volume of the track",
                                      1, 0, 1);
    
    preDelayMs =    addIntParameter("Pre Delay MS",
                                    "Pre process delay (in milliseconds)",
                                    0, 0, 200);
    
    preDelayMs->isControllableExposed = false;
    
    recPlayTrig->addTriggerListener(this);
    playTrig->addTriggerListener(this);
    clearTrig->addTriggerListener(this);
    stopTrig->addTriggerListener(this);
    
    
    // post init
    volume->setValue(defaultVolumeValue);
}

void LooperNode::Looper::Track::processBlock(AudioBuffer<float>& buffer, MidiBuffer &midi){
    
    
    updatePendingLooperTrackState(TimeManager::getInstance()->timeInSample);
    
    
    // RECORDING
    if (internalTrackState == BUFFER_RECORDING )
    {
        if(recordNeedle + buffer.getNumSamples()> parentLooper->getSampleRate() * MAX_LOOP_LENGTH_S){
            setTrackState(STOPPED);
        }
        else{
            monoLoopSample.copyFrom(0, recordNeedle, buffer, 0, 0, buffer.getNumSamples());
            recordNeedle += buffer.getNumSamples();
        }
        
    }
    
    else{
        streamBipBuffer.writeBlock(buffer);
    }
    
    // PLAYING
    // allow circular reading , although not sure that overflow need to be handled as its written with same block sizes than read
    // we may need it if we start to use a different clock  than looperState in OOServer that has a granularity of blockSize
    // or if we dynamicly change blockSize
    if (internalTrackState==BUFFER_PLAYING && recordNeedle>0 && monoLoopSample.getNumSamples())
    {
        if ( (playNeedle + buffer.getNumSamples()) > recordNeedle)
        {
            
            //assert false for now see above
            //            jassert(false);
            int firstSegmentLength = recordNeedle - playNeedle;
            int secondSegmentLength = buffer.getNumSamples() - firstSegmentLength;
            buffer.copyFrom(0, 0, monoLoopSample, 0, playNeedle, firstSegmentLength);
            buffer.copyFrom(0, 0, monoLoopSample, 0, 0, secondSegmentLength);
            playNeedle = secondSegmentLength;
            
        }else{
            buffer.copyFrom(0, 0, monoLoopSample, 0, playNeedle, buffer.getNumSamples());
            playNeedle += buffer.getNumSamples();
            playNeedle %= recordNeedle;
        }
        buffer.applyGainRamp(0, 0, buffer.getNumSamples(), lastVolume,volume->value);
        lastVolume = volume->value;
        
        
    }
    else{
        // silence output buffer
        buffer.applyGain(0, 0, buffer.getNumSamples(), 0);
    }
    
    
}
void LooperNode::Looper::Track::updatePendingLooperTrackState(uint64 curTime){
    
    if(quantizedRecordStart>0){
        if(curTime>quantizedRecordStart){
            preDelayMs->setValue( 0);
            setTrackState(RECORDING);
        }
        
    }
    else if( quantizedRecordEnd>0){
        if(curTime>quantizedRecordEnd){
            preDelayMs->setValue(0);
            setTrackState(PLAYING);
            
        }
    }
    
    
    
    if(quantizedPlayStart>0){
        if(curTime>quantizedPlayStart){
            setTrackState(PLAYING);
        }
    }
    else if( quantizedPlayEnd>0){
        if(curTime>quantizedPlayEnd){
            setTrackState(STOPPED);
        }
    }
    
    
}




void LooperNode::Looper::Track::triggerTriggered(Trigger * t){
    if(t == recPlayTrig){
        if(trackState == CLEARED || trackState == STOPPED){
            setTrackState(SHOULD_RECORD);
        }
        else{
            setTrackState(SHOULD_PLAY);
        }
    }
    else if(t == playTrig){
        setTrackState(SHOULD_PLAY);
    }
    else if(t== clearTrig){
        setTrackState(SHOULD_CLEAR);
    }
    else if (t == stopTrig){
        setTrackState(STOPPED);
    }
}

bool LooperNode::Looper::Track::isMasterTempoTrack(){
    return TimeManager::getInstance()->askForBeingMasterNode(parentLooper->looperNode)
    && parentLooper->askForBeingMasterTrack(this);
}
void LooperNode::Looper::Track::setSelected(bool isSelected){
    trackStateListeners.call(&LooperNode::Looper::Track::Listener::trackSelected,isSelected);
}
void LooperNode::Looper::Track::askForSelection(bool isSelected){
    parentLooper->selectMe(isSelected?this:nullptr);
}


void LooperNode::Looper::Track::setTrackState(TrackState newState){
    
    
    
    
    
    // quantify
    if(newState == SHOULD_RECORD){
        // are we able to set the tempo
        if( isMasterTempoTrack()){
            TimeManager::getInstance()->stop();
            TimeManager::getInstance()->setPlayState(true);
            newState = RECORDING;
            quantizedRecordStart = -1;
        }
        else{
            quantizedRecordStart =TimeManager::getInstance()->getNextQuantifiedTime();
        }
    }
    
    
    
    
    // on true start recording
    if(newState == RECORDING){
        internalTrackState = BUFFER_RECORDING;
        quantizedRecordStart = -1;
        if(preDelayMs->value>0){
            monoLoopSample.copyFrom(0,0,streamBipBuffer.getLastBlock(preDelayMs->value),preDelayMs->value);
            recordNeedle = preDelayMs->value;
        }
        else{
            preDelayMs->setValue( 0);
            recordNeedle = 0;
        }
    }
    // on true end recording
    else if(trackState == RECORDING && newState==SHOULD_PLAY){
        {
            
            if( isMasterTempoTrack()){
                recordNeedle-=preDelayMs->value;
                // 22 ms if 44100
                int fadeNumSaples = 10;
                if(recordNeedle>2*fadeNumSaples){
                    monoLoopSample.applyGainRamp(0, 0, fadeNumSaples, 0, 1);
                    monoLoopSample.applyGainRamp(0,recordNeedle - fadeNumSaples, fadeNumSaples, 1, 0);
                }
                
                quantizedRecordEnd = -1;
                
                TimeManager::getInstance()->setBPMForLoopLength(recordNeedle);
                newState = PLAYING;
            }
            else{
                quantizedRecordEnd = TimeManager::getInstance()->getNextQuantifiedTime();
            }
        }
    }
    
    // on ask for play
    if(newState ==SHOULD_PLAY){
        quantizedRecordEnd = -1;
        quantizedRecordStart = -1;
        quantizedPlayStart = TimeManager::getInstance()->getNextQuantifiedTime();
    }
    // on true start of play
    else if(newState ==PLAYING){
        internalTrackState = BUFFER_PLAYING;
        quantizedRecordEnd = -1;
        quantizedPlayStart = -1;
        playNeedle = 0;
        
    }
    // on true end of play
    else if (trackState== PLAYING && newState!=PLAYING){
        quantizedPlayEnd = -1;
    }
    // on should clear
    if(newState == SHOULD_CLEAR){
        recordNeedle = 0;
        playNeedle = 0;
        quantizedPlayEnd = -1;
        quantizedPlayStart = -1;
        quantizedRecordEnd = -1;
        quantizedRecordStart = -1;
        volume->setValue(defaultVolumeValue);
        newState = CLEARED;
        internalTrackState = BUFFER_STOPPED;
        
        
    }
    
    
    if(newState == STOPPED){
        internalTrackState = BUFFER_STOPPED;
        // force a track to stay in cleared state if stop triggered
        if(trackState == CLEARED){
            newState = CLEARED;
        }
    }
    //DBG(newState <<","<<trackState );
    
    trackState = newState;
    parentLooper->checkIfNeedGlobalLooperStateUpdate();
    trackStateListeners.call(&LooperNode::Looper::Track::Listener::internalTrackStateChanged,trackState);
};




NodeBaseUI * LooperNode::createUI(){return new NodeBaseUI(this, new LooperNodeUI);}