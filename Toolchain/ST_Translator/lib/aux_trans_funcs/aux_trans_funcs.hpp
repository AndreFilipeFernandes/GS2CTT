#pragma once
#include <iostream>
#include <filesystem>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <array>
#include <vector>
#include <ctime>
#include "../pugixml/pugixml.hpp"
#include "../inja/inja.hpp"
#include "../nlohmann/json.hpp"
#include "../aux_structs/aux_structs.hpp"

enum FunctionBlockTypeEnum
{
    e_classicalFB,
    e_macroFB,
    e_enclosingFB,
};

extern nlohmann::json data; // Will contain data for populating the PLCOpenXML project template and the Symbol Configuration template
extern std::map<std::string, PartialGrafcetType> functionBlockTypeMap; // Maps name of function block to its type (easier to get values when needed)

extern std::map<std::string, std::string> symbolDataTypeTranslation;

void build_translation_template();
void build_transitions_template(nlohmann::json& templateData, std::vector<ConnectedGrafcet*>& cgList);
void build_steps_template(nlohmann::json& templateData, std::vector<ConnectedGrafcet*>& cgList);
void build_actions_template(nlohmann::json& templateData, std::vector<ConnectedGrafcet*>& cgList);
void build_pou_template(nlohmann::json& templateData, std::string pgName, PartialGrafcetType pgType);
std::string generate_UUID();
std::string parse_conditional_statement(std::string statement, nlohmann::json& fb);
void parse_trigger_statement(std::string& statement, nlohmann::json& fb);
std::string add_trigger(std::string varName, std::string trigType, bool isStep, nlohmann::json& fb);
void parse_time_statement(std::string& statement, nlohmann::json& fb);
std::string add_time_condition(std::string& statement, nlohmann::json& fb, std::string& tonTime,
                               std::string& tofTime, std::string& condition, std::vector<size_t>& indexList);
std::string build_variable_template(const std::string &name);
std::string build_variable_template(const std::string &name, const std::string &type);
std::string build_variable_template(const std::string &name, const std::string &type, const std::string &initValue);
void initialize_common_methods(nlohmann::json &methodList, PartialGrafcetType pgType);
void initialize_method(nlohmann::json &methodList, std::string methodName, std::string methodVarList,
                       std::string methodBody, std::string methodReturnType);
nlohmann::json& find_pou_by_name(nlohmann::json &pouList, const std::string& pouName);
nlohmann::json& find_method_by_name(nlohmann::json &methodList, const std::string& methodName);
std::string find_trigger_variable_by_name(nlohmann::json &methodBody, const std::string& varName, const std::string& trigType);
std::vector<std::string> find_macro_grafcet_entry_and_exit(std::string pgName);
std::vector<std::string> find_enclosing_grafcet_activation_links(std::string pgName);
PartialGrafcetType find_pg_type_by_node_id(int identifier);
std::string find_pou_name_from_step(std::string stepName);
std::string element_name_trim(std::string elementName, ElementType elementType);
std::string add_spaces_to_statements(std::string storedAction);
bool check_if_in_variable_list(nlohmann::json &varList, std::string varName);
void add_to_variable_list(nlohmann::json &varList, std::string varBuiltTemplate);
std::string parse_stored_action(std::string storedAction, nlohmann::json& fb);
std::string parse_forcing_action(std::string forceAction, ActionType forceType, nlohmann::json& fb);
void add_symbols_to_gvl(nlohmann::json &gvl);
void order_POUs(std::vector<std::string>& orderedPOUList, std::vector<ConnectedGrafcet*>& connectedGrafcetList);
void categorize_POUs(std::vector<std::string>& pouNameList, std::vector<PartialGrafcet*>& partialGrafcetList, std::map<std::string, PartialGrafcetType>& pouTypeMap);
std::string get_timestamp();
void build_symbol_configuration_template();
std::string symbol_configuration_build_variable(std::string variableName, std::string variableType, std::string variableAccess);
std::string get_project_file_name();