#include "aux_structs.hpp"

Grafcet mainGrafcet;

std::map<std::string, ElementType> grafcetTypes = {
    {"StepNormal", e_NormalStep},
    {"StepInitial", e_InitialStep},
    {"StepMakro", e_MacroStep},
    {"StepEnclosing", e_EnclosedStep},
    {"StepInitialEnclosing", e_EnclosedInitialStep},
    {"Transition", e_Transition},
    {"ActionNormal", e_ContinuousAction},
    {"ActionConditional", e_ContinuousActionWCondition},
    {"ActionActivation", e_StoredAction},
    {"ActionDeactivation", e_StoredAction},
    {"ActionEvent", e_EventAction},
    {"ActionForcing", e_ForcingOrder},
    {"ParallelConnectionTop", e_Parallelization},
    {"ParallelConnectionBottom", e_Synchronization},
};

std::map<ElementType, std::string> grafcetTypesR = {
    {e_NormalStep, "StepNormal"},
    {e_InitialStep, "StepInitial"},
    {e_MacroStep, "StepMakro"},
    {e_EnclosedStep, "StepEnclosing"},
    {e_EnclosedInitialStep, "StepInitialEnclosing"},
    {e_Transition, "Transition"},
    {e_ContinuousAction, "ActionNormal"},
    {e_ContinuousActionWCondition, "ActionConditional"},
    {e_StoredAction, "ActionActivation"},
    {e_StoredAction, "ActionDeactivation"},
    {e_EventAction, "ActionEvent"},
    {e_ForcingOrder, "ActionForcing"},
    {e_Parallelization, "ParallelConnectionTop"},
    {e_Synchronization, "ParallelConnectionBottom"},
};

std::map<PartialGrafcetType, std::string> partialGrafcetTypesR = {
    {partial_normal, "Partial"},
    {partial_macro, "MacroStepExpansion"},
    {partial_enclosing, "EnclosingStepExpansion"},
};

std::map<TransitionType, std::string> transitionTypesR = {
    {transition_normal, "Normal"},
    {transition_sink, "Sink"},
    {transition_source, "Source"},
};

std::map<ActionType, std::string> actionTypesR = {
    {explicitSituation, "ExplicitSituation"},
    {currentSituation, "CurrentSituation"},
    {emptySituation, "EmptySituation"},
    {initialSituation, "InitialSituation"},
    {ActivationAction, "Activation"},
    {DeactivationAction, "Deactivation"},
    {EventAction, "Event"},
    {ContinuousActionType, "Continuous"},
    {ConditionalActionType, "Conditional"},
};

std::map<SynchronizationType, std::string> synchronizationTypesR = {
    {Synchronization, "Synchronization"},
    {Parallelization, "Parallelization"},
};

