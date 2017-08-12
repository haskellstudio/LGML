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


#include "ControllableContainer.h"


#include "ControllableUI.h"

#include "DebugHelpers.h"
#include "StringUtil.hpp"
#include "JsHelpers.h"
#include "ParameterFactory.h"


const Identifier ControllableContainer::presetIdentifier("preset");
const Identifier ControllableContainer::paramsIdentifier("parameters");

const Identifier ControllableContainer::containerNameIdentifier("containerName");

const Identifier ControllableContainer::controlAddressIdentifier("controlAddress");


const Identifier ControllableContainer::childContainerId("/");


ControllableContainer::ControllableContainer(const String & niceName,bool _isUserDefined) :
parentContainer(nullptr),
hasCustomShortName(false),
skipControllableNameInAddress(false),
currentPreset(nullptr),
canHavePresets(true),
numContainerIndexed(0),
localIndexedPosition(-1),
presetSavingIsRecursive(false),
isUserDefined(false)
{

  nameParam = addNewParameter<StringParameter>("Name", "Set the visible name of the node.", "");
  nameParam->isPresettable = false;
  nameParam->isEditable = _isUserDefined;
  nameParam->setValue(niceName);
  currentPresetName = addNewParameter<StringParameter>("Preset", "Current Preset", "");
  currentPresetName->hideInEditor = true;
  savePresetTrigger = addNewParameter<Trigger>("Save Preset", "Save current preset");
  savePresetTrigger->hideInEditor = true;
  isUserDefined = _isUserDefined;

}

ControllableContainer::~ControllableContainer()
{
  //controllables.clear();
  //DBG("CLEAR CONTROLLABLE CONTAINER");
  
  // manage memory if not cleared
  auto all = getAllControllableContainers(true);
  for(auto &a : all){
    if (a.get())delete a.get();
  }

  
  clear();
  masterReference.clear();


}
void ControllableContainer::clear()
{
  cleanUpPresets();
  while(controllables.size()){
  removeControllable(controllables[0]);
  }
  controllableContainers.clear();
}

void ControllableContainer::remove(){
  jassert(parentContainer);
  if(parentContainer){
    parentContainer->removeChildControllableContainer(this);
  }
}
Parameter *  ControllableContainer::addParameter(Parameter * p)
{

  p->setParentContainer(this);
  controllables.add(p);
  controllableContainerListeners.call(&ControllableContainerListener::controllableAdded,this, p);
  notifyStructureChanged(this);
  addControllableInternal(p);
  p->addParameterListener(this);
  p->addAsyncParameterListener(this);
  p->isUserDefined = isUserDefined;
  return p;


}



void ControllableContainer::removeControllable(Controllable * c)
{
  controllableContainerListeners.call(&ControllableContainerListener::controllableRemoved,this, c);

  if(Parameter * p = dynamic_cast<Parameter*>(c)){
    p->removeParameterListener(this);
    p->removeAsyncParameterListener(this);
  }
  controllables.removeObject(c);
  notifyStructureChanged(this);
}


void ControllableContainer::notifyStructureChanged(ControllableContainer * origin){

  controllableContainerListeners.call(&ControllableContainerListener::childStructureChanged, this,origin);
}

void ControllableContainer::newMessage(const Parameter::ParamWithValue& pv){
  if(pv.parameter == currentPresetName){
    loadPresetWithName(pv.parameter->stringValue());
  }
  if(!pv.isRange()){
    onContainerParameterChangedAsync(pv.parameter, pv.value);
  }
}
void ControllableContainer::setNiceName(const String &_niceName) {
  String targetName (_niceName);
  if(parentContainer){
    targetName = parentContainer->getUniqueNameInContainer(_niceName,0,this);
  }
  
  nameParam->setValue(targetName,targetName==getNiceName(),true);

}
const String  ControllableContainer::getNiceName(){
  return nameParam->stringValue();
}

