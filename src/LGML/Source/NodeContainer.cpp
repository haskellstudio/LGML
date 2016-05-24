/*
 ==============================================================================

 NodeContainer.cpp
 Created: 18 May 2016 7:53:56pm
 Author:  bkupe

 ==============================================================================
 */

#include "NodeContainer.h"
#include "NodeManager.h"
#include "NodeConnection.h"
#include "NodeContainerUI.h"


#include "DebugHelpers.h"
NodeContainer::NodeContainer(const String &name) :
containerInNode(nullptr),
containerOutNode(nullptr),
ConnectableNode(name, NodeType::ContainerType,false)
{
    saveAndLoadRecursiveData = false;
}


NodeContainer::~NodeContainer()
{
    //connections.clear();
    clear(false);
}

void NodeContainer::clear(bool recreateContainerNodes)
{

    while (connections.size() > 0)
    {
        connections[0]->remove();
    }

    while (nodes.size() > 0)
    {
        nodes[0]->remove();
    }

    //connections.clear();


    containerInNode = nullptr;
    containerOutNode = nullptr;

    if (recreateContainerNodes && parentNodeContainer != nullptr)
    {
        containerInNode = (ContainerInNode *)addNode(new ContainerInNode());
        containerOutNode = (ContainerOutNode *)addNode(new ContainerOutNode());

        containerInNode->xPosition->setValue(150);
        containerInNode->yPosition->setValue(100);
        containerOutNode->xPosition->setValue(450);
        containerOutNode->yPosition->setValue(100);

        containerInNode->addRMSListener(this);
        containerOutNode->addRMSListener(this);

        //maybe keep it ?
        addConnection(containerInNode, containerOutNode, NodeConnection::ConnectionType::AUDIO);
    }

    ConnectableNode::clear();
}


ConnectableNode * NodeContainer::addNode(NodeType nodeType, const String &nodeName,bool callNodeAddedNow)
{
    ConnectableNode * n = NodeFactory::createNode(nodeType);
    return addNode(n,nodeName,callNodeAddedNow);
}

ConnectableNode * NodeContainer::addNode(ConnectableNode * n, const String &nodeName,bool callNodeAddedNow)
{
    nodes.add(n);
    n->setParentNodeContainer(this);

    if (n->type == NodeType::ContainerType)
    {
        nodeContainers.add((NodeContainer *)n);
        ((NodeContainer *)n)->clear(true);
        //DBG("Check containerIn Node : " << String(((NodeContainer *)n)->containerInNode != nullptr));
    }


    n->addNodeListener(this);
    String targetName = (nodeName.isNotEmpty())?nodeName:n->nameParam->stringValue();

    DBG("Target Name for new node :" << targetName);
    n->nameParam->setValue(getUniqueNameInContainer(targetName));

    addChildControllableContainer(n); //ControllableContainer
   if(callNodeAddedNow) nodeContainerListeners.call(&NodeContainerListener::nodeAdded, n);
    return n;
}



bool NodeContainer::removeNode(ConnectableNode * n)
{
    Array<NodeConnection *> relatedConnections = getAllConnectionsForNode(n);

    for (auto &connection : relatedConnections) removeConnection(connection);

    if (n == nullptr) return false;
    n->removeNodeListener(this);
    removeChildControllableContainer(n);

    nodeContainerListeners.call(&NodeContainerListener::nodeRemoved, n);
    nodes.removeAllInstancesOf(n);

    n->clear();
    n->removeFromAudioGraph();

    if (n->type == NodeType::ContainerType) nodeContainers.removeObject((NodeContainer*)n);

    //if(NodeManager::getInstanceWithoutCreating() != nullptr) NodeManager::getInstance()->audioGraph.removeNode(n->audioNode);

    return true;
}

ConnectableNode * NodeContainer::getNodeForName(const String & name)
{
    for (auto &n : nodes)
    {
        if (n->shortName == name) return n;
    }
    return nullptr;
}



int NodeContainer::getNumConnections() {
    return connections.size();
}

bool NodeContainer::loadPreset(PresetManager::Preset * preset)
{
    if(!ControllableContainer::loadPreset(preset)) return false;

    for (auto &n : nodes) n->loadPresetWithName(preset->name);

    return true;
}

void NodeContainer::saveNewPreset(const String & name)
{
    ControllableContainer::saveNewPreset(name);
    for (auto &n : nodes) n->saveNewPreset(name);
}

bool NodeContainer::saveCurrentPreset()
{
    if (!ControllableContainer::saveCurrentPreset()) return false;

    for (auto &n : nodes) n->saveCurrentPreset();
    return true;
}

