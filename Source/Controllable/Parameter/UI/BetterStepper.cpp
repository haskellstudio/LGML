/* Copyright © Organic Orchestra, 2017
*
* This file is part of LGML.  LGML is a software to manipulate sound in realtime
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation (version 3 of the License).
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*/


#include "BetterStepper.h"

BetterStepper::BetterStepper (TooltipClient* tooltip) : Slider (SliderStyle::IncDecButtons, TextEntryBoxPosition::TextBoxLeft)
,tooltipClient(tooltip)
{

    setIncDecButtonsMode (IncDecButtonMode::incDecButtonsDraggable_AutoDirection);
    
    setScrollWheelEnabled (true);


}

BetterStepper::~BetterStepper()
{
}

String BetterStepper::getTooltip() {
    if (tooltipClient)
        return tooltipClient->getTooltip();
    return "";

}

void BetterStepper::resized()
{
    Slider::resized();
    isMini = getWidth() < 30;
    bool isWider = getWidth()>getHeight();
    auto linearStyle = isWider?SliderStyle::LinearBar:SliderStyle::LinearBarVertical;

    if (isMini)
    {
        setSliderStyle (linearStyle);
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    }
    else
    {
        setSliderStyle (isEditable ? SliderStyle::IncDecButtons:linearStyle);
        setColour (juce::Slider::textBoxBackgroundColourId, !isEditable?Colours::black:juce::Colours::white.withAlpha (0.1f));
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.7f));
        //    setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
    }

}


void BetterStepper::setEditable(bool s){
    setTextBoxIsEditable (s);
    setEnabled ( s);

    isEditable = s;
    resized();
}

void BetterStepper::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel) {
    Viewport *vp =findParentComponentOfClass<Viewport>();
    bool _scrollWheelAllowed = !vp  || !(vp->canScrollVertically() || vp->canScrollHorizontally());
    if(_scrollWheelAllowed){
        Slider::mouseWheelMove(e, wheel);
    }
    else{
        Component::mouseWheelMove(e, wheel);
    }

}

void BetterStepper::mouseEnter(const MouseEvent& e){
    //update  tooltip of inner Slider component
    lookAndFeelChanged();
};