void ControllableContainer::setCustomShortName(const String &_shortName){
  shortName = _shortName;
  hasCustomShortName = true;
  updateChildrenControlAddress();
  controllableContainerListeners.call(&ControllableContainerListener::childAddressChanged,this);
}

void ControllableContainer::setAutoShortName() {
  hasCustomShortName = false;
  shortName = StringUtil::toShortName(getNiceName());
  updateChildrenControlAddress();
  controllableContainerListeners.call(&ControllableContainerListener::childAddressChanged,this);
}



Controllable * ControllableContainer::getControllableByName(const String & name, bool searchNiceNameToo)
{
  for (auto &c : controllables)
  {
    if (c->shortName == name || (searchNiceNameToo && c->niceName == name)) return c;
  }

  return nullptr;
}

ControllableContainer* ControllableContainer::addChildControllableContainer(ControllableContainer * container)
{
  String oriName = container->getNiceName();
  String targetName = getUniqueNameInContainer(oriName);
  if(targetName!=oriName){
    container->setNiceName(targetName);
  }
  controllableContainers.add(container);
  container->addControllableContainerListener(this);
  container->setParentContainer(this);
  controllableContainerListeners.call(&ControllableContainerListener::controllableContainerAdded,this, container);
  notifyStructureChanged(this);
  return container;
}

void ControllableContainer::removeChildControllableContainer(ControllableContainer * container)
{
  if(numContainerIndexed>0 &&
     container->localIndexedPosition>=0 &&
     controllableContainers.getUnchecked(container->localIndexedPosition) == container){
    numContainerIndexed--;
  }
  controllableContainerListeners.call(&ControllableContainerListener::controllableContainerRemoved,this, container);
  controllableContainers.removeAllInstancesOf(container);

  container->removeControllableContainerListener(this);
  notifyStructureChanged(this);
  container->setParentContainer(nullptr);
}

void ControllableContainer::addChildIndexedControllableContainer(ControllableContainer * container,int idx){
  if(idx == -1 )idx = numContainerIndexed;
  jassert(idx<=numContainerIndexed);

  controllableContainers.insert(idx, container);
  container->localIndexedPosition = idx;
  numContainerIndexed++;

  container->addControllableContainerListener(this);
  container->setParentContainer(this);
  controllableContainerListeners.call(&ControllableContainerListener::controllableContainerAdded,this, container);
  notifyStructureChanged(this);
}

void ControllableContainer::removeChildIndexedControllableContainer(int idx){
  if(idx == -1 )idx = numContainerIndexed-1;
  jassert(idx<numContainerIndexed);


  removeChildControllableContainer(controllableContainers.getUnchecked(idx));
  numContainerIndexed--;

  for(int i = idx ; i < numContainerIndexed ; i ++){
    controllableContainers.getUnchecked(i)->localIndexedPosition = i;
    controllableContainers.getUnchecked(i)->localIndexChanged();
  }

}

int ControllableContainer::getNumberOfIndexedContainer(){return numContainerIndexed;}
int ControllableContainer::getIndexedPosition(){return localIndexedPosition;}
bool ControllableContainer::hasIndexedContainers(){return numContainerIndexed>0;}
bool ControllableContainer::isIndexedContainer(){return localIndexedPosition>=0;}
void ControllableContainer::localIndexChanged(){};

ControllableContainer * ControllableContainer::getControllableContainerByName(const String & name, bool searchNiceNameToo)
{
  ScopedLock lk(controllableContainers.getLock());
  for (auto &cc : controllableContainers)
  {
    if (cc.get() && (cc->shortName == name || (searchNiceNameToo && cc->getNiceName() == name))) return cc;
  }

  return nullptr;

}