bool NodeContainer::resetFromPreset()
{
    if (!ControllableContainer::resetFromPreset()) return false;

    for (auto &n : nodes) n->resetFromPreset();

    return true;
}



var NodeContainer::getJSONData()
{
    var data = ConnectableNode::getJSONData();
    var nodesData;

    for (auto &n : nodes)
    {
        nodesData.append(n->getJSONData());
    }

    var connectionsData;

    for (auto &c : connections)
    {
        connectionsData.append(c->getJSONData());
    }

    data.getDynamicObject()->setProperty("nodes", nodesData);
    data.getDynamicObject()->setProperty("connections", connectionsData);
    return data;
}

void NodeContainer::loadJSONDataInternal(var data)
{
    clear(false);

    Array<var> * nodesData = data.getProperty("nodes", var()).getArray();
    if(nodesData!=nullptr){
        for (var &nData : *nodesData)
        {
            addNodeFromJSON(nData);
        }
    }
    Array<var> * connectionsData = data.getProperty("connections", var()).getArray();

    if (connectionsData)
    {
        for (var &cData : *connectionsData)
        {

            ConnectableNode * srcNode = (ConnectableNode*)(getNodeForName(cData.getDynamicObject()->getProperty("srcNode").toString())) ;
            ConnectableNode * dstNode = (ConnectableNode*)(getNodeForName(cData.getDynamicObject()->getProperty("dstNode").toString()));

            int cType = cData.getProperty("connectionType", var());

            if (srcNode && dstNode && isPositiveAndBelow(cType, (int)NodeConnection::ConnectionType::UNDEFINED)) {
                NodeConnection * c = addConnection(srcNode, dstNode, NodeConnection::ConnectionType(cType));
                c->loadJSONData(cData);
            }
            else {
                // TODO nicely handle file format errors?

                if(srcNode==nullptr){
                    NLOG("loadJSON","no srcnode for shortName : "+cData.getDynamicObject()->getProperty("srcNode").toString());
                }
                if(dstNode==nullptr){
                    NLOG("loadJSON","no dstnode for shortName : "+cData.getDynamicObject()->getProperty("dstNode").toString());
                }
                LOG("Available Nodes in "+ shortName+" : ");
                for (auto &node : nodes)
                {
                    DBG(node->niceName+"//"+ node->shortName);
                }
                jassertfalse;
            }
        }
    }

    removeIllegalConnections();
}

ConnectableNode * NodeContainer::addNodeFromJSON(var nodeData, const String &baseName)
{
    NodeType nodeType = NodeFactory::getTypeFromString(nodeData.getProperty("nodeType", var()));

    ConnectableNode * node = addNode(nodeType, baseName,false);
//    String newNodeName = node->niceName;

    if (node->type == NodeType::ContainerInType)
    {
        containerInNode = (ContainerInNode *)node;
        containerInNode->addRMSListener(this);
    } else if (node->type == NodeType::ContainerOutType)
    {
        containerOutNode = (ContainerOutNode *)node;
        containerOutNode->addRMSListener(this);
    }

    // @ben
    node->loadJSONData(nodeData);

    nodeContainerListeners.call(&NodeContainerListener::nodeAdded, node);

    // @ ben why??? name should be updated from loadJSONData
    // it erase custom names
    //	node->nameParam->setValue(newNodeName);

    return node;

}


NodeConnection * NodeContainer::getConnectionBetweenNodes(ConnectableNode * sourceNode, ConnectableNode * destNode, NodeConnection::ConnectionType connectionType)
{
    ConnectableNode * tSourceNode = (sourceNode->type == ContainerType) ? ((NodeContainer *)sourceNode)->containerOutNode : sourceNode;
    ConnectableNode * tDestNode = (destNode->type == ContainerType) ? ((NodeContainer *)destNode)->containerInNode : destNode;

    for (int i = connections.size(); --i >= 0;)
    {
        NodeConnection * c = connections.getUnchecked(i);
        if (c->sourceNode == tSourceNode && c->destNode == tDestNode && c->connectionType == connectionType) return c;
    }

    return nullptr;
}

Array<NodeConnection*> NodeContainer::getAllConnectionsForNode(ConnectableNode * node)
{
    Array<NodeConnection*> result;
    ConnectableNode * tSourceNode = (node->type == ContainerType) ? ((NodeContainer *)node)->containerOutNode : node;
    ConnectableNode * tDestNode = (node->type == ContainerType) ? ((NodeContainer *)node)->containerInNode : node;

    for (auto &connection : connections)
    {
        if (connection->sourceNode == tSourceNode || connection->destNode == tDestNode) result.add(connection);
    }

    return result;
}

