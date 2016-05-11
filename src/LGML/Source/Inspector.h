/*
  ==============================================================================

    Inspector.h
    Created: 9 May 2016 6:41:38pm
    Author:  bkupe

  ==============================================================================
*/

#ifndef INSPECTOR_H_INCLUDED
#define INSPECTOR_H_INCLUDED

#include "ShapeShifterContent.h"
#include "InspectableComponent.h"
#include "InspectorEditor.h"

class Inspector : public ShapeShifterContent, public InspectableComponent::InspectableListener
{
public:
	Inspector();
	virtual ~Inspector();

	InspectableComponent * currentComponent;

	ScopedPointer<InspectorEditor> currentEditor;

	bool isEnabled;
	void setEnabled(bool value);

	void setCurrentComponent(InspectableComponent * component);

	void resized() override;

	void clear();
	void inspectCurrentComponent();

	void inspectableRemoved(InspectableComponent * component);
};


#endif  // INSPECTOR_H_INCLUDED
