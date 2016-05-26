/*
  ==============================================================================

    AudioDeviceInNodeUI.h
    Created: 25 May 2016 9:31:12am
    Author:  bkupe

  ==============================================================================
*/

#ifndef AUDIODEVICEINNODEUI_H_INCLUDED
#define AUDIODEVICEINNODEUI_H_INCLUDED

#include "NodeBaseContentUI.h"
#include "NodeBase.h"

class VuMeter;
class AudioDeviceInNode;

class AudioDeviceInNodeContentUI :
	public NodeBaseContentUI,
	public NodeBase::NodeBaseListener,
	public ConnectableNode::ConnectableNodeListener
{
public:
	AudioDeviceInNodeContentUI();
	virtual ~AudioDeviceInNodeContentUI();

	OwnedArray<BoolToggleUI> muteToggles;
	OwnedArray<VuMeter> vuMeters;

	AudioDeviceInNode * audioInNode;

	void init() override;

	void resized() override;

	void updateVuMeters();

	void addVuMeter();
	void removeLastVuMeter();

	virtual void nodeParameterChanged(ConnectableNode*, Parameter *) override;

	virtual void numAudioOutputChanged(NodeBase *, int newNum) override;
	//virtual void numAudioOutputChanged() { DBG("Output changed !"); }

};

#endif  // AUDIODEVICEINNODEUI_H_INCLUDED