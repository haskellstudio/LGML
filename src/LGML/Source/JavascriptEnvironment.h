/*
  ==============================================================================

    JavascriptEnvironnement.h
    Created: 5 May 2016 9:03:35am
    Author:  Martin Hermant

  ==============================================================================
*/

#ifndef JAVASCRIPTENVIRONNEMENT_H_INCLUDED
#define JAVASCRIPTENVIRONNEMENT_H_INCLUDED

#include "JuceHeader.h"
#include "ControllableContainer.h"
class JavascriptEnvironment : public JavascriptEngine,public ControllableContainer::Listener{
public:
    JavascriptEnvironment();
    ~JavascriptEnvironment();

    juce_DeclareSingleton(JavascriptEnvironment, true);

    typedef juce::var::NativeFunctionArgs NativeFunctionArgs;


    static  DynamicObject *  createDynamicObjectFromContainer(ControllableContainer * c,DynamicObject * parent);
    void    linkToControllableContainer(const String & jsNamespace,ControllableContainer * c);
    void    addToNamespace(const String & name,const String & elemName,DynamicObject *);
    void removeFromNamespace(const String & name,const String & elemName);
    void    removeNamespace(const String & jsNamespace);
    void    loadFile(const String & path);
    void rebuildAllNamespaces();

    class JsContainerNamespace;
    bool existInNamespace(const String & name,const String & module );
    JsContainerNamespace* getContainerNamespace(ControllableContainer *);
    JsContainerNamespace* getContainerNamespace(const String & );
    bool existInContainerNamespace(const String &);

    //////////////////
    // helperclasses

    class JsContainerNamespace{
    public:
        JsContainerNamespace(const String & n,ControllableContainer * c,DynamicObject * o):nsName (n),container(c),jsObject(o){}
        WeakReference<ControllableContainer> container;
        DynamicObject::Ptr jsObject;
        String nsName;

    };


    class OwnedJsArgs {


    public:
        OwnedJsArgs(var _owner):owner(_owner){}
        void addArg(float f){ownedArgs.add(new var(f));}
        void addArg(String f){ownedArgs.add(new var(f));}
        void addArgs(const StringArray & a){for(auto & s:a){addArg(s.getFloatValue());}}

        NativeFunctionArgs getNativeArgs(){
            return NativeFunctionArgs(owner,ownedArgs.getFirst(),ownedArgs.size());
        }
    private:
        var owner;
        OwnedArray<var> ownedArgs;
    };


    DynamicObject * getEnv(){return localEnvironment.getDynamicObject();}

    String printAllNamespace();
private:

    var localEnvironment;


    OwnedArray<JsContainerNamespace>  linkedContainerNamespaces;

    StringArray loadedFiles;


    static void  post(const String & s);
    void internalLoadFile(const File &);
    void childStructureChanged(ControllableContainer * )override;

    static var post(const NativeFunctionArgs& a);
    static var set(const NativeFunctionArgs& a);

    String namespaceToString(const NamedValueSet & v,int indentLevel = 0);

};




#endif  // JAVASCRIPTENVIRONNEMENT_H_INCLUDED
