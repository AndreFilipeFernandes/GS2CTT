#pragma once
#include <vector>
#include "../pugiXML/pugixml.hpp"
#include "../aux_structs/aux_structs.hpp"

// Function declaration
void model_check_errors_map();
void model_check_set_struct_false();
bool model_check_struct_true();
void check_model(pugi::xml_node &fileNode);
void print_partial_grafcets(pugi::xml_node mainNode, std::vector<PartialGrafcet*> &partialGrafcetList);
void print_connected_grafcets(pugi::xml_node mainNode, std::vector<ConnectedGrafcet*> &connectedGrafcetList);
void print_nodes(pugi::xml_node mainNode, std::vector<Node*> &nodeList);

struct ModelCheckErrorVars
{
    bool CommonVariablesAssociation = false;
    bool PartialGrafcetAssociation = false;
    bool NodeArcAssociation = false;
    bool ActionAssociation = false;
    bool ConditionAssociation = false;
    bool ActionListAssociation = false;
    bool ActivationLinkAssociation = false;
    bool SpecificTypeAssociation = false;
};

extern ModelCheckErrorVars MCEV;