ControllableContainer * ControllableContainer::getControllableContainerForAddress( StringArray  addressSplit)
{

  if (addressSplit.size() == 0) jassertfalse; // SHOULD NEVER BE THERE !

  bool isTargetAControllable = addressSplit.size() == 1;

  if (isTargetAControllable)
  {

    if(ControllableContainer * res = getControllableContainerByName(addressSplit[0]))
      return res;

    //no found in direct children Container, maybe in a skip container ?
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers)
    {
      if (cc->skipControllableNameInAddress)
      {
        if (ControllableContainer * res = cc->getControllableContainerForAddress(addressSplit)) return res;
      }
    }
  }
  else
  {
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers)
    {

      if (!cc->skipControllableNameInAddress)
      {
        if (cc->shortName == addressSplit[0])
        {
          addressSplit.remove(0);
          return cc->getControllableContainerForAddress(addressSplit);
        }
      }
      else
      {
        ControllableContainer * tc = cc->getControllableContainerByName(addressSplit[0]);
        if (tc != nullptr)
        {
          addressSplit.remove(0);
          return tc->getControllableContainerForAddress(addressSplit);
        }

      }
    }
  }

  return nullptr;

}

String ControllableContainer::getControlAddress(ControllableContainer * relativeTo){
  StringArray addressArray;
  ControllableContainer * pc = this;
  while (pc != relativeTo && pc != nullptr)
  {
    if(!pc->skipControllableNameInAddress) addressArray.insert(0, pc->shortName);
    pc = pc->parentContainer;
  }
  if(addressArray.size()==0)return "/";
  else return "/" + addressArray.joinIntoString("/");
}

void ControllableContainer::setParentContainer(ControllableContainer * container)
{
  this->parentContainer = container;
  updateChildrenControlAddress();

}

void ControllableContainer::updateChildrenControlAddress()
{
  {
    ScopedLock lk(controllables.getLock());
    for (auto &c : controllables) c->updateControlAddress();
  }
  {
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers) if(cc.get())cc->updateChildrenControlAddress();
  }


}

Array<WeakReference<Controllable>> ControllableContainer::getAllControllables(bool recursive,bool getNotExposed)
{

  Array<WeakReference<Controllable>> result;
  {
    ScopedLock lk(controllables.getLock());
    for (auto &c : controllables)
    {
      if (getNotExposed || c->isControllableExposed) result.add(c);
    }
  }

  if (recursive)
  {
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers) if(cc.get())result.addArray(cc->getAllControllables(true,getNotExposed));
  }

  return result;
}

Array<WeakReference<ControllableContainer>> ControllableContainer::getAllControllableContainers(bool recursive)
{
  Array<WeakReference<ControllableContainer>> containers;
  containers.addArray(controllableContainers);
  if (!recursive){
    return containers;
  }

  {
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers) if(cc.get())containers.addArray(cc->getAllControllableContainers(true));

  }
  return containers;

}

Array<WeakReference<Parameter>> ControllableContainer::getAllParameters(bool recursive, bool getNotExposed)
{
  Array<WeakReference<Parameter>> result;
  for (auto &c : controllables)
  {
    if (c->type == Controllable::Type::TRIGGER) continue;
    if (getNotExposed || c->isControllableExposed){
      if(Parameter * cc = dynamic_cast<Parameter*>(c)){
        result.add(cc);
      }
    }
  }

  if (recursive)
  {
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers) if(cc.get())result.addArray(cc->getAllParameters(true, getNotExposed));
  }

  return result;
}



Controllable * ControllableContainer::getControllableForAddress(String address, bool recursive, bool getNotExposed)
{
  StringArray addrArray;
  addrArray.addTokens(address, juce::StringRef("/"), juce::StringRef("\""));
  addrArray.remove(0);

  return getControllableForAddress(addrArray, recursive, getNotExposed);
}

