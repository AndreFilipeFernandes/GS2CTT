#include "aux_funcs.hpp"

ModelCheckErrorVars MCEV;

// Prints for each type of error
void model_check_errors_map()
{
    if(MCEV.CommonVariablesAssociation) std::cout << "Common Variables Association" << " | ";
    if(MCEV.PartialGrafcetAssociation) std::cout << "Partial Grafcet Association" << " | ";
    if(MCEV.NodeArcAssociation) std::cout << "Node Arc Association" << " | ";
    if(MCEV.ActionAssociation) std::cout << "Action Association" << " | ";
    if(MCEV.ConditionAssociation) std::cout << "Condition Association" << " | ";
    if(MCEV.ActionListAssociation) std::cout << "Action List Association" << " | ";
    if(MCEV.ActivationLinkAssociation) std::cout << "Activation Link Association" << " | ";
    return;
}

// Sets MCEV's values to false
void model_check_set_struct_false()
{
    MCEV.CommonVariablesAssociation = false;
    MCEV.PartialGrafcetAssociation = false;
    MCEV.NodeArcAssociation = false;
    MCEV.ActionAssociation = false;
    MCEV.ConditionAssociation = false;
    MCEV.ActionListAssociation = false;
    MCEV.ActivationLinkAssociation = false;
    MCEV.SpecificTypeAssociation = false;
    return;
}

// Checks whether there are true values in MCEV struct
bool model_check_struct_true()
{
    if(MCEV.CommonVariablesAssociation) return true;
    if(MCEV.PartialGrafcetAssociation) return true;
    if(MCEV.NodeArcAssociation) return true;
    if(MCEV.ActionAssociation) return true;
    if(MCEV.ConditionAssociation) return true;
    if(MCEV.ActionListAssociation) return true;
    if(MCEV.ActivationLinkAssociation) return true;
    return false;
}