// Reads the elements from an XML file node (cNode), creates and pushes the according 
// element to a main vector (nodeVector) which will be removed later
void create_element(std::vector<Node*> &nodeVector, pugi::xml_node cNode, std::vector<ElementLinks*> &links, 
                    std::vector<PartialGrafcetIdentifications*> &PGIs)
{
    ElementType tempType = grafcetTypes.at(cNode.child("Type").text().as_string());
    ElementLinks* tempLink = new ElementLinks();
    Node* tempNode = new Node();
    std::string macroStepName = cNode.child("AssociatedMacroStep").text().as_string();
    std::string enclosedStepName = cNode.child("AssociatedEnclosedStep").text().as_string();

    tempNode->id = cNode.child("Identifier").text().as_int();
    tempLink->id = tempNode->id;
    tempNode->name = cNode.child("Name").text().as_string();
    tempNode->mainType = tempType;

    // Partial grafcet identifications creation and population
    if(macroStepName != "NULL" || enclosedStepName != "NULL")
    {
        PGIs.push_back(new PartialGrafcetIdentifications());
        PGIs.back()->id = tempNode->id;
        PGIs.back()->associatedPartialGrafcetName = cNode.child("PartialGrafcetName").text().as_string();
        if(macroStepName != "NULL")
        {
            PGIs.back()->type = partial_macro;
            PGIs.back()->associatedStep = macroStepName;
        }
        else if(enclosedStepName != "NULL")
        {
            PGIs.back()->type = partial_enclosing;
            PGIs.back()->associatedStep = enclosedStepName;
        }
    }
    else if((std::string)cNode.child("PartialGrafcetName").text().as_string() != "NULL") // Check partial grafcets w/o associated macro/enclosing step
    {
        PGIs.push_back(new PartialGrafcetIdentifications());
        PGIs.back()->id = tempNode->id;
        PGIs.back()->associatedPartialGrafcetName = cNode.child("PartialGrafcetName").text().as_string();
        PGIs.back()->type = partial_normal;
    }
    

    switch(tempType)
    {
        case e_NormalStep:
        case e_InitialStep:
        case e_MacroStep:
        case e_EnclosedStep:
        case e_EnclosedInitialStep:
        {
            if(tempType == e_InitialStep || tempType == e_EnclosedInitialStep) tempNode->isInitial = true;
            tempNode->activationLink = cNode.child("IsInitial").text().as_bool();
            element_arc_search(tempLink->previousElements, cNode.child("PreviousComponents").text().as_string());
            element_arc_search(tempLink->nextElements, cNode.child("NextComponents").text().as_string());
            break;
        }

        case e_Transition:
        {
            if((std::string)cNode.child("TransitionType").text().as_string() == "Normal") tempNode->transitionType = transition_normal;
            else if((std::string)cNode.child("TransitionType").text().as_string() == "Sink") tempNode->transitionType = transition_sink;
            else tempNode->transitionType = transition_source;
            tempNode->condition = cNode.child("Condition").text().as_string();
            element_arc_search(tempLink->previousElements, cNode.child("PreviousComponents").text().as_string());
            element_arc_search(tempLink->nextElements, cNode.child("NextComponents").text().as_string());
            break;
        }

        case e_Synchronization:
        case e_Parallelization:
        {
            tempNode->syncType = (tempType == e_Synchronization) ? Synchronization : Parallelization;
            element_arc_search(tempLink->previousElements, cNode.child("PreviousComponents").text().as_string());
            element_arc_search(tempLink->nextElements, cNode.child("NextComponents").text().as_string());
            break;
        }

        case e_ContinuousAction:
        {
            tempNode->actionType = ContinuousActionType;
            tempNode->action = cNode.child("Action").text().as_string();
            tempLink->associatedStep = cNode.child("AssociatedStep").text().as_int();
            break;
        }

        case e_ContinuousActionWCondition:
        {
            tempNode->actionType = ConditionalActionType;
            tempNode->action = cNode.child("Action").text().as_string();
            tempNode->condition = cNode.child("Condition").text().as_string();
            tempLink->associatedStep = cNode.child("AssociatedStep").text().as_int();
            break;
        }

        case e_StoredAction:
        {
            tempNode->actionType = ((std::string)cNode.child("Edge").text().as_string() == "Rising") ? ActivationAction : DeactivationAction;
            tempNode->action = cNode.child("Action").text().as_string();
            tempLink->associatedStep = cNode.child("AssociatedStep").text().as_int();
            break;
        }

        case e_EventAction:
        {
            tempNode->actionType = EventAction;
            tempNode->action = cNode.child("Action").text().as_string();
            tempNode->condition = cNode.child("Condition").text().as_string();
            tempLink->associatedStep = cNode.child("AssociatedStep").text().as_int();
            break;
        }

        case e_ForcingOrder:
        {
            tempNode->action = cNode.child("Action").text().as_string();
            if(tempNode->action.find("*") != std::string::npos) tempNode->actionType = currentSituation;
            else if(tempNode->action.find("{}") != std::string::npos) tempNode->actionType = emptySituation;
            else if(tempNode->action.find("INIT") != std::string::npos) tempNode->actionType = initialSituation;
            else tempNode->actionType = explicitSituation;
            tempLink->associatedStep = cNode.child("AssociatedStep").text().as_int();
            break;
        }
        
        default: break;
    }

    nodeVector.push_back(tempNode);
    links.push_back(tempLink);

    return;
}

