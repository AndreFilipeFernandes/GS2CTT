//***********************************************************************************
//    GRAFCET MUST BE VALID IN ORDER TO WORK IN THIS PROGRAM
//-----------------------------------------------------------------------------------
//    Connection Lines that intersect each other WILL trip up the program 
//***********************************************************************************

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include "../lib/pugiXML/pugixml.hpp"
#include "../lib/aux_structs/aux_structs.hpp"
#include "../lib/aux_funcs/aux_funcs.hpp"

#define DEBUG // Uncomment to get debug messages

// Source file related vars
std::string xmlSourceDir = "../../shared";  // Source directory for xml files
std::string xmlExtension = ".grafcet";      // GRAFCET-Studio xml extension
std::string xmlFilename = "";               // Name of grafcet XML file

// Destination file related vars
std::string xmlDestinationName = "IntermediateSave.xml";    // Name of xml file for intermediate save file
std::string xmlDestination = std::string("../../shared") + "/" + xmlDestinationName;  // Path to intermediate save file
std::string xmlSymDestinationName = "SymbolSave.xml";     // Name of xml file for symbol save file
std::string xmlSymDestination = std::string("../../shared") + "/" + xmlSymDestinationName;  // Path to symbol save file

// Log files related vars
std::string logDestination = "../log"; // Path to log directory
std::string resultsLogName = "Results.log"; // Name of results log file
std::string resultsLogDestination = logDestination + '/' + resultsLogName; // Path to results log file

// Node related vars for xml search
// Nodes begin on the first element
const char* pageMainNodeRef = "GrafcetsProperties"; // Node for page name section
const char* pageSpecificNodeRef = "GrafcetItemCollectionHelper"; // Node for specific page name section, belongs to "GrafcetsProperties"
const char* grafcetMainNodeRef = "Grafcets"; // Node for the component section
const char* grafcetSpecificNodeRef = "ArrayOfGrafcetItemProperties"; // Node for specific component section, belongs to "Grafcets"
const char* componentNodeRef = "GrafcetItemProperties"; // Node for specific component, belongs to "ArrayOfGrafcetItemProperties"
const char* symbolNodeRef = "Symbols"; // Node for the symbol section
const char* symbolSpecificNodeRef = "SymbolItem"; // Node for specific symbol section