// Checks whether the meta model assembled is correct according to the intermediate file from which the data was extracted
void check_model(pugi::xml_node &fileNode)
{
    fileNode = fileNode.child("GrafcetProject");

    bool existFlag = false;
    bool actFlag = false;
    bool elemFlag = false;
    std::vector<int> prevNodes;
    std::vector<int> nextNodes;

    // Check partial
    for(const auto& pgraf : mainGrafcet.partialGrafcets)
    {
        for(const auto& cgraf : pgraf->connectedGrafcets)
        {
            for(const auto& node : cgraf->elements)
            {
                existFlag = false;
                actFlag = false;
                elemFlag = false;
                model_check_set_struct_false();
                prevNodes.clear();
                nextNodes.clear();
                
                for(pugi::xml_node page : fileNode.children("Page"))
                {
                    for(pugi::xml_node compList : page.children("ComponentCollection"))
                    {
                        for(pugi::xml_node comp : compList.children("Component"))
                        {
                            if(comp.child("Identifier").text().as_int() != node->id) continue;

                            existFlag = true;

                            if((std::string)comp.child("Name").text().as_string() != node->name
                            || grafcetTypes.at(comp.child("Type").text().as_string()) != node->mainType
                            || (std::string)comp.child("PartialGrafcetName").text().as_string() != pgraf->name) MCEV.CommonVariablesAssociation = true;

                            if((std::string)comp.child("AssociatedMacroStep").text().as_string() != "NULL")
                            {
                                if(comp.child("AssociatedMacroStep").text().as_int() != pgraf->associatedStep->id
                                || pgraf->type != partial_macro) MCEV.PartialGrafcetAssociation = true;
                            }
                            else if((std::string)comp.child("AssociatedEnclosedStep").text().as_string() != "NULL")
                            {
                                if(comp.child("AssociatedEnclosedStep").text().as_int() != pgraf->associatedStep->id
                                || pgraf->type != partial_enclosing) MCEV.PartialGrafcetAssociation = true;
                            }
                            else if(pgraf->type != partial_normal) MCEV.PartialGrafcetAssociation = true;

                            if((std::string)comp.child("PreviousComponents").text().as_string() != "NULL")
                            {
                                element_arc_search(prevNodes, comp.child("PreviousComponents").text().as_string());
                                for(const auto& p : prevNodes) for(const auto& nd : node->previousElements)
                                    if(nd->id == p) elemFlag = true;
                            }
                            else if(node->previousElements.empty()) elemFlag = true;

                            if((std::string)comp.child("NextComponents").text().as_string() != "NULL")
                            {
                                element_arc_search(nextNodes, comp.child("NextComponents").text().as_string());
                                for(const auto& n : nextNodes) for(const auto& nd : node->nextElements)
                                    if(nd->id == n) elemFlag = true;
                            }
                            else if(node->nextElements.empty()) elemFlag = true;

                            if(!elemFlag) MCEV.NodeArcAssociation = true;

                            switch(node->mainType)
                            {
                                case e_Transition:
                                {
                                    if((std::string)comp.child("Condition").text().as_string() != node->condition)
                                        MCEV.ConditionAssociation = true;
                                    if((std::string)comp.child("TransitionType").text().as_string() != transitionTypesR.at(node->transitionType))
                                        MCEV.SpecificTypeAssociation = true;
                                    break;
                                }

                                case e_ContinuousAction:
                                case e_StoredAction:
                                {
                                    if((std::string)comp.child("Action").text().as_string() != node->action)
                                        MCEV.ActionAssociation = true;
                                    for(const auto& act : node_pointer_by_id(cgraf->elements, comp.child("AssociatedStep").text().as_int())->actionList)
                                        if(comp.child("Identifier").text().as_int() == act->id)
                                            actFlag = true;
                                    if(!actFlag) MCEV.ActionListAssociation = true;
                                    break;
                                }

                                case e_ContinuousActionWCondition:
                                case e_EventAction:
                                case e_ForcingOrder:
                                {
                                    if((std::string)comp.child("Action").text().as_string() != node->action)
                                        MCEV.ActionAssociation = true;
                                    if((std::string)comp.child("Condition").text().as_string() != node->condition)
                                        MCEV.ConditionAssociation = true;
                                    for(const auto& act : node_pointer_by_id(cgraf->elements, comp.child("AssociatedStep").text().as_int())->actionList)
                                        if(comp.child("Identifier").text().as_int() == act->id)
                                            actFlag = true;
                                    if(!actFlag) MCEV.ActionListAssociation = true;
                                    break;
                                }

                                case e_NormalStep:
                                case e_InitialStep:
                                case e_MacroStep:
                                case e_EnclosedStep:
                                case e_EnclosedInitialStep:
                                {
                                    if(comp.child("IsInitial").text().as_bool() != node->activationLink)
                                        MCEV.ActivationLinkAssociation = true;

                                    if((node->mainType == e_InitialStep || node->mainType == e_EnclosedInitialStep) 
                                    && !node->isInitial)
                                        MCEV.SpecificTypeAssociation = true;
                                    else if((node->mainType == e_NormalStep || node->mainType == e_MacroStep || node->mainType == e_EnclosedStep)
                                    && node->isInitial)
                                        MCEV.SpecificTypeAssociation = true;
                                    break;
                                }
                                case e_Synchronization:
                                case e_Parallelization:
                                {
                                    if((std::string)comp.child("Type").text().as_string() != "ParallelConnectionTop"
                                    && node->mainType == e_Parallelization)
                                        MCEV.SpecificTypeAssociation = true;
                                    else if((std::string)comp.child("Type").text().as_string() != "ParallelConnectionBottom"
                                    && node->mainType == e_Synchronization)
                                        MCEV.SpecificTypeAssociation = true;
                                    break;
                                }
                                default: break;
                            }
                        }
                    }
                }

                if(!existFlag) std::cout << "NOT FOUND | " << "Node " << node->id << std::endl;
                else if(!model_check_struct_true()) std::cout << "OK" << " | Node " << node->id << std::endl;
                else
                {
                    std::cout << "ERROR" << " | Node " << node->id << " - ";
                    model_check_errors_map();
                    std::cout << std::endl;
                }
            }
        }
    }

    // Check connected
    for(const auto& cgraf : mainGrafcet.connectedGrafcets)
    {
        for(const auto& node : cgraf->elements)
        {
            existFlag = false;
            actFlag = false;
            elemFlag = false;
            model_check_set_struct_false();
            prevNodes.clear();
            nextNodes.clear();
            
            for(pugi::xml_node page : fileNode.children("Page"))
            {
                for(pugi::xml_node compList : page.children("ComponentCollection"))
                {
                    for(pugi::xml_node comp : compList.children("Component"))
                    {
                        if(comp.child("Identifier").text().as_int() != node->id) continue;

                        existFlag = true;

                        if((std::string)comp.child("Name").text().as_string() != node->name
                        || grafcetTypes.at(comp.child("Type").text().as_string()) != node->mainType) MCEV.CommonVariablesAssociation = true;
                        
                        if((std::string)comp.child("PreviousComponents").text().as_string() != "NULL")
                        {
                            element_arc_search(prevNodes, comp.child("PreviousComponents").text().as_string());
                            for(const auto& p : prevNodes) for(const auto& nd : node->previousElements)
                                if(nd->id == p) elemFlag = true;
                        }
                        else if(node->previousElements.empty()) elemFlag = true;

                        if((std::string)comp.child("NextComponents").text().as_string() != "NULL")
                        {
                            element_arc_search(nextNodes, comp.child("NextComponents").text().as_string());
                            for(const auto& n : nextNodes) for(const auto& nd : node->nextElements)
                                if(nd->id == n) elemFlag = true;
                        }
                        else if(node->nextElements.empty()) elemFlag = true;

                        if(!elemFlag) MCEV.NodeArcAssociation = true;

                        switch(node->mainType)
                        {
                            case e_Transition:
                            {
                                if((std::string)comp.child("Condition").text().as_string() != node->condition)
                                    MCEV.ConditionAssociation = true;
                                if((std::string)comp.child("TransitionType").text().as_string() != transitionTypesR.at(node->transitionType))
                                    MCEV.SpecificTypeAssociation = true;
                                break;
                            }

                            case e_ContinuousAction:
                            case e_StoredAction:
                            {
                                if((std::string)comp.child("Action").text().as_string() != node->action)
                                    MCEV.ActionAssociation = true;
                                for(const auto& act : node_pointer_by_id(cgraf->elements, comp.child("AssociatedStep").text().as_int())->actionList)
                                    if(comp.child("Identifier").text().as_int() == act->id)
                                        actFlag = true;
                                if(!actFlag) MCEV.ActionListAssociation = true;
                                break;
                            }

                            case e_ContinuousActionWCondition:
                            case e_EventAction:
                            case e_ForcingOrder:
                            {
                                if((std::string)comp.child("Action").text().as_string() != node->action)
                                    MCEV.ActionAssociation = true;
                                if((std::string)comp.child("Condition").text().as_string() != node->condition)
                                    MCEV.ConditionAssociation = true;
                                for(const auto& act : node_pointer_by_id(cgraf->elements, comp.child("AssociatedStep").text().as_int())->actionList)
                                    if(comp.child("Identifier").text().as_int() == act->id)
                                        actFlag = true;
                                if(!actFlag) MCEV.ActionListAssociation = true;
                                break;
                            }

                            case e_NormalStep:
                            case e_InitialStep:
                            case e_MacroStep:
                            case e_EnclosedStep:
                            case e_EnclosedInitialStep:
                            {
                                if(comp.child("IsInitial").text().as_bool() != node->activationLink)
                                    MCEV.ActivationLinkAssociation = true;

                                if((node->mainType == e_InitialStep || node->mainType == e_EnclosedInitialStep) 
                                && !node->isInitial)
                                    MCEV.SpecificTypeAssociation = true;
                                else if((node->mainType == e_NormalStep || node->mainType == e_MacroStep || node->mainType == e_EnclosedStep)
                                && node->isInitial)
                                    MCEV.SpecificTypeAssociation = true;
                                break;
                            }
                            case e_Synchronization:
                            case e_Parallelization:
                            {
                                if((std::string)comp.child("Type").text().as_string() != "ParallelConnectionTop"
                                && node->mainType == e_Parallelization)
                                    MCEV.SpecificTypeAssociation = true;
                                else if((std::string)comp.child("Type").text().as_string() != "ParallelConnectionBottom"
                                && node->mainType == e_Synchronization)
                                    MCEV.SpecificTypeAssociation = true;
                                break;
                            }
                            default: break;
                        }
                    }
                }
            }

            if(!existFlag) std::cout << "NOT FOUND | " << "Node " << node->id << std::endl;
            else if(!model_check_struct_true()) std::cout << "OK" << " | Node " << node->id << std::endl;
            else
            {
                std::cout << "ERROR" << " | Node " << node->id << " - ";
                model_check_errors_map();
                std::cout << std::endl;
            }
        }
    }

    return;
}

