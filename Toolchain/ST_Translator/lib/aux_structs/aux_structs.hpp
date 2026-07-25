#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include "../pugiXML/pugixml.hpp"

//#define DEBUG // Uncomment to get debug messages. Yes, the debug define is here :)

enum ElementType
{
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
};

enum ActionType
{
    // Forcing Order
    explicitSituation,
    currentSituation,
    emptySituation,
    initialSituation,

    // Stored Action
    ActivationAction,
    DeactivationAction,
    EventAction,

    // Continuous Action
    ContinuousActionType,
    ConditionalActionType,
};

enum TransitionType
{
    transition_normal,
    transition_sink,
    transition_source,
};

enum SynchronizationType
{
    Synchronization,
    Parallelization,
};

enum PartialGrafcetType
{
    partial_normal,
    partial_macro,
    partial_enclosing,
};

// Forward declarations
struct PartialGrafcet;
struct Node;

struct PartialGrafcetIdentifications
{
    int id = -1;
    std::string associatedPartialGrafcetName = "";
    std::string associatedStep = "";
    PartialGrafcetType type;
};

struct ElementLinks
{
    int id = -1;
    std::vector<int> previousElements;
    std::vector<int> nextElements;
    int associatedStep = -1;
};

struct Node
{
    // General
    int id = -1; // Identifier
    std::string name = ""; // Name
    ElementType mainType; // Main type according to GRAFCET-STUDIO
    std::vector<Node*> previousElements; // List of previous linked nodes
    std::vector<Node*> nextElements; // List of following linked nodes

    // Transitions & Actions
    std::string condition = ""; // Firing condition for transitions and action conditions
    std::string action = ""; // What the actions do
    TransitionType transitionType; // Type of transition
    ActionType actionType; // Type of action

    // Steps
    std::vector<Node*> actionList; // List of associated actions
    bool activationLink = false; // Used in enclosing step expansions
    bool isInitial = false; // Only for enclosing and normal steps
    std::vector<PartialGrafcet*> expansions; // For macro and enclosing steps; one for macro steps and multiple for enclosing steps

    // Synchronizations
    SynchronizationType syncType; // Type of synchronization
};

struct ConnectedGrafcet
{
    int id = -1;
    std::vector<Node*> elements; // Includes both Nodes and ActionTypes
};

struct PartialGrafcet
{
    int id = -1;
    std::string name = "";
    PartialGrafcetType type;
    Node* associatedStep = nullptr;
    std::vector<ConnectedGrafcet*> connectedGrafcets;
};

struct Grafcet
{
    std::vector<ConnectedGrafcet*> connectedGrafcets;   // Grafcets with no partial grafcet associated
    std::vector<PartialGrafcet*> partialGrafcets;       // Includes Macro and Enclosing steps expansions
};

extern Grafcet mainGrafcet;

extern std::map<std::string, ElementType> grafcetTypes;
extern std::map<ElementType, std::string> grafcetTypesR;
extern std::map<PartialGrafcetType, std::string> partialGrafcetTypesR;
extern std::map<TransitionType, std::string> transitionTypesR;
extern std::map<ActionType, std::string> actionTypesR;
extern std::map<SynchronizationType, std::string> synchronizationTypesR;

void create_element(std::vector<Node*> &nodeVector, pugi::xml_node cNode, std::vector<ElementLinks*> &links, std::vector<PartialGrafcetIdentifications*> &PGIs);
void element_arc_search(std::vector<int> &arcList, std::string arcStr);
void make_arc_associations(std::vector<Node*> &nodes, std::vector<ElementLinks*> &links);
void create_grafcets(std::vector<Node*> &nodeVector, std::vector<PartialGrafcetIdentifications*> &PGIVector);
void create_connected_grafcets(std::vector<ConnectedGrafcet*> &connectedGrafcetList, std::vector<Node*> &nodeVector);
void connected_grafcet_element_association(Node* &node, std::vector<Node*> &nodeList);
void element_arc_search(std::vector<int> &arcList, std::string arcStr);
void make_arc_associations(std::vector<Node*> &nodes, std::vector<ElementLinks*> &links);
Node* node_pointer_by_id(std::vector<Node*> &nodes, int nodeID);
Node* node_pointer_by_name(std::vector<Node*> &nodes, std::string nodeName);