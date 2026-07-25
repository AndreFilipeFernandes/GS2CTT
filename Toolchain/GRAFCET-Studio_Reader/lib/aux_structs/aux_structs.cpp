#include "aux_structs.hpp"

std::vector<Page*> pages;
std::vector<Symbol*> symbols;

int globalID = 0;

std::map<std::string, ComponentType> grafcetTypes = {
    {"Page", e_Page},
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
    {"ConnectionLine", e_ConnectionLine},
    {"GroupBorder", e_GroupBorder},
    {"Jump", e_Jump},
};

std::map<ComponentType, std::string> grafcetTypesR = {
    {e_Page, "Page"},
    {e_NormalStep, "StepNormal"},
    {e_InitialStep, "StepInitial"},
    {e_MacroStep, "StepMakro"},
    {e_EnclosedStep, "StepEnclosing"},
    {e_EnclosedInitialStep, "StepInitialEnclosing"},
    {e_Transition, "Transition"},
    {e_ContinuousAction, "ActionNormal"},
    {e_ContinuousActionWCondition, "ActionConditional"},
    {e_StoredAction, "ActionActivation"},
    {e_EventAction, "ActionEvent"},
    {e_ForcingOrder, "ActionForcing"},
    {e_Parallelization, "ParallelConnectionTop"},
    {e_Synchronization, "ParallelConnectionBottom"},
    {e_ConnectionLine, "ConnectionLine"},
    {e_GroupBorder, "GroupBorder"},
    {e_Jump, "Jump"},
};

std::map<TransitionType, std::string> transitionTypes = {
    {e_normal , "Normal"},
    {e_source , "Source"},
    {e_sink , "Sink"},
};

std::map<ActionEdge, std::string> actionEdgeTypes = {
    {e_falling_edge , "Falling"},
    {e_rising_edge , "Rising"},
};

std::map<std::string, SymbolDataType> symbolDataTypes = {
    {"BOOL", e_bool},
    {"INT16", e_int16},
    {"INT32", e_int32},
    {"UINT8", e_uint8},
    {"UINT16", e_uint16},
    {"UINT32", e_uint32},
    {"FLOAT", e_float},
};

std::map<SymbolDataType, std::string> symbolDataTypesR = {
    {e_bool, "BOOL"},
    {e_int16, "INT16"},
    {e_int32, "INT32"},
    {e_uint8, "UINT8"},
    {e_uint16, "UINT16"},
    {e_uint32, "UINT32"},
    {e_float, "FLOAT"},
};

std::map<std::string, SymbolType> symbolTypes = {
    {"Input", e_input},
    {"Output", e_output},
    {"Data", e_data},
};

std::map<SymbolType, std::string> symbolTypesR = {
    {e_input, "Input"},
    {e_output, "Output"},
    {e_data, "Data"},
};