// Prints full partial grafcet list formated in XML
void print_partial_grafcets(pugi::xml_node mainNode, std::vector<PartialGrafcet*> &partialGrafcetList)
{
    pugi::xml_node pgrafMainNode = mainNode.append_child("PartialGrafcetList");

    for(const auto& pgraf : partialGrafcetList)
    {
        pugi::xml_node pgrafNode = pgrafMainNode.append_child("PartialGrafcet");

        pgrafNode.append_child("Identifier").text() = pgraf->id;
        pgrafNode.append_child("Name").text() = pgraf->name.c_str();
        pgrafNode.append_child("Type").text() = partialGrafcetTypesR.at(pgraf->type);
        if(pgraf->associatedStep != nullptr) pgrafNode.append_child("AssociatedStepID").text() = pgraf->associatedStep->id;

        print_connected_grafcets(pgrafNode, pgraf->connectedGrafcets);
    }

    return;
}

// Prints full connected grafcet list formated in XML
void print_connected_grafcets(pugi::xml_node mainNode, std::vector<ConnectedGrafcet*> &connectedGrafcetList)
{
    pugi::xml_node cgrafMainNode = mainNode.append_child("ConnectedGrafcetList");

    for(const auto& cgraf : connectedGrafcetList)
    {
        pugi::xml_node cgrafNode = cgrafMainNode.append_child("ConnectedGrafcet");

        cgrafNode.append_child("Identifier").text() = cgraf->id;
        
        print_nodes(cgrafNode, cgraf->elements);
    }

    return;
}