Controllable * ControllableContainer::getControllableForAddress(StringArray addressSplit, bool recursive, bool getNotExposed)
{
  if (addressSplit.size() == 0) jassertfalse; // SHOULD NEVER BE THERE !

  bool isTargetAControllable = addressSplit.size() == 1;

  if (isTargetAControllable)
  {
    {
      //DBG("Check controllable Address : " + shortName);
      const ScopedLock lk(controllables.getLock());
      for (auto &c : controllables)
      {
        if (c->shortName == addressSplit[0])
        {
          //DBG(c->shortName);
          if (c->isControllableExposed || getNotExposed) return c;
          else return nullptr;
        }
      }
    }
    {
      //no found in direct children controllables, maybe in a skip container ?
      ScopedLock lk(controllableContainers.getLock());
      for (auto &cc : controllableContainers)
      {if(cc.get()){
        if (cc->skipControllableNameInAddress)
        {
          Controllable * tc = cc->getControllableByName(addressSplit[0]);

          if (tc != nullptr) return tc;
        }
      }
      }
    }
  }
  else
  {
    ScopedLock lk(controllableContainers.getLock());
    for (auto &cc : controllableContainers)
    {
      if(cc.get()){
        if (!cc->skipControllableNameInAddress)
        {
          if (cc->shortName == addressSplit[0])
          {
            addressSplit.remove(0);
            return cc->getControllableForAddress(addressSplit,recursive,getNotExposed);
          }
        }
        else
        {
          ControllableContainer * tc = cc->getControllableContainerByName(addressSplit[0]);
          if (tc != nullptr)
          {
            addressSplit.remove(0);
            return tc->getControllableForAddress(addressSplit,recursive,getNotExposed);
          }

        }
      }
    }
  }

  return nullptr;
}

bool ControllableContainer::containsControllable(Controllable * c, int maxSearchLevels)
{
  if (c == nullptr) return false;

  ControllableContainer * pc = c->parentContainer;
  if (pc == nullptr) return false;
  int curLevel = 0;
  while (pc != nullptr)
  {
    if (pc == this) return true;
    curLevel++;
    if (maxSearchLevels >= 0 && curLevel > maxSearchLevels) return false;
    pc = pc->parentContainer;
  }

  return false;
}


bool ControllableContainer::loadPresetWithName(const String & name)
{
  // TODO weird feedback when loading preset on parameter presetName
  if(isLoadingPreset){return false;}
  if(name=="") return false;
  isLoadingPreset = true;

  PresetManager::Preset * preset = PresetManager::getInstance()->getPreset(getPresetFilter(), name);
  if (preset == nullptr){isLoadingPreset = false;currentPresetName->setValue("", true,true); return false;}
  bool hasLoaded = loadPreset(preset);
  isLoadingPreset = false;
  return hasLoaded;

}

bool ControllableContainer::loadPreset(PresetManager::Preset * preset)
{
  if (preset == nullptr){
    currentPresetName->setValue("", true);
    return false;
  }

  loadPresetInternal(preset);

  for (auto &pv : preset->presetValues)
  {

    Parameter * p = dynamic_cast<Parameter *>(getControllableForAddress(pv->paramControlAddress));
    //DBG("Load preset, param set container : " << niceName << ", niceName : " << p->niceName << ",pv controlAddress : " << p->controlAddress << "" << pv->presetValue.toString());
    if (p != nullptr && p != currentPresetName) p->setValue(pv->presetValue);
  }

  currentPreset = preset;
  currentPresetName->setValue(currentPreset->name, false);

  controllableContainerListeners.call(&ControllableContainerListener::controllableContainerPresetLoaded, this);
  return true;
}

PresetManager::Preset* ControllableContainer::saveNewPreset(const String & _name)
{
  PresetManager::Preset * pre = PresetManager::getInstance()->addPresetFromControllableContainer(_name, getPresetFilter(), this, presetSavingIsRecursive);
  savePresetInternal(pre);
  NLOG(getNiceName(), "New preset saved : " + pre->name);
  loadPreset(pre);
  return pre;
}