void create_component(std::vector<Page*> &pg, int pgNum, pugi::xml_node cNode)
{
    std::string tempTypeEnum = cNode.child("TypeEnum").text().as_string();  

    if(tempTypeEnum == "Comment") return; // Ignore comments and documentation boxes

    // Allocate memory for component and add to page list
    pg[pgNum]->all.push_back(new Component());  

    // Populate common variables
    pg[pgNum]->all.back()->type = grafcetTypes.at(tempTypeEnum);                                // Type
    pg[pgNum]->all.back()->positionX = round(cNode.child("Margin").child("Left").text().as_float());     // X
    pg[pgNum]->all.back()->positionY = round(cNode.child("Margin").child("Top").text().as_float());      // Y
    pg[pgNum]->all.back()->width = round(cNode.child("Width").text().as_float());                        // Width
    pg[pgNum]->all.back()->height = round(cNode.child("Height").text().as_float());                      // Height
    pg[pgNum]->all.back()->associatedPage = pg[pgNum];                                          // Associated Page

    switch(pg[pgNum]->all.back()->type)
    {
        case e_NormalStep:
        case e_InitialStep:
        case e_MacroStep:
        case e_EnclosedStep:
        case e_EnclosedInitialStep:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                                 // Identifier
            pg[pgNum]->all.back()->name = "Step_" + (std::string)cNode.child("Caption").text().as_string(); // Name
            pg[pgNum]->all.back()->isInitial = cNode.child("IsStartStep").text().as_bool();                 // isInitial
            break;
        }

        case e_Transition:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                               // Identifier
            pg[pgNum]->all.back()->name = "Transition_" + count_type(pg, pg[pgNum]->all.back()->type);    // Name
            pg[pgNum]->all.back()->isHorizontal = cNode.child("IsHorizontalTransition").text().as_bool(); // isHorizontal
            // Transition Type
            if(cNode.child("IsSourceTransition").text().as_bool() == true) pg[pgNum]->all.back()->transitionType = e_source;
            else if(cNode.child("IsFinalTransition").text().as_bool() == true) pg[pgNum]->all.back()->transitionType = e_sink;
            else pg[pgNum]->all.back()->transitionType = e_normal;
            pg[pgNum]->all.back()->condition = cNode.child("TransitionStr").text().as_string(); // Condition
            break;
        }  

        case e_Parallelization:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                                    // Identifier
            pg[pgNum]->all.back()->name = "Parallelization_" + count_type(pg, pg[pgNum]->all.back()->type);    // Name            
            break;
        }  

        case e_Synchronization:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                                    // Identifier
            pg[pgNum]->all.back()->name = "Synchronization_" + count_type(pg, pg[pgNum]->all.back()->type);    // Name            
            break;
        }   

        case e_ConnectionLine:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                         // Identifier
            pg[pgNum]->all.back()->isHorizontal = cNode.child("IsHorizontalLine").text().as_bool(); // isHorizontal                                                   
            // No name for connection lines
            break;
        }     
        
        case e_ContinuousAction:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                             // Identifier
            pg[pgNum]->all.back()->name = "ContinuousAction_" + count_type(pg, pg[pgNum]->all.back()->type);      // Name
            pg[pgNum]->all.back()->action = cNode.child("ActionStr").text().as_string();                // Action
            break;
        }

        case e_ForcingOrder:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                             // Identifier
            pg[pgNum]->all.back()->name = "ForcingOrder_" + count_type(pg, pg[pgNum]->all.back()->type);      // Name
            pg[pgNum]->all.back()->action = cNode.child("ActionStr").text().as_string();                // Action
            break;
        }

        case e_StoredAction:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                             // Identifier
            pg[pgNum]->all.back()->name = "StoredAction_" + count_type(pg, pg[pgNum]->all.back()->type);      // Name
            pg[pgNum]->all.back()->edge = (tempTypeEnum == "ActionActivation") ? e_rising_edge : e_falling_edge; // Edge
            pg[pgNum]->all.back()->action = cNode.child("ActionStr").text().as_string();                // Action
            break;
        }

        case e_ContinuousActionWCondition:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                             // Identifier
            pg[pgNum]->all.back()->name = "ContinuousActionWCondition_" + count_type(pg, pg[pgNum]->all.back()->type);      // Name
            pg[pgNum]->all.back()->condition = cNode.child("ConditionStr").text().as_string();          // Condition
            pg[pgNum]->all.back()->action = cNode.child("ActionStr").text().as_string();                // Action
            break;
        }

        case e_EventAction:        
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                             // Identifier
            pg[pgNum]->all.back()->name = "EventAction_" + count_type(pg, pg[pgNum]->all.back()->type);      // Name
            pg[pgNum]->all.back()->condition = cNode.child("ConditionStr").text().as_string();          // Condition
            pg[pgNum]->all.back()->action = cNode.child("ActionStr").text().as_string();                // Action
            break;
        }

        case e_GroupBorder:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                         // Identifier
            pg[pgNum]->all.back()->name = cNode.child("GroupName").text().as_string();              // Name
            std::string tempCaption = cNode.child("Caption").text().as_string();            
            if(tempCaption != "-") pg[pgNum]->all.back()->stepIdentifier = tempCaption;             // Step Identifier
            break;
        }

        case e_Jump:
        {
            pg[pgNum]->all.back()->identifier = globalID++;                                         // Identifier
            // No name for Jumps
            pg[pgNum]->all.back()->stepIdentifier = cNode.child("Caption").text().as_string();      // Step Identifier
            break;
        }

        default: break;
    }
}

void create_page(std::vector<Page*> &pg, pugi::xml_node pNode)
{
    // Allocate memory for page
    pg.push_back(new Page());
    pg.back()->identifier = globalID++;                                 // Identifier
    pg.back()->name = pNode.child("Title").text().as_string();          // Name
    pg.back()->type = e_Page;                                           // Type
}

void create_symbol(std::vector<Symbol*> &sym, pugi::xml_node sNode)
{
    // Allocate memory for symbol
    sym.push_back(new Symbol());
    sym.back()->name = sNode.child("Symbol").text().as_string();                            // Name
    sym.back()->dataType = symbolDataTypes.at(sNode.child("DataType").text().as_string());  // Data type
    sym.back()->symbolType = symbolTypes.at(sNode.child("SymbolType").text().as_string());  // Symbol Type
}

std::string count_type(std::vector<Page*> pg, ComponentType t)
{
    int count = 0;

    for(Page* p : pg) for(Component* c : p->all) if(c->type == t) count++;

    return std::to_string(count);
}