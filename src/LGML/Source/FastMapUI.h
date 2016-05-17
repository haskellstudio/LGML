/*
  ==============================================================================

    FastMapUI.h
    Created: 17 May 2016 6:05:44pm
    Author:  bkupe

  ==============================================================================
*/

#ifndef FASTMAPUI_H_INCLUDED
#define FASTMAPUI_H_INCLUDED

#include "FastMap.h"
#include "ControlVariableReferenceUI.h"
#include "ControllableHelpers.h"
#include "FloatSliderUI.h"



class FastMapUI : 
	public Component,
	public FastMapListener,
	public  ControllableChooser::Listener
{
public:
	FastMapUI(FastMap * f);
	virtual ~FastMapUI();

	ScopedPointer<ControlVariableReferenceUI> refUI;

	ScopedPointer<FloatSliderUI> minInputUI;
	ScopedPointer<FloatSliderUI> maxInputUI;
	ScopedPointer<FloatSliderUI> minOutputUI;
	ScopedPointer<FloatSliderUI> maxOutputUI;

	ControllableChooser chooseTargetBT;

	FastMap * fastMap;

	void resized();

	virtual void choosedControllableChanged(Controllable *) override;
		
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FastMapUI)

};




#endif  // FASTMAPUI_H_INCLUDED