bool ControllableContainer::saveCurrentPreset()
{
  //Same as saveNewPreset because PresetManager now replaces if name is the same
  if (currentPreset == nullptr) {
    jassertfalse;
    return false;
  }

  PresetManager::Preset * pre = PresetManager::getInstance()->addPresetFromControllableContainer(currentPreset->name, getPresetFilter(), this, presetSavingIsRecursive);
  savePresetInternal(pre);
  NLOG(getNiceName(), "Current preset saved : " + pre->name);
  return loadPreset(pre);

  /*
   for (auto &pv : currentPreset->presetValues)
   {
   Parameter * p = dynamic_cast<Parameter*> (getControllableForAddress(pv->paramControlAddress));
   if (p != nullptr && p!=currentPresetName)
   {
   pv->presetValue = var(p->value);
   }
   }
   savePresetInternal(currentPreset);
   NLOG(niceName, "Current preset saved : " + currentPreset->name);

   return true;
   */
}

int ControllableContainer::getNumPresets()
{
  return PresetManager::getInstance()->getNumPresetForFilter(getPresetFilter());
}

bool ControllableContainer::resetFromPreset()
{
  if (currentPreset == nullptr) return false;


  for (auto &pv : currentPreset->presetValues)
  {
    Parameter * p = (Parameter *)getControllableForAddress(pv->paramControlAddress);
    if (p != nullptr && p !=currentPresetName) p->resetValue();
  }


  currentPreset = nullptr;
  currentPresetName->setValue("", true);

  return true;
}

var ControllableContainer::getPresetValueFor(Parameter * p)
{
  if (currentPreset == nullptr) return var();
  return currentPreset->getPresetValue(p->getControlAddress(this));
}


void ControllableContainer::cleanUpPresets()
{
  PresetManager * pm = PresetManager::getInstanceWithoutCreating();
  if (pm != nullptr) pm->deletePresetsForContainer(this, true);

}

String ControllableContainer::getPresetFilter()
{
  return shortName;
}


void ControllableContainer::dispatchFeedback(Controllable * c)
{
  
  if (parentContainer != nullptr){ parentContainer->dispatchFeedback(c); }
  controllableContainerListeners.call(&ControllableContainerListener::controllableFeedbackUpdate,this, c);

}



void ControllableContainer::parameterValueChanged(Parameter * p)
{
  if (p == nameParam)
  {
    if(parentContainer){
      String oN = nameParam->stringValue();
      String tN = parentContainer->getUniqueNameInContainer(oN,0,this);
      if(tN!=oN){
        nameParam->setValue(tN,false,true);
      }
    }
    if (!hasCustomShortName) setAutoShortName();
  }
  else   if (p == savePresetTrigger)
  {
    saveCurrentPreset();

  }

  if(p && p->type==Controllable::TRIGGER){
    onContainerTriggerTriggered((Trigger*)p);
  }
  else{
    onContainerParameterChanged(p);
  }

  if ( (p!=nullptr && p->parentContainer==this && p->isControllableExposed ) ) dispatchFeedback(p);
}






var ControllableContainer::getJSONData()
{
  DynamicObject * data = new DynamicObject();
  data->setProperty(containerNameIdentifier, getNiceName());
  data->setProperty("uid",uid.toString());
  {
    var paramsData(new DynamicObject);

    for(auto & c :controllables){
        if(c->isUserDefined || c->shouldSaveObject  ){
          paramsData.getDynamicObject()->setProperty(c->shortName,ParameterFactory::getVarObjectFromControllable(c));
        }
        else if (c->isSavable){
          paramsData.getDynamicObject()->setProperty(c->shortName,c->getVarState());
        }
      }
    

    data->setProperty(paramsIdentifier, paramsData);

  }


  if(controllableContainers.size()){
    DynamicObject *  childData = new DynamicObject();

    for(auto controllableCont: controllableContainers){
      childData->setProperty(controllableCont->shortName,controllableCont.get()->getJSONData());
    }
    data->setProperty(childContainerId, childData);
  }



  return data;
}