// Prints full node list formated in XML
void print_nodes(pugi::xml_node mainNode, std::vector<Node*> &nodeList)
{
    pugi::xml_node elementMainNode = mainNode.append_child("NodeList");
    pugi::xml_node elementNode;

    std::string prevStr = "";
    std::string nextStr = "";
    std::string actStr = "";
    std::string expStr = "";

    for(const auto& node : nodeList)
    {
        prevStr = "";
        nextStr = "";
        actStr = "";
        expStr = "";

        elementNode = elementMainNode.append_child("Node");

        elementNode.append_child("Identifier").text() = node->id;
        elementNode.append_child("Name").text() = node->name;
        elementNode.append_child("Type").text() = grafcetTypesR.at(node->mainType);

        if(node->previousElements.empty()) prevStr = "NULL";
        else for(const auto& prev : node->previousElements) 
        {            
            if(prev->id == node->previousElements.back()->id)
            {
                prevStr = prevStr + std::to_string(prev->id);
                break;
            }
            prevStr = prevStr + std::to_string(prev->id) + ",";
        }
        elementNode.append_child("PreviousNodes").text() = prevStr;

        if(node->nextElements.empty()) nextStr = "NULL";
        else for(const auto& next : node->nextElements) 
        {            
            if(next->id == node->nextElements.back()->id)
            {
                nextStr = nextStr + std::to_string(next->id);
                break;
            }
            nextStr = nextStr + std::to_string(next->id) + ",";
        }
        elementNode.append_child("NextNodes").text() = nextStr;

        switch(node->mainType)
        {
            case e_Transition:
            {
                elementNode.append_child("Condition").text() = node->condition;
                elementNode.append_child("TransitionType").text() = transitionTypesR.at(node->transitionType);
                break;
            }

            case e_ContinuousAction:
            case e_StoredAction:
            {
                elementNode.append_child("Action").text() = node->action;
                elementNode.append_child("ActionType").text() = actionTypesR.at(node->actionType);
                break;
            }

            case e_ContinuousActionWCondition:
            case e_EventAction:
            case e_ForcingOrder:
            {
                elementNode.append_child("Action").text() = node->action;
                elementNode.append_child("Condition").text() = node->condition;
                elementNode.append_child("ActionType").text() = actionTypesR.at(node->actionType);
                break;
            }

            case e_NormalStep:
            case e_InitialStep:
            case e_MacroStep:
            case e_EnclosedStep:
            case e_EnclosedInitialStep:
            {
                if(node->actionList.empty()) actStr = "NULL";
                else for(const auto& act : node->actionList) 
                {
                    if(act->id == node->actionList.back()->id)
                    {
                        actStr = actStr + std::to_string(act->id);
                        break;
                    }
                    actStr = actStr + std::to_string(act->id) + ",";
                }
                elementNode.append_child("ActionList").text() = actStr;
                elementNode.append_child("ActivationLink").text() = node->activationLink;
                elementNode.append_child("IsInitial").text() = node->isInitial;
                
                if(node->expansions.empty()) expStr = "NULL";
                else for(const auto& exp : node->expansions) 
                {
                    if(exp->id == node->expansions.back()->id)
                    {
                        expStr = expStr + std::to_string(exp->id);
                        break;
                    }
                    expStr = expStr + std::to_string(exp->id) + ",";
                }
                elementNode.append_child("Expansions").text() = expStr;

                break;
            }

            case e_Synchronization:
            case e_Parallelization:
            {
                elementNode.append_child("SynchronizationType").text() = synchronizationTypesR.at(node->syncType);
                break;
            }
            default: break;
        }
    }

    return;
}