// Creates, populates and associates connected and partial grafcets given element-by-element information
// Note: Due to how the partial grafcet ID association is made, this function is only intended to run once OR be
// the partial grafcets must be associated to a diferent main Grafcet, otherwise duplicate IDs might appear
void create_grafcets(std::vector<Node*> &nodeVector, std::vector<PartialGrafcetIdentifications*> &PGIVector)
{
    std::vector<ConnectedGrafcet*> tempConnectedGrafcetList;
    std::vector<PartialGrafcet*> tempPartialGrafcetList;
    bool foundPartialGrafcetFlag = false;
    bool createPartialGrafcetSkipFlag = false;
    int newID = 0;
    Node* tempNode = nullptr;

    create_connected_grafcets(tempConnectedGrafcetList, nodeVector); // Get list of connected grafcets
    
    // Checks all connect grafcets
    for(const auto& cgraf : tempConnectedGrafcetList)
    {
        foundPartialGrafcetFlag = false;
        createPartialGrafcetSkipFlag = false;

        // Checks if connect grafcet belongs to partial grafcet
        for(const auto& pgi : PGIVector) if(cgraf->elements[0]->id == pgi->id)
        {
            foundPartialGrafcetFlag = true; // Partial grafcet found

            // Checks if partial grafcet associated to connect grafcet exists already in partial grafcet list
            // If it does, associated it to said partial grafcet 
            if(!tempPartialGrafcetList.empty()) 
            {
                for(const auto& pgraf : tempPartialGrafcetList)
                {
                    if(pgraf->name == pgi->associatedPartialGrafcetName) // If partial grafcet already exists
                    {
                        pgraf->connectedGrafcets.push_back(cgraf);
                        createPartialGrafcetSkipFlag = true;
                        break;
                    }
                }
            }

            if(createPartialGrafcetSkipFlag) break;

            // If no partial grafcet found in list, create and populate partial grafcet
            tempPartialGrafcetList.push_back(new PartialGrafcet());
            tempPartialGrafcetList.back()->id = newID++;
            tempPartialGrafcetList.back()->name = pgi->associatedPartialGrafcetName;
            tempPartialGrafcetList.back()->type = pgi->type;
            if(tempPartialGrafcetList.back()->type != partial_normal)
            {
                // Does extra checks if name is actually ID (which it seems to be for many)
                tempNode = node_pointer_by_name(nodeVector, pgi->associatedStep);
                if(tempNode == nullptr) tempNode = node_pointer_by_id(nodeVector, stoi(pgi->associatedStep));
                if(tempNode == nullptr)
                {
                    std::cout << "ERROR: create_grafcets(): No associated step found for partial grafcet of ID " 
                              << tempPartialGrafcetList.back()->id << "." << std::endl;
                    exit(EXIT_FAILURE);
                }
                tempPartialGrafcetList.back()->associatedStep = tempNode;
                tempPartialGrafcetList.back()->associatedStep->expansions.push_back(tempPartialGrafcetList.back());
            }
            tempPartialGrafcetList.back()->connectedGrafcets.push_back(cgraf);

            break;
        }
        
        if(!foundPartialGrafcetFlag) mainGrafcet.connectedGrafcets.push_back(cgraf); // If no partial grafcet found, add to main grafcet
    }
    
    // Associate partial grafcets to the main Grafcet
    for(const auto& pgraf : tempPartialGrafcetList) mainGrafcet.partialGrafcets.push_back(pgraf);

    // No dangling pointers
    tempConnectedGrafcetList.clear();
    tempPartialGrafcetList.clear();

    return;
}