NodeConnection * NodeContainer::addConnection(ConnectableNode * sourceNode, ConnectableNode * destNode, NodeConnection::ConnectionType connectionType)
{
    if (getConnectionBetweenNodes(sourceNode, destNode, connectionType) != nullptr)
    {
        //connection already exists
        DBG("Connection already exists");
        return nullptr;
    }

    NodeConnection * c = new NodeConnection(sourceNode, destNode, connectionType);
    connections.add(c);
    c->addConnectionListener(this);

    // DBG("Dispatch connection Added from NodeManager");
    nodeContainerListeners.call(&NodeContainerListener::connectionAdded, c);

    return c;
}


bool NodeContainer::removeConnection(NodeConnection * c)
{
    if (c == nullptr) return false;
    c->removeConnectionListener(this);

    connections.removeObject(c);

    nodeContainerListeners.call(&NodeContainerListener::connectionRemoved, c);

    return true;
}



//From NodeBase Listener
void NodeContainer::askForRemoveNode(ConnectableNode * node)
{
    removeNode((NodeBase*)node);
}


void NodeContainer::askForRemoveConnection(NodeConnection *connection)
{
    removeConnection(connection);
}

void NodeContainer::RMSChanged(ConnectableNode * node, float _rmsInValue, float _rmsOutValue)
{
    if (node == containerInNode) rmsInValue = _rmsInValue;
    else if (node == containerOutNode) rmsOutValue = _rmsOutValue;

    rmsListeners.call(&ConnectableNode::RMSListener::RMSChanged, this, rmsInValue, rmsOutValue);
}

void NodeContainer::onContainerParameterChanged(Parameter * p)
{
    ConnectableNode::onContainerParameterChanged(p);
    if (p == enabledParam)
    {

        bypassNode(!enabledParam->boolValue());

    }
}
void NodeContainer::bypassNode(bool bypass){
    if(bypass){
        jassert(containerInNode!=nullptr &&containerOutNode!=nullptr);
//        save old ones
        Array<NodeConnection*> connectionPointers;
        connectionPointers = getAllConnectionsForNode(containerInNode);
        containerInConnections.clear();
        for(auto &c: connectionPointers){containerInConnections.add(new NodeConnection(c->sourceNode,c->destNode,c->connectionType));}
        for(auto & c:connectionPointers){removeConnection(c);}


        containerOutConnections.clear();
        connectionPointers = getAllConnectionsForNode(containerOutNode);
        for(auto &c: connectionPointers){containerOutConnections.add(new NodeConnection(c->sourceNode,c->destNode,c->connectionType));}
        for(auto & c:connectionPointers){removeConnection(c);}
        // add a pass-thru
        addConnection(containerInNode, containerOutNode,NodeConnection::ConnectionType::AUDIO);
        addConnection(containerInNode, containerOutNode,NodeConnection::ConnectionType::DATA);
        }
    else{
        // remove pass thru
        Array<NodeConnection * > bypassConnection = getAllConnectionsForNode(containerInNode);
        jassert(bypassConnection.size()==2);
        for(auto & c:bypassConnection) {removeConnection(c);}
        bypassConnection = getAllConnectionsForNode(containerOutNode);
        jassert(bypassConnection.size()==0);

        for(auto & c:containerInConnections){addConnection(c->sourceNode, c->destNode, c->connectionType);}
        for(auto & c:containerOutConnections){addConnection(c->sourceNode, c->destNode, c->connectionType);}
        

    }
}

ConnectableNodeUI * NodeContainer::createUI()
{
    return new NodeContainerUI(this);
}


bool NodeContainer::hasDataInputs()
{
    return containerInNode != nullptr ? containerInNode->hasDataInputs() : false;
}

bool NodeContainer::hasDataOutputs()
{
    return containerOutNode != nullptr ? containerOutNode->hasDataOutputs() : false;
}


AudioProcessorGraph::Node * NodeContainer::getAudioNode(bool isInput)
{
    if (isInput)
    {
        return containerInNode == nullptr ? nullptr : containerInNode->getAudioNode();
    }
    else
    {
        return containerOutNode == nullptr ? nullptr : containerOutNode->getAudioNode();
    }
}




void NodeContainer::removeIllegalConnections() {
    //TODO synchronize this and implement it for data
    // it's not indispensable
    if (NodeManager::getInstanceWithoutCreating() != nullptr)
    {
        jassert(!NodeManager::getInstance()->audioGraph.removeIllegalConnections());
    }
}
