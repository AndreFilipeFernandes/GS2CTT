#include <windows.h>
#include <iostream>
#include <fstream>
#include "../lib/pugiXML/pugixml.hpp"
#include "../lib/aux_structs/aux_structs.hpp"
#include "../lib/aux_funcs/aux_funcs.hpp"
#include "../lib/aux_trans_funcs/aux_trans_funcs.hpp"

// Source file related vars
std::string xmlSourceName = "IntermediateSave.xml"; // Name of intermediate save XML file
std::string xmlSourceFile = std::string("../../shared") + "/" + xmlSourceName; // Path to intermediate save file
std::string plcOpenXmlProjectTemplateSourceName = "ProjectTemplate.xml"; // Name of PLCOpenXML template file
std::string plcOpenXmlProjectTemplateSourceFile = std::string("../../templates/ProjectTemplates") + "/" + plcOpenXmlProjectTemplateSourceName; // Path to PLCOpenXML template file
std::string symConfigXmlProjectTemplateSourceName = "SymbolConfigExportTemplate.export"; // Name of symbol configuration template file
std::string symConfigXmlProjectTemplateSourceFile = std::string("../../templates/SymbolConfigTemplates") + "/" + symConfigXmlProjectTemplateSourceName; // Path to symbol configuration template file

// Destination file (meta model) related vars
std::string xmlMMDestinationName = "MetaModel.xml"; // Name of xml file for meta model save file
std::string xmlMMDestination = std::string("../../shared") + "/" + xmlMMDestinationName; // Path to meta model save file
std::string plcOpenXmlProjectName = "CODESYSProject.xml"; // Name of PLCOpenXML file for project save file
std::string plcOpenXmlProjectDestination = std::string("../../shared") + "/" + plcOpenXmlProjectName; // Path to PLCOpenXML project save file
std::string symConfigXmlProjectName = "SymbolConfiguration.xml"; // Name of XML file for symbol configuration save file
std::string symConfigXmlProjectDestination = std::string("../../shared") + "/" + symConfigXmlProjectName; // Path to XML symbol configuration save file