int main()
{    
    for (const auto& entry : std::filesystem::directory_iterator(xmlSourceDir)) 
        if (entry.is_regular_file() && entry.path().extension() == xmlExtension)
            xmlFilename = entry.path().stem().string() + xmlExtension;

    if(xmlFilename == "")
    {
        std::cout << "ERROR - main(): .grafcet file not found, check 'shared' directory." << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        std::cout << "main(): File " << xmlFilename << " found." << std::endl;
    }

    // Add file to source dir string, for easier use going forward
    xmlSourceDir = xmlSourceDir + '/' + xmlFilename;

    pugi::xml_document docIN;
    pugi::xml_parse_result result = docIN.load_file(xmlSourceDir.c_str());

    if(!result)
    {
        std::cout << "ERROR - main(): [" << result.status << "] " << result.description() << "." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): [" << result.status << "] " 
    << result.description() << " - " << xmlFilename << " opened." << std::endl;

    pugi::xml_node mainNode = docIN.child("GrafcetProjectFileSystemV1"); // Node for full file

    // READ FROM XML
    #pragma region
    std::cout << "main(): Reading " << xmlFilename << "..." << std::endl;

    // For every page section
    for(pugi::xml_node pageNodeInstance : mainNode.child(pageMainNodeRef).children(pageSpecificNodeRef))
    {
        // Create page
        create_page(pages, pageNodeInstance);
    }

    int pageNum = 0;

    // For every component section belonging to each page
    for(pugi::xml_node componentNodeInstance : mainNode.child(grafcetMainNodeRef).children(grafcetSpecificNodeRef))
    {        
        // For every specific component belonging to each component page section
        for(pugi::xml_node specificComponentNodeInstance : componentNodeInstance.children(componentNodeRef))
        {            
            // Create component
            create_component(pages, pageNum, specificComponentNodeInstance);       
        }
        
        pageNum++;
    }

    // Reads all symbols
    for(pugi::xml_node symbolNodeInstance : mainNode.child(symbolNodeRef).children(symbolSpecificNodeRef))
    {
        // Create symbol, assume there are always symbols present, because who the hell uses grafcet without symbols?
        create_symbol(symbols, symbolNodeInstance);
    }

    std::cout << "main(): All components and symbols from " << xmlFilename << " read." << std::endl;
    #pragma endregion

    // MAKE ASSOCIATIONS
    #pragma region
    std::cout << "main(): Making component associations..." << std::endl;

    // Action association must come before, otherwise connection line related associations won't work
    for(const auto& p : pages)
    {
        for(const auto& c : p->all)
        {            
            switch(c->type)
            {
                // Check all actions
                case e_ContinuousAction:
                case e_ContinuousActionWCondition:
                case e_StoredAction:
                case e_EventAction:
                case e_ForcingOrder:
                {
                    check_action(*p, *c);
                    break;
                }
            
                default: break;
            }                
        }
    }

    for(const auto& p : pages)
    {
        for(const auto& c : p->all)
        {
            switch (c->type)
            {
                // Check all group borders (Macros and Enclosed)
                case e_GroupBorder:
                {            
                    check_groupBorder(pages, *p, *c);    
                    break;
                }

                // Check all steps
                case e_NormalStep:
                case e_InitialStep:
                case e_MacroStep:
                case e_EnclosedStep:
                case e_EnclosedInitialStep:
                {
                    check_step(*p, *c);                    
                    break;
                }

                // Check all parallelizations
                case e_Parallelization:
                {
                    check_parallelization(*p, *c);
                    break;
                }

                // Check all synchronizations
                case e_Synchronization:
                {
                    check_synchronization(*p, *c);
                    break;
                }

                // Check all jumps
                case e_Jump:
                {
                    check_jump(pages, *p, *c);
                    break;
                }

                default: break;
            }
        }
    }    

    for(const auto& p : pages)
        for(const auto& c : p->all)
            if(c->type == e_Transition)
                recheck_transition(*p, *c);

    std::cout << "main(): Component associations made." << std::endl;
    #pragma endregion

    #ifdef DEBUG

    std::ofstream resultsFile(resultsLogDestination);

    std::ostream& out = resultsFile.is_open() ? resultsFile : std::cout;

    if (!resultsFile.is_open())
    {
        std::cout << "ERROR - main(): Could not open " << resultsLogName
        << ", printing results to console" << std::endl;
    }
    else
    {    
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        resultsFile << "Created: " << std::put_time(std::localtime(&now_c),"%Y-%m-%d %H:%M:%S") << std::endl;
        std::cout << "main(): " << resultsLogName << " opened." << std::endl;
    }

    for(const auto& p : pages)
    {
        out << "===================================================================================================="
        << std::endl << "====================================================================================================" << std::endl
        << p->identifier << " | " 
        <<grafcetTypesR.at(p->type) << " - " 
        << p->name << std::endl;

        for(const auto& c : p->all)
        {
            // Skips undesirable components
            if(c->type == e_ConnectionLine || c->type == e_Jump || c->type == e_GroupBorder) continue;

            out << "----------------------------------------------------------------------------------------------------" << std::endl
            << c->identifier << " | " 
            << grafcetTypesR.at(c->type) << " - " 
            << c->name << std::endl 
            << "    X: " << c->positionX 
            << "; Y: " << c->positionY 
            << "; W: " << c->width
            << "; H: " << c->height << std::endl;

            switch (c->type)
            {           
                case e_NormalStep:
                case e_InitialStep:
                case e_MacroStep:
                case e_EnclosedStep:
                case e_EnclosedInitialStep:
                {                    
                    out << "    isInitial: " << (c->isInitial ? "true":"false")
                    << "; hasAction: " <<  (c->hasAction ? "true":"false") << std::endl
                    << "    associatedPage: " << c->associatedPage->identifier;                                                   
                    break;
                }
                
                case e_Transition:
                {
                    out << "    isHorizontal: " << (c->isHorizontal ? "true":"false")
                    << "; transitionType: " << transitionTypes.at(c->transitionType)
                    << "; condition: " << c->condition << std::endl
                    << "    associatedPage: " << c->associatedPage->identifier;
                    break;
                }

                case e_StoredAction:
                {
                    out << "    action: " << c->action
                    << "; edge: " << actionEdgeTypes.at(c->edge) 
                    << "; condition: " <<  c->condition << std::endl
                    << "    associatedPage: " << c->associatedPage->identifier
                    << "; associatedStep: " << c->associatedStep->identifier;
                    break;
                }

                case e_ContinuousActionWCondition:                
                case e_EventAction:
                {
                    out << "    action: " << c->action
                    << "; condition: " <<  c->condition << std::endl
                    << "    associatedPage: " << c->associatedPage->identifier
                    << "; associatedStep: " << c->associatedStep->identifier;
                    break;
                }

                case e_ContinuousAction:                
                case e_ForcingOrder:
                {
                    out << "    action: " << c->action << std::endl
                    << "    associatedPage: " << c->associatedPage->identifier
                    << "; associatedStep: " << c->associatedStep->identifier;                                    
                    break;
                }

                // Only need the top and bottom prints of the for loop,
                // no component specific ones except the associated page
                case e_Parallelization:
                case e_Synchronization:
                {
                    out << "    associatedPage: " << c->associatedPage->identifier;
                    break;
                } 
            
                default: continue;
            }

            out << std::endl;

            if(c->associatedMacroStep == nullptr)
                out << "    associatedMacroStep: NULL";
            else out << "    associatedMacroStep: " << c->associatedMacroStep->identifier;

            if(c->associatedEnclosedStep == nullptr)
                out << "; associatedEnclosedStep: NULL";
            else out << "; associatedEnclosedStep: " << c->associatedEnclosedStep->identifier; 

            if(c->partialGrafcetName == "") out << "; partialGrafcetName: NULL";
            else out << "; partialGrafcetName: " << c->partialGrafcetName;

            out << std::endl << "    previousComponent:";

            for(const auto& a : c->previousComponent)
            {
                out << " " << a->identifier;
            }
            
            if(c->previousComponent.empty()) out << " NULL";

            out << "; nextComponent:";

            for(const auto& b : c->nextComponent)
            {
                out << " " << b->identifier;
            }

            if(c->nextComponent.empty()) out << " NULL";

            out << std::endl;            
        }
    }

    out << "===================================================================================================="
    << std::endl << "====================================================================================================";

    if(resultsFile.is_open()) std::cout << "main(): " << resultsLogName << " populated." << std::endl;
    else std::cout << std::endl;

    #endif

    // BUILD INTERMEDIATE SAVE FILE
    #pragma region
    std::ofstream interSaveFile(xmlDestination);

    if(!interSaveFile.is_open())
    {
        std::cout << "ERROR - main(): Could not open " << xmlDestinationName << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): " << xmlDestinationName << " opened." << std::endl;

    pugi::xml_document docOUTInter;

    // Add XML declarationInter
    pugi::xml_node declarationInter = docOUTInter.append_child(pugi::node_declaration);
    declarationInter.append_attribute("version") = "1.0";
    declarationInter.append_attribute("encoding") = "UTF-8";    

    // Root node
    pugi::xml_node rootInter = docOUTInter.append_child("GrafcetProject");

    int countPrev;
    int countNext;
    std::string prevStr;
    std::string nextStr;

    for(const auto& p : pages)
    {
        pugi::xml_node pageNode = rootInter.append_child("Page");

        pageNode.append_child("Identifier").text() = p->identifier;
        pageNode.append_child("Name").text() = p->name.c_str();

        pugi::xml_node componentCollection = pageNode.append_child("ComponentCollection");

        for(const auto& c : p->all)
        {
            countPrev = 0;
            countNext = 0;
            prevStr = "";
            nextStr = "";

            if(c->type == e_ConnectionLine || c->type == e_GroupBorder || c->type == e_Jump) continue;

            pugi::xml_node compNode = componentCollection.append_child("Component");
            
            compNode.append_child("Identifier").text() = c->identifier;
            compNode.append_child("Name").text() = c->name.c_str();
            compNode.append_child("Type").text() = grafcetTypesR.at(c->type);

            if(c->partialGrafcetName == "") compNode.append_child("PartialGrafcetName").text() = "NULL";
            else compNode.append_child("PartialGrafcetName").text() = c->partialGrafcetName;

            if(c->associatedMacroStep == nullptr) compNode.append_child("AssociatedMacroStep").text() = "NULL";
            else compNode.append_child("AssociatedMacroStep").text() = c->associatedMacroStep->identifier;

            if(c->associatedEnclosedStep == nullptr) compNode.append_child("AssociatedEnclosedStep").text() = "NULL";
            else compNode.append_child("AssociatedEnclosedStep").text() = c->associatedEnclosedStep->identifier;

            if(c->associatedPage == nullptr) compNode.append_child("AssociatedPage").text() = "NULL";
            else compNode.append_child("AssociatedPage").text() = c->associatedPage->identifier;

            if(c->previousComponent.empty()) prevStr = "NULL";
            else for(const auto& prev : c->previousComponent) prevStr = prevStr + std::to_string(prev->identifier) + ",";

            compNode.append_child("PreviousComponents").text() = prevStr;

            if(c->nextComponent.empty()) nextStr = "NULL";
            else for(const auto& next : c->nextComponent) nextStr = nextStr + std::to_string(next->identifier) + ",";

            compNode.append_child("NextComponents").text() = nextStr;

            compNode.append_child("IsInitial").text() = c->isInitial ? "true" : "false";
            compNode.append_child("TransitionType").text() = transitionTypes.at(c->transitionType);
            compNode.append_child("Edge").text() = actionEdgeTypes.at(c->edge);

            if(c->associatedStep == nullptr) compNode.append_child("AssociatedStep").text() = "NULL";
            else compNode.append_child("AssociatedStep").text() = c->associatedStep->identifier;

            compNode.append_child("Action").text() = c->action.c_str();
            compNode.append_child("Condition").text() = c->condition.c_str();
        }
    }

    docOUTInter.save_file(xmlDestination.c_str(), "  ");

    std::cout << "main(): " << xmlDestinationName << " populated." << std::endl;
    #pragma endregion

    // BUILD SYMBOL SAVE FILE
    #pragma region
    std::ofstream symbolSaveFile(xmlSymDestination);

    if (!symbolSaveFile.is_open())
    {
        std::cout << "ERROR - main(): Could not open " << xmlSymDestination << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "main(): " << xmlSymDestinationName << " opened." << std::endl;

    pugi::xml_document docOUTSym;

    // Add XML declaration
    pugi::xml_node declarationSym = docOUTSym.append_child(pugi::node_declaration);
    declarationSym.append_attribute("version") = "1.0";
    declarationSym.append_attribute("encoding") = "UTF-8";

    // Root node
    pugi::xml_node rootSym = docOUTSym.append_child("SymbolList");

    for(Symbol* symbol : symbols)
    {
        pugi::xml_node symNode = rootSym.append_child("Symbol");

        symNode.append_child("Name").text() = symbol->name;
        symNode.append_child("SymType").text() = symbolTypesR.at(symbol->symbolType);
        symNode.append_child("DataType").text() = symbolDataTypesR.at(symbol->dataType);
    }

    docOUTSym.save_file(xmlSymDestination.c_str(), "  ");

    std::cout << "main(): " << xmlSymDestinationName << " populated." << std::endl;
    #pragma endregion

    std::cout << "main(): GRAFCET-Studio Reader program finished. Closing program..." << std::endl;

    return EXIT_SUCCESS;
}