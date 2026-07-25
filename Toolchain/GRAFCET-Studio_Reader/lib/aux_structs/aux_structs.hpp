#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include "../pugiXML/pugixml.hpp"

// Forward declarations
struct Page;
struct Component;

// ID vars
extern int globalID;

enum ComponentType
{
    e_Page,
    e_NormalStep,
    e_InitialStep,
    e_MacroStep,
    e_EnclosedStep,
    e_EnclosedInitialStep,
    e_Transition,
    e_ContinuousAction,
    e_ContinuousActionWCondition,
    e_StoredAction,
    e_EventAction,
    e_ForcingOrder,
    e_Parallelization,
    e_Synchronization,
    e_ConnectionLine,
    e_GroupBorder,
    e_Jump,
};

enum ActionEdge
{
    e_falling_edge,
    e_rising_edge,
};

enum TransitionType
{
    e_normal,
    e_source,
    e_sink,
};

enum SymbolDataType
{
    e_bool,
    e_int16,
    e_int32,
    e_uint8,
    e_uint16,
    e_uint32,
    e_float,
};

enum SymbolType
{
    e_input,
    e_output,
    e_data,
};

// Not the best implementation, brain-dead simple, works, may bloat since not all
// component types use all variables
struct Component 
{
    // Common
    int identifier = 0;
    std::string name = "";
    ComponentType type = e_Page;
    int positionX = 0;
    int positionY = 0;
    int width = 0;
    int height = 0;
    Component* associatedMacroStep = nullptr;
    Component* associatedEnclosedStep = nullptr;
    Page* associatedPage = nullptr;

    // Knot related
    std::vector<Component*> previousComponent;
    std::vector<Component*> nextComponent;
    bool isInitial = false;
    bool hasAction = false;
    bool isHorizontal = false;
    TransitionType transitionType = e_normal; // Yes ... even if it isn't a transition
    
    // Action related
    ActionEdge edge = e_falling_edge;
    Component* associatedStep = nullptr;
    std::string action = "";
    std::string condition = ""; // Using for both actions and transition, yes, it is scuffed

    // Jump & GroupBorder related
    std::string stepIdentifier = "";

    // Serves as partial grafcet name for GroupBorders (USE FOR GROUPBORDER COMPONENTS ONLY)
    std::string partialGrafcetName = "";
};

struct Page
{
    int identifier = 0;
    std::string name = "";
    ComponentType type = e_Page;
    std::vector<Component*> all;
};

struct Symbol
{
    std::string name = "";
    SymbolDataType dataType;
    SymbolType symbolType;
};

extern std::map<std::string, ComponentType> grafcetTypes;
extern std::map<ComponentType, std::string> grafcetTypesR;
extern std::map<TransitionType, std::string> transitionTypes;
extern std::map<ActionEdge, std::string> actionEdgeTypes;
extern std::map<std::string, SymbolDataType> symbolDataTypes;
extern std::map<SymbolDataType, std::string> symbolDataTypesR;
extern std::map<std::string, SymbolType> symbolTypes;
extern std::map<SymbolType, std::string> symbolTypesR;
extern std::vector<Page*> pages;
extern std::vector<Symbol*> symbols;

void create_component(std::vector<Page*> &pg, int pgNum, pugi::xml_node cNode);
void create_page(std::vector<Page*> &pg, pugi::xml_node pNode);
void create_symbol(std::vector<Symbol*> &sym, pugi::xml_node sNode);
std::string count_type(std::vector<Page*> pg, ComponentType t);