// Creates connected grafcets objects into a vector "connectedGrafcetList" and associates the according nodes to it
// Note: Due to how the connect grafcet ID association is made, this function is only intended to run once OR be
// the connected grafcets must be associated to a diferent main Grafcet, otherwise duplicate IDs might appear
void create_connected_grafcets(std::vector<ConnectedGrafcet*> &connectedGrafcetList, std::vector<Node*> &nodeVector)
{
    bool skipFlag = false; // Skips checking structure in case of repitions
    ConnectedGrafcet* tempConnectedGrafcet;
    int newID = 0;
    std::vector<Node*> tempNodeVector;

    for(auto& node : nodeVector)
    {
        skipFlag = false;
        tempConnectedGrafcet = nullptr;
        tempNodeVector.clear();

        if(!connectedGrafcetList.empty())
        {
            for(const auto& cgraf : connectedGrafcetList)
            {                
                for(const auto& e : cgraf->elements)
                {
                    if(node->id == e->id) skipFlag = true;
                }
                if(skipFlag) break;
            }
            if(skipFlag) continue;           
        }
        
        tempConnectedGrafcet = new ConnectedGrafcet();
        tempConnectedGrafcet->id = newID++;
        
        connected_grafcet_element_association(node, tempNodeVector); // List with all node associations
        if(!tempNodeVector.empty()) 
        {
            for(const auto& n : tempNodeVector) tempConnectedGrafcet->elements.push_back(n); // Add list to tempConnectedGrafcet
            connectedGrafcetList.push_back(tempConnectedGrafcet); // Add tempConnectedGrafcet to connectedGrafcetList
        }
    }

    return;
}

// Iterative function that goes over the previous/next nodes of a given node,
// and associates them all into an ID based vector
void connected_grafcet_element_association(Node* &node, std::vector<Node*> &nodeList)
{
    for(const auto& n : nodeList) if(node->id == n->id) return; // If node already exists in list, return

    nodeList.push_back(node);

    if(!node->actionList.empty()) for(auto& act : node->actionList) nodeList.push_back(act);
    if(!node->previousElements.empty()) for(auto& prev : node->previousElements) connected_grafcet_element_association(prev, nodeList);
    if(!node->nextElements.empty()) for(auto& next : node->nextElements) connected_grafcet_element_association(next, nodeList);

    return;
}

// Parses string (arcStr) with identifiers for previous/next elements for a given element, 
// pushing into their element vector (arcList)
void element_arc_search(std::vector<int> &arcList, std::string arcStr)
{
    std::string tempStr = "";

    for(char c : arcStr)
    {
        if(c == 'N') return; // If first character is 'N' it implies field is "NULL", therefore no association is made

        if(c != ',')
        {
            tempStr = tempStr + c;
        }
        else
        {
            arcList.push_back(std::stoi(tempStr));
            tempStr = "";
        }
    }
    
    return;
}

// Makes the associations between elements, including action-step associations
void make_arc_associations(std::vector<Node*> &nodes, std::vector<ElementLinks*> &links)
{
    for(const auto& link : links)
    {
        for(const auto& node : nodes)
        {
            if(node->id != link->id) continue;            

            for(const auto& dirlnk : nodes)
            {
                // Other way around since id refers to actions and associatedStep to the step
                if(dirlnk->id == link->associatedStep){ dirlnk->actionList.push_back(node); break; }

                for(const auto& previous : link->previousElements) if(dirlnk->id == previous) node->previousElements.push_back(dirlnk);
                for(const auto& next : link->nextElements) if(dirlnk->id == next) node->nextElements.push_back(dirlnk);
            }
        }
    }
    
    return;
}

// Finds the pointer of the vector "nodes" for the node with id "nodeID" and returns it
Node* node_pointer_by_id(std::vector<Node*> &nodes, int nodeID)
{
    for(const auto& node : nodes) if(node->id == nodeID) return node;
    #ifdef DEBUG
    std::cout << "node_pointer_by_id(): No node with ID " << nodeID << " found." << std::endl;
    #endif
    return nullptr;
}

// Finds the pointer of the vector "nodes" for the node with name "nodeName" and returns it
Node* node_pointer_by_name(std::vector<Node*> &nodes, std::string nodeName)
{
    for(const auto& node : nodes) if(node->name == nodeName) return node;
    #ifdef DEBUG
    std::cout << "node_pointer_by_name(): No node with name " << nodeName << " found." << std::endl;
    #endif
    return nullptr;
}