// Node related vars for xml search
// Nodes begin on the first element
const char* projectNodeRef = "GrafcetProject"; // Node for project
const char* pageNodeRef = "Page"; // Node for page section
const char* grafcetMainNodeRef = "ComponentCollection"; // Node for the component collection section
const char* grafcetSpecificNodeRef = "Component"; // Node for specific component section

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    pugi::xml_document docIN;
    pugi::xml_parse_result result = docIN.load_file(xmlSourceFile.c_str());

    if(!result)
    {
        std::cout << "ERROR - main(): [" << result.status << "] " << result.description() << "." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): [" << result.status << "] " 
    << result.description() << " - " << xmlSourceName << " opened." << std::endl;

    // READ FROM FILE INTO META MODEL
    #pragma region
    std::cout << "main(): Reading " << xmlSourceName << "..." << std::endl;
    
    pugi::xml_node mainNode = docIN.child(projectNodeRef); // Node for full file

    std::vector<Node*> tempNodeVector; // Temporary node vector until later associations
    std::vector<ElementLinks*> tempLinks; // Temporary link vector until later associations
    std::vector<PartialGrafcetIdentifications*> tempPGI; // Temporary partial grafcet vector until later associations

    // For every page section
    // Note: Pages are discarded for the purposes of this code
    for(pugi::xml_node pageNodeInstance : mainNode.children(pageNodeRef))
    {
        // For every component section belonging to each page
        for(pugi::xml_node componentNodeInstance : pageNodeInstance.child(grafcetMainNodeRef).children(grafcetSpecificNodeRef))
        {        
            // Reads each node and associations            
            create_element(tempNodeVector, componentNodeInstance, tempLinks, tempPGI);
        }
    }

    std::cout << "main(): All components from " << xmlSourceName << " read." << std::endl;
    #pragma endregion

    // MAKE FURTHER ASSOCIATIONS INTO META MODEL
    #pragma region
    std::cout << "main(): Making meta model associations..." << std::endl;
    std::cout << "main(): Making element to element associations..." << std::endl;

    // Make previous/next associations and action-step associations
    make_arc_associations(tempNodeVector, tempLinks);

    std::cout << "main(): Element to element associations made." << std::endl;
    std::cout << "main(): Making grafcet associations..." << std::endl;

    // Make connected grafcets and partial grafcets
    create_grafcets(tempNodeVector, tempPGI);

    std::cout << "main(): Grafcet associations made." << std::endl;
    std::cout << "main(): Meta model associations made." << std::endl;
    #pragma endregion

    // Cleanup
    #pragma region
    #ifdef DEBUG 
    std::cout << "main(): Deleting temporary link vector 'tempLinks':" << std::endl;
    #endif
    for(auto& link : tempLinks)
    {
        #ifdef DEBUG
        std::cout << "main(): Deleting link with id " << link->id << "." << std::endl;
        #endif
        delete link;
        #ifdef DEBUG
        std::cout << "main(): Link deleted." << std::endl;
        #endif
    }

    tempLinks.clear();
    
    #ifdef DEBUG
    if(tempLinks.empty()) std::cout << "main(): Temporary link vector 'tempLinks' is now empty." << std::endl;
    else std::cout << "ERROR: main(): Temporary link vector 'tempLinks' NOT empty." << std::endl;
    #endif

    #ifdef DEBUG 
    std::cout << "main(): Deleting temporary partial grafcet identifications vector 'tempPGI':" << std::endl;
    #endif
    for(auto& pgi : tempPGI)
    {
        #ifdef DEBUG
        std::cout << "main(): Deleting partial grafcet identifications with id " << pgi->id << "." << std::endl;
        #endif
        // No need to delete pointer to node pgi->associatedStep, since its handled elsewhere (tempNodeVector)
        delete pgi;
        #ifdef DEBUG
        std::cout << "main(): Partial grafcet identifications deleted." << std::endl;
        #endif
    }

    tempPGI.clear();
    
    #ifdef DEBUG
    if(tempPGI.empty()) std::cout << "main(): Temporary partial grafcet identifications vector 'tempPGI' is now empty." << std::endl;
    else std::cout << "ERROR: main(): Temporary partial grafcet identifications vector 'tempPGI' NOT empty." << std::endl;
    #endif

    #ifdef DEBUG 
    std::cout << "main(): Clearing temporary node vector 'tempNodeVector':" << std::endl;
    #endif
    
    tempNodeVector.clear();
    
    #ifdef DEBUG
    if(tempNodeVector.empty()) std::cout << "main(): Temporary node vector 'tempNodeVector' is now empty." << std::endl;
    else std::cout << "ERROR: main(): Temporary node vector 'tempNodeVector' NOT empty." << std::endl;
    #endif
    #pragma endregion

    // "Results" print to ensure meta model is correct (simpler than the one from GRAFCET-Studio_Reader)
    #pragma region
    #ifdef DEBUG
    for(const auto& pgraf : mainGrafcet.partialGrafcets)
    {
        std::cout << "PG" << pgraf->id << ":\n----------" << std::endl
        << pgraf->name << " " << pgraf->type << " ";
        if(pgraf->associatedStep != nullptr) std::cout << pgraf->associatedStep->id << std::endl;
        else std::cout << "NULL" << std::endl;
        for(const auto& cgraf : pgraf->connectedGrafcets)
        {
            std::cout << "|  PG" << pgraf->id << " CG" << cgraf->id << ":" << std::endl;
            for(const auto& node : cgraf->elements)
            {
                std::cout << "|  |  " << node->id << " | " 
                << grafcetTypesR.at(node->mainType) << " |";

                if(node->previousElements.empty()) std::cout << " NA";
                else for(const auto& p : node->previousElements) std::cout << " " << p->id;

                std::cout << " ,";

                if(node->nextElements.empty()) std::cout << " NA";
                else for(const auto& n : node->nextElements) std::cout << " " << n->id;             

                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }
    std::cout << "NO PGs" << ":\n----------" << std::endl;
    for(const auto& cgraf :  mainGrafcet.connectedGrafcets)
    {
        std::cout << "|  CG" << cgraf->id << ":" << std::endl;
        for(const auto& node : cgraf->elements)
        {
            std::cout << "|  |  " << node->id << " | " 
            << grafcetTypesR.at(node->mainType) << " |";

            if(node->previousElements.empty()) std::cout << " NA";
            else for(const auto& p : node->previousElements) std::cout << " " << p->id;

            std::cout << " ,";

            if(node->nextElements.empty()) std::cout << " NA";
            else for(const auto& n : node->nextElements) std::cout << " " << n->id;

            if(!node->expansions.empty()) for(const auto& pgraf : node->expansions) std::cout << " /pgraf:" << pgraf->id << "/";

            std::cout << std::endl;
        }
    }
    #endif
    #pragma endregion

    // Check meta model against the intermediate save
    #pragma region
    #ifdef DEBUG
    std::cout << "main(): Checking internal model for inconsistencies..." << std::endl;
    check_model(docIN);
    std::cout << "main(): Model inconsistency check finished." << std::endl;
    #endif
    #pragma endregion
    
    // Writing meta model to xml
    #pragma region
    std::cout << "main(): Writing meta model to XML..." << std::endl;

    std::ofstream mmSaveFile(xmlMMDestination);

    if(!mmSaveFile.is_open())
    {
        std::cout << "ERROR - main(): Could not open " << xmlMMDestinationName << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): " << xmlMMDestinationName << " opened." << std::endl;

    pugi::xml_document docOUTmm;

    // Add XML declaration
    pugi::xml_node declaration = docOUTmm.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";    

    // Root node
    pugi::xml_node root = docOUTmm.append_child("Grafcet");

    // Partial grafcets
    print_partial_grafcets(root, mainGrafcet.partialGrafcets);

    // "Loose" connected grafcets
    print_connected_grafcets(root, mainGrafcet.connectedGrafcets);

    docOUTmm.save_file(xmlMMDestination.c_str(), "  ");

    std::cout << "main(): Meta model successfully written to XML." << std::endl;
    #pragma endregion

    // Imprinting metamodel onto templates (including OPC-UA), assembling them into CODESYS ST project
    #pragma region
    std::cout << "main(): Building PLCOpenXML and Symbol Configuration XML data structure..." << std::endl;

    build_translation_template();
    build_symbol_configuration_template();

    #ifdef DEBUG
    std::cout << data.dump(4) << std::endl;
    #endif 

    std::cout << "main(): PLCOpenXML and Symbol Configuration XML data structure built." << std::endl;
    
    std::cout << "main(): Building PLCOpenXML template..." << std::endl;

    std::ofstream plcOpenXMLSaveFile(plcOpenXmlProjectDestination);
    inja::Environment env;
    inja::Template projectTemplate = env.parse_template(plcOpenXmlProjectTemplateSourceFile);

    if(!plcOpenXMLSaveFile.is_open())
    {
        std::cout << "ERROR - main(): Could not open " << plcOpenXmlProjectName << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): " << plcOpenXmlProjectName << " opened." << std::endl;

    // Rendered file will become kinda fugly with regards to variables, a problem from the "build_variable_template()" function
    env.render_to(plcOpenXMLSaveFile, projectTemplate, data);

    std::cout << "main(): Building PLCOpenXML template built." << std::endl;

    std::cout << "main(): Building Symbol Configuration XML template..." << std::endl;

    std::ofstream symConfigXMLSaveFile(symConfigXmlProjectDestination);
    inja::Environment env2;
    inja::Template symConfigTemplate = env2.parse_template(symConfigXmlProjectTemplateSourceFile);

    if(!symConfigXMLSaveFile.is_open())
    {
        std::cout << "ERROR - main(): Could not open " << symConfigXmlProjectName << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): " << symConfigXmlProjectName << " opened." << std::endl;

    // Rendered file will become kinda fugly with regards to variables, a problem from the "build_variable_template()" function
    env2.render_to(symConfigXMLSaveFile, symConfigTemplate, data);

    std::cout << "main(): Building Symbol Configuration XML template built." << std::endl;
    #pragma endregion

    return EXIT_SUCCESS;
}