void ControllableContainer::loadJSONData(const var & data)
{
  auto dyn =data.getDynamicObject();
  if (dyn->hasProperty("uid")) uid = dyn->getProperty("uid");
  if (dyn->hasProperty(containerNameIdentifier))
  {
    String  name =dyn->getProperty(containerNameIdentifier);
    setNiceName(name);
  }
  {
    DynamicObject * paramsData = dyn->getProperty(paramsIdentifier).getDynamicObject();
    jassert(paramsData);
    if (paramsData != nullptr)
    {
      auto props = paramsData->getProperties();
      for (auto & p:props)
      {
        if(Controllable * c = getControllableByName(p.name.toString(),true)){
          if(c->isSavable){
            if (Parameter * par = dynamic_cast<Parameter*>(c)) {
              // we don't load preset when already loading a state
              if (par->shortName != presetIdentifier.toString() ){
                par->setValue(p.value);
              }
            }
            else {
              // we don't use custom types for now
              jassertfalse;
              loadCustomJSONElement(p.name,p.value);
            }
          }
        }
        else {
          addParameterFromVar(p.name.toString(),p.value);
        }

      }
    }
  }

  {


    auto cD = dyn->getProperty(childContainerId).getDynamicObject();
    if(cD){
      auto ob = cD->getProperties();

      for(auto & o: ob){
        auto cont = getControllableContainerByName(o.name.toString());
        if(cont){
          cont->loadJSONData(o.value);
        }
        else{
          auto c = addContainerFromVar(o.name.toString(),o.value);
          if(c){
            c->loadJSONData(o.value);
          }
          else{
            jassertfalse;
          }
        }
      }

    }
  }
  

  


}

void ControllableContainer::childStructureChanged(ControllableContainer * /*notifier*/,ControllableContainer *origin)
{
  notifyStructureChanged(origin);
}

String ControllableContainer::getUniqueNameInContainer(const String & sourceName, int suffix,void * me)
{
  String resultName = sourceName;
  if (suffix > 0)
  {
    StringArray sa;
    sa.addTokens(resultName,false);
    if (sa.size() > 1 && (sa[sa.size()-1].getIntValue() != 0 || sa[sa.size()-1].containsOnly("0")))
    {
      int num = sa[sa.size() - 1].getIntValue() + suffix;
      sa.remove(sa.size() - 1);
      sa.add(String(num));
      resultName = sa.joinIntoString(" ");
    } else
    {
      resultName += " " + String(suffix);
    }
  }
  void * elem = getControllableByName(resultName,true);
  if ( elem!=nullptr && elem != me)
  {
    return getUniqueNameInContainer(sourceName, suffix + 1,me);
  }
  elem = getControllableContainerByName(resultName,true) ;
  if (elem!=nullptr && elem != me)
  {
    return getUniqueNameInContainer(sourceName, suffix + 1,me);
  }

  return resultName;
}

template<class T>
Array<T*> ControllableContainer::getObjectsOfType(bool recursive){
  Array<T*> res;

  for(auto & c:controllables){
    if(T* o = dynamic_cast<T*>(c)){
      res.add(o);
    }
  }
  for(auto & c:controllableContainers){
    if(T* o = dynamic_cast<T*>(c)){
      res.add(o);
    }
    if(recursive){res.addArray(c->getObjectsOfType<T>(recursive));}
  }

  return res;
}

bool ControllableContainer::containsContainer(ControllableContainer * c){
  if(c==this)return true;
  ScopedLock lk(controllableContainers.getLock());
  for(auto & cc:controllableContainers){
    if(c==cc){return true;}
    if(cc->containsContainer(c))return true;
  }
  return false;
}


 ControllableContainer *  ControllableContainer::addContainerFromVar(const String & name,const var & data) {
   return addChildControllableContainer(new ControllableContainer(name,true));
 };
 Parameter* ControllableContainer::addParameterFromVar(const String & name,const var & data) {
   // handle automagically for userdefined
   jassert(isUserDefined);

   return addParameter(ParameterFactory::createFromVarObject(data, name));
 };