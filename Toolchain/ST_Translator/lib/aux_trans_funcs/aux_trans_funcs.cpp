#include "aux_trans_funcs.hpp"

nlohmann::json data; // Main json data structure

// Counter for triggers and time-related variables
int rTriggerNumber = 1;
int fTriggerNumber = 1;
int tOnNumber = 1;
int tOfNumber = 1;
int tCondNumber = 1;

// From Grafcet-Studio to CODESYS
std::map<std::string, std::string> symbolDataTypeTranslation = {
    {"BOOL", "BOOL"},
    {"INT16", "INT"},
    {"INT32", "DINT"},
    {"UINT8", "USINT"},
    {"UINT16", "UINT"},
    {"UINT32", "UDINT"},
    {"FLOAT", "REAL"},
};

// Builds main translation template data structure "data"
void build_translation_template()
{
    // Preliminary data definitions
    // Project specific
    data["project_name"] = get_project_file_name();
    data["time_of_creation"] = get_timestamp();

    // Task specific
    data["task"]["name"] = "GrafcetTask";
    data["task"]["UUID"] = generate_UUID();

    // Main program specific
    data["plcprg"]["name"] = "PLC_PRG";
    data["plcprg"]["UUID"] = generate_UUID();

    // Global Variable List specific
    data["gvl"]["name"] = "GVL";
    data["gvl"]["UUID"] = generate_UUID();
    data["gvl"]["variable_list"]["others"] = nlohmann::json::array();
    add_symbols_to_gvl(data["gvl"]); // Add symbols in "SymbolSave.xml" to the GVL
    
    // Misc
    data["libman_UUID"] = generate_UUID();
    data["application_UUID"] = generate_UUID();
    data["device_UUID"] = generate_UUID();

    // Init main grafcet
    build_pou_template(data, "MainGrafcet", partial_normal);

    std::vector<std::string> orderedPOUList;
    std::map<std::string, PartialGrafcetType> functionBlockTypeMap;

    std::cout << "build_translation_template(): Creating POUs..." << std::endl;

    orderedPOUList.push_back("MainGrafcet");
    functionBlockTypeMap.insert({"MainGrafcet", partial_normal});
    
    // Add partial grafcets of type "normal" to both the ordered list and the type map
    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        if(pg->type == partial_normal)
        {
            orderedPOUList.push_back(pg->name);
            functionBlockTypeMap.insert({pg->name, pg->type});
        }
    }   
    
    std::vector<std::vector<ConnectedGrafcet*>> tempConnectedList;
    tempConnectedList.push_back(mainGrafcet.connectedGrafcets);

    for(const auto& OPLElement : orderedPOUList)
        for(const auto& pg : mainGrafcet.partialGrafcets)
            if(OPLElement == pg->name) tempConnectedList.push_back(pg->connectedGrafcets);

    for(auto& TCLElement : tempConnectedList)            
        order_POUs(orderedPOUList, TCLElement);

    categorize_POUs(orderedPOUList, mainGrafcet.partialGrafcets, functionBlockTypeMap);

    // Init partial grafcets
    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        for(const auto& pouName : orderedPOUList)
        {
            if(pouName != pg->name) continue;
            build_pou_template(data, pg->name, pg->type);
        }
    }

    std::cout << "build_translation_template(): POUs created." << std::endl;

    std::cout << "build_translation_template(): Populating POUs..." << std::endl;

    // Check all connected grafcets from maingrafcet and lump them together
    nlohmann::json& mainGrafcetTemplateData = find_pou_by_name(data["pou_list"], "MainGrafcet");
    std::vector<ConnectedGrafcet*>& tempConnectedGrafcetList = mainGrafcet.connectedGrafcets;
    build_transitions_template(mainGrafcetTemplateData, tempConnectedGrafcetList);    
    build_steps_template(mainGrafcetTemplateData, tempConnectedGrafcetList);
    nlohmann::json* tempSEH = &find_method_by_name(mainGrafcetTemplateData["method_list"], "stateEvolutionHandler");
    std::string tempVarStr = mainGrafcetTemplateData["misc_variable_list"].dump();
    
    if(tempVarStr.find("forcedFlag") == std::string::npos) // Ensure forcedFlag exists regardless (can do better, code too chunky as it stands)
        add_to_variable_list(mainGrafcetTemplateData["misc_variable_list"], build_variable_template("forcedFlag", "BOOL", "FALSE"));
    if((*tempSEH)["body"].size() == 1)
    {
        (*tempSEH)["body"].push_back("    // Nothingness...\n");
        (*tempSEH)["body"].push_back("END_IF;\nstateEvolutionHandler := stableState;");
    }
    else
        (*tempSEH)["body"].push_back("END_IF;\nstateEvolutionHandler := stableState;");

    // Check all partial grafcets from maingrafcet
    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        nlohmann::json& pgTemplateData = find_pou_by_name(data["pou_list"], pg->name);
        build_transitions_template(pgTemplateData, pg->connectedGrafcets);
        build_steps_template(pgTemplateData, pg->connectedGrafcets);
        tempSEH = &find_method_by_name(pgTemplateData["method_list"], "stateEvolutionHandler");
        tempVarStr = pgTemplateData["misc_variable_list"].dump();
        if(tempVarStr.find("forcedFlag") == std::string::npos) // Ensure forcedFlag exists regardless (can do better, code too chunky as it stands)
            add_to_variable_list(pgTemplateData["misc_variable_list"], build_variable_template("forcedFlag", "BOOL", "FALSE"));
        if((*tempSEH)["body"].size() == 1)
        {
            (*tempSEH)["body"].push_back("    // Nothingness...\n");
            (*tempSEH)["body"].push_back("END_IF;\nstateEvolutionHandler := stableState;");
        }
        else
            (*tempSEH)["body"].push_back("END_IF;\nstateEvolutionHandler := stableState;");
        
        std::string stepDeactivationList = "";
        for(const auto& stepDeclaration : pgTemplateData["step_variable_list"])
        {
            std::string tempStepDeclaration = stepDeclaration;
            std::string tempStepStr = "";
            for(int i=tempStepDeclaration.find('S') ; i<tempStepDeclaration.find('"', tempStepDeclaration.find('S')) ; i++)
                tempStepStr.push_back(tempStepDeclaration[i]);
            stepDeactivationList.append("    " + tempStepStr + " := FALSE;\n");
        }
        
        stepDeactivationList.push_back(' ');
        stepDeactivationList.erase(0, 1); // Erases extra space at begining of string (no idea where it comes from lol)

        nlohmann::json* tempMethod = nullptr;
        try
        {
            tempMethod = &find_method_by_name(pgTemplateData["method_list"], "initializeAndStop");
            std::string tempMethodStr = (*tempMethod)["body"][0];
            int insertPosition = tempMethodStr.find("\n")+2; // +2 because of "\n"
            tempMethodStr.insert(insertPosition, stepDeactivationList);
            (*tempMethod)["body"].clear();
            (*tempMethod)["body"].push_back(tempMethodStr);
        }
        catch(const std::exception& e){}        
    }

    // Actions must be build afterwards as they may depend on the variable lists of other POUs
    build_actions_template(mainGrafcetTemplateData, tempConnectedGrafcetList);
    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        nlohmann::json& pgTemplateData = find_pou_by_name(data["pou_list"], pg->name);
        build_actions_template(pgTemplateData, pg->connectedGrafcets);
    }

    for(const auto& pou : data["pou_list"])
        for(const auto& line : pou["step_variable_list"])
            data["gvl"]["variable_list"]["others"].push_back(line.get<std::string>());

    std::cout << "build_translation_template(): POUs populated." << std::endl;

    data["plcprg"]["body"]["fb_triggerHandler"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_stepTriggerHandler"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_timedConditionHandler"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_storedActionHandler"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_activateExpansions"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_evaluateTransitions"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_evaluateFireableTransitions"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_stateEvolutionHandler"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_pullDownSignals"] = nlohmann::json::array();
    data["plcprg"]["body"]["fb_continuousActions"] = nlohmann::json::array();

    // Populate PLC_PRG body
    for(const auto& pou : data["pou_list"])
    {
        std::string pouName = pou["name"];
        std::string pouNameDeclaration = "fb_" + pouName;

        for(const auto& method : pou["method_list"])
        {
            std::string methodName = method["name"];
            std::string methodNameDeclaration = "fb_" + methodName;

            if(methodNameDeclaration == "fb_triggerHandler") // fb_triggerHandler
                data["plcprg"]["body"]["fb_triggerHandler"].push_back(pouNameDeclaration + "." + methodName + "();");
            if(methodNameDeclaration == "fb_stepTriggerHandler") // fb_stepTriggerHandler
                data["plcprg"]["body"]["fb_stepTriggerHandler"].push_back(pouNameDeclaration + "." + methodName + "();");                
            else if(methodNameDeclaration == "fb_timedConditionHandler") // fb_timedConditionHandler
                data["plcprg"]["body"]["fb_timedConditionHandler"].push_back(pouNameDeclaration + "." + methodName + "();");
            else if(methodNameDeclaration == "fb_storedActionHandler") // fb_storedActionHandler
                data["plcprg"]["body"]["fb_storedActionHandler"].push_back(pouNameDeclaration + "." + methodName + "();");
            else if(methodNameDeclaration == "fb_activateExpansions") // fb_activateExpansions
                data["plcprg"]["body"]["fb_activateExpansions"].push_back(pouNameDeclaration + "." + methodName + "();");
            else if(methodNameDeclaration == "fb_evaluateTransitions") // fb_evaluateTransitions
                data["plcprg"]["body"]["fb_evaluateTransitions"].push_back(pouNameDeclaration + "." + methodName + "();");
            else if(methodNameDeclaration == "fb_evaluateFireableTransitions") // fb_evaluateFireableTransitions
                data["plcprg"]["body"]["fb_evaluateFireableTransitions"].push_back(pouNameDeclaration + "." + methodName + "();");
            else if(methodNameDeclaration == "fb_stateEvolutionHandler") // fb_stateEvolutionHandler
                data["plcprg"]["body"]["fb_stateEvolutionHandler"].push_back("globalStableState := " + pouNameDeclaration + "." + methodName + "(stableState := globalStableState);");
            else if(methodNameDeclaration == "fb_pullDownSignals") // fb_pullDownSignals
                data["plcprg"]["body"]["fb_pullDownSignals"].push_back(pouNameDeclaration + "." + methodName + "();");
            else if(methodNameDeclaration == "fb_continuousActions") // fb_continuousActions
                data["plcprg"]["body"]["fb_continuousActions"].push_back(pouNameDeclaration + "." + methodName + "();");
        }
    }

    return;
}

// Builds translation template data structure for POU "templateData" for transitions from a given
// connected grafcet list "cgList"
void build_transitions_template(nlohmann::json& templateData, std::vector<ConnectedGrafcet*>& cgList)
{
    #ifdef DEBUG
    std::cout << "build_transitions_template(): Building transition template for POU named '" << templateData["name"].dump() << "'..." << std::endl;
    #endif

    // Ensures no "does not exist" errors from inja
    templateData["transition_variable_list"] = nlohmann::json::array();

    for(const auto& cg : cgList)
    {
        for(const auto& elem : cg->elements)
        {
            if(elem->mainType == e_Transition) // Rules 6, 7 and 8
            {
                int fireableTransitionNumber = 1;
                std::string transitionName = element_name_trim(elem->name, elem->mainType);
                std::string nextElementsStr = "";

                add_to_variable_list(templateData["transition_variable_list"], build_variable_template(transitionName, "BOOL"));
                find_method_by_name(templateData["method_list"], "evaluateTransitions")["body"].push_back(
                    transitionName + " := " + parse_conditional_statement(elem->condition, templateData) + ";\n");

                for(const auto& nextElem : elem->nextElements)
                {
                    if(nextElem->mainType == e_Parallelization)
                    {
                        for(const auto& nextElem2 : nextElem->nextElements)
                            nextElementsStr.append("        " + element_name_trim(nextElem2->name, nextElem2->mainType) + " := TRUE;\n");

                        continue;
                    }
                    
                    nextElementsStr.append("        " + element_name_trim(nextElem->name, nextElem->mainType) + " := TRUE;\n");
                }

                if(elem->transitionType == transition_normal
                || elem->transitionType == transition_sink)
                {
                    for(const auto& prevElem : elem->previousElements)
                    {
                        std::string fireableTransitionName = "F" + transitionName + "_" + std::to_string(fireableTransitionNumber);
                        std::string fireableTransitionTrigName = fireableTransitionName + "TRIG";
                        
                        add_to_variable_list(templateData["transition_variable_list"], build_variable_template(fireableTransitionName, "BOOL"));
                        add_to_variable_list(templateData["misc_variable_list"], build_variable_template(fireableTransitionTrigName, "R_TRIG"));

                        fireableTransitionNumber++;

                        if(prevElem->mainType == e_MacroStep)
                        {
                            find_method_by_name(templateData["method_list"], "evaluateFireableTransitions")["body"].push_back(
                                fireableTransitionTrigName + "(CLK:=" + transitionName + " AND "
                                + element_name_trim(prevElem->name, prevElem->mainType)
                                + " AND fb_" + prevElem->expansions[0]->name + ".getFinishedFlag());\n"
                                + fireableTransitionName + " := " + fireableTransitionTrigName + ".Q;\n");

                            find_method_by_name(templateData["method_list"], "stateEvolutionHandler")["body"].push_back(
                                "    IF " + fireableTransitionName + " THEN\n        stableState := FALSE;\n        "
                                + element_name_trim(prevElem->name, prevElem->mainType) + " := FALSE;\n"
                                + nextElementsStr + "    END_IF;\n");
                            
                            #ifdef DEBUG
                            std::cout << "build_transitions_template(): POU '" << templateData["name"] << "', applied rule 8." << std::endl;
                            std::cout << "                              Macrostep named '" << prevElem->name << "'." << std::endl;
                            #endif
                        }

                        if(prevElem->mainType == e_Synchronization)
                        {
                            std::string prevElementsStrEFT = "";
                            std::string prevElementsStrSEH = "";

                            for(const auto& prevStep : prevElem->previousElements)
                            {
                                prevElementsStrEFT.append(" AND " + element_name_trim(prevStep->name, prevStep->mainType));
                                prevElementsStrSEH.append("        " + element_name_trim(prevStep->name, prevStep->mainType) + " := FALSE;\n");
                            }

                            find_method_by_name(templateData["method_list"], "evaluateFireableTransitions")["body"].push_back(
                                fireableTransitionTrigName + "(CLK:=" + transitionName + prevElementsStrEFT + ");\n"
                                + fireableTransitionName + " := " + fireableTransitionTrigName + ".Q;\n");

                            find_method_by_name(templateData["method_list"], "stateEvolutionHandler")["body"].push_back(
                                "    IF " + fireableTransitionName + " THEN\n        stableState := FALSE;\n"
                                + prevElementsStrSEH + nextElementsStr + "    END_IF;\n");
                            
                            #ifdef DEBUG
                            std::cout << "build_transitions_template(): POU '" << templateData["name"] << "', applied rule 7." << std::endl;
                            std::cout << "                              Sync/Parl named '" << prevElem->name << "'." << std::endl;
                            #endif
                        }

                        if(prevElem->mainType == e_NormalStep || prevElem->mainType == e_InitialStep
                        || prevElem->mainType == e_EnclosedStep || prevElem->mainType == e_EnclosedInitialStep)
                        {
                            find_method_by_name(templateData["method_list"], "evaluateFireableTransitions")["body"].push_back(
                                fireableTransitionTrigName + "(CLK:=" + transitionName + " AND "
                                + element_name_trim(prevElem->name, prevElem->mainType) + ");\n"
                                + fireableTransitionName + " := " + fireableTransitionTrigName + ".Q;\n");

                            std::string strCapSEH = "";
                            if(nextElementsStr.find("SS") != std::string::npos) // Very scuffed implementation, if is transition FROM final step to another it breaks :)
                                strCapSEH = "        finishedFlag := TRUE;\n    END_IF;\n";
                            else
                                strCapSEH = "    END_IF;\n";

                            find_method_by_name(templateData["method_list"], "stateEvolutionHandler")["body"].push_back(
                                "    IF " + fireableTransitionName + " THEN\n        stableState := FALSE;\n        "
                                + element_name_trim(prevElem->name, prevElem->mainType) + " := FALSE;\n"
                                + nextElementsStr + strCapSEH);
                            
                            #ifdef DEBUG
                            std::cout << "build_transitions_template(): POU '" << templateData["name"] << "', applied rule 6." << std::endl;
                            std::cout << "                              Step named '" << prevElem->name << "' of type '" << grafcetTypesR.at(prevElem->mainType) << "'." << std::endl;
                            #endif
                        }
                    }
                }
                else // If elem->transitionType == transition_source
                {
                    std::string fireableTransitionName = "F" + transitionName + "_" + std::to_string(fireableTransitionNumber);
                    std::string fireableTransitionTrigName = fireableTransitionName + "TRIG";
                        
                    add_to_variable_list(templateData["transition_variable_list"], build_variable_template(fireableTransitionName, "BOOL"));
                    add_to_variable_list(templateData["misc_variable_list"], build_variable_template(fireableTransitionTrigName, "R_TRIG"));

                    fireableTransitionNumber++;

                    find_method_by_name(templateData["method_list"], "evaluateFireableTransitions")["body"].push_back(
                        fireableTransitionTrigName + "(CLK:=" + transitionName + ");\n"
                        + fireableTransitionName + " := " + fireableTransitionTrigName + ".Q;\n");

                    find_method_by_name(templateData["method_list"], "stateEvolutionHandler")["body"].push_back(
                        "    IF " + fireableTransitionName + " THEN\n        stableState := FALSE;\n"
                        + nextElementsStr + "    END_IF;\n");

                    #ifdef DEBUG
                    std::cout << "build_transitions_template(): POU '" << templateData["name"] << "', applied rule SOURCE_TRANSITION." << std::endl;
                    #endif
                }
            }
        }
    }
    
    #ifdef DEBUG
    std::cout << "build_transitions_template(): Transition template for POU named '" << templateData["name"].dump() << "' built." << std::endl;
    #endif

    return;
}

// Builds translation template data structure for POU "templateData" for steps from a given
// connected grafcet list "cgList"
void build_steps_template(nlohmann::json& templateData, std::vector<ConnectedGrafcet*>& cgList)
{
    #ifdef DEBUG
    std::cout << "build_steps_template(): Building step template for POU named '" << templateData["name"].dump() << "'..." << std::endl;
    #endif

    PartialGrafcetType pgType = find_pg_type_by_node_id(cgList[0]->elements[0]->id); // Any element works
    std::vector<std::string> enclosingInitialStepList, enclosingActLinkStepList;

    for(const auto& cg : cgList)
    {
        for(const auto& elem : cg->elements)
        {
            switch(elem->mainType)
            {
                case e_NormalStep:
                case e_InitialStep:
                {
                    std::string stepName = element_name_trim(elem->name, elem->mainType);

                    if(stepName[1] == 'E') // Macrostep expansion entry step
                    {
                        nlohmann::json* tempMethod = &find_method_by_name(templateData["method_list"], "activate");
                        std::string tempStr = "IF active = FALSE THEN\n    " + stepName + " := TRUE;\nEND_IF;\n";
                        for (const auto& line : (*tempMethod)["body"])
                            tempStr += line.get<std::string>();
                        (*tempMethod)["body"].clear();
                        (*tempMethod)["body"].push_back(tempStr);
                    }

                    if(pgType == partial_enclosing)
                    {
                        if(elem->isInitial)
                        {
                            std::string tempStr = element_name_trim(elem->name, elem->mainType) + " := TRUE;\n";
                            enclosingInitialStepList.push_back(tempStr);
                        }

                        if(elem->activationLink)
                        {
                            std::string tempStr = element_name_trim(elem->name, elem->mainType) + " := TRUE;\n";
                            enclosingActLinkStepList.push_back(tempStr);
                        }
                    }

                    if(elem->isInitial && pgType != partial_enclosing)
                    {
                        add_to_variable_list(templateData["step_variable_list"], 
                            build_variable_template(stepName, "BOOL", "TRUE"));
                        
                        #ifdef DEBUG
                        std::cout << "build_steps_template(): POU '" << templateData["name"] << "', applied rule 2." << std::endl;
                        std::cout << "                        Step named '" << elem->name << "'." << std::endl;
                        #endif
                    }
                    else
                    {
                        add_to_variable_list(templateData["step_variable_list"], 
                            build_variable_template(stepName, "BOOL"));

                        #ifdef DEBUG
                        std::cout << "build_steps_template(): POU '" << templateData["name"] << "', applied rule 1." << std::endl;
                        std::cout << "                        Step named '" << elem->name << "'." << std::endl;
                        #endif
                    }

                    break;
                }

                case e_EnclosedStep:
                case e_EnclosedInitialStep:
                {
                    std::string stepName = element_name_trim(elem->name, elem->mainType);
                    nlohmann::json* tempAE = nullptr;
                    
                    try
                        {tempAE = &find_method_by_name(templateData["method_list"], "activateExpansions");}
                    catch(const std::exception& e)
                    {
                        initialize_method(templateData["method_list"], "activateExpansions", "", "", "");
                        tempAE = &find_method_by_name(templateData["method_list"], "activateExpansions");
                    }

                    (*tempAE)["body"].push_back(
                        "IF " + stepName + " THEN\n");
                        
                    for(const auto& expansion : elem->expansions)
                    {
                        (*tempAE)["body"].push_back(
                            "    fb_" + expansion->name + ".activate();\n"
                        );
                    }

                    (*tempAE)["body"].push_back("ELSE\n");

                    for(const auto& expansion : elem->expansions)
                    {
                        (*tempAE)["body"].push_back(
                            "    fb_" + expansion->name + ".initializeAndStop();\n"
                        );
                    }

                    (*tempAE)["body"].push_back("END_IF;\n");

                    if(pgType == partial_enclosing)
                    {
                        if(elem->isInitial)
                        {
                            std::string tempStr = element_name_trim(elem->name, elem->mainType) + " := TRUE;\n";
                            enclosingInitialStepList.push_back(tempStr);
                        }

                        if(elem->activationLink)
                        {
                            std::string tempStr = element_name_trim(elem->name, elem->mainType) + " := TRUE;\n";
                            enclosingActLinkStepList.push_back(tempStr);
                        }
                    }

                    if(elem->isInitial)
                    {
                        add_to_variable_list(templateData["step_variable_list"], 
                            build_variable_template(stepName, "BOOL", "TRUE"));

                        #ifdef DEBUG
                        std::cout << "build_steps_template(): POU '" << templateData["name"] << "', applied rule 5." << std::endl;
                        std::cout << "                        Step named '" << elem->name << "'." << std::endl;
                        #endif
                    }
                    else
                    {
                        add_to_variable_list(templateData["step_variable_list"], 
                            build_variable_template(stepName, "BOOL"));

                        #ifdef DEBUG
                        std::cout << "build_steps_template(): POU '" << templateData["name"] << "', applied rule 4." << std::endl;
                        std::cout << "                        Step named '" << elem->name << "'." << std::endl;
                        #endif
                    }

                    break;
                }

                case e_MacroStep:
                {
                    std::string stepName = element_name_trim(elem->name, elem->mainType);
                    std::string associatedGrafcetName = elem->expansions[0]->name;
                    nlohmann::json* tempAE = nullptr;

                    try
                        {tempAE = &find_method_by_name(templateData["method_list"], "activateExpansions");}
                    catch(const std::exception& e)
                    {
                        initialize_method(templateData["method_list"], "activateExpansions", "", "", "");
                        tempAE = &find_method_by_name(templateData["method_list"], "activateExpansions");
                    }
                    
                    (*tempAE)["body"].push_back(
                        "IF " + stepName + " THEN\n" +
                        "    fb_" + associatedGrafcetName + ".activate();\nELSE\n" +
                        "    fb_" + associatedGrafcetName + ".initializeAndStop();\nEND_IF;\n"
                    );

                    if(pgType == partial_enclosing)
                    {
                        if(elem->isInitial)
                        {
                            std::string tempStr = element_name_trim(elem->name, elem->mainType) + " := TRUE;\n";
                            enclosingInitialStepList.push_back(tempStr);
                        }

                        if(elem->activationLink)
                        {
                            std::string tempStr = element_name_trim(elem->name, elem->mainType) + " := TRUE;\n";
                            enclosingActLinkStepList.push_back(tempStr);
                        }
                    }

                    add_to_variable_list(templateData["step_variable_list"], 
                        build_variable_template(stepName, "BOOL"));

                    #ifdef DEBUG
                    std::cout << "build_steps_template(): POU '" << templateData["name"] << "', applied rule 3." << std::endl;
                    std::cout << "                        Step named '" << elem->name << "'." << std::endl;
                    #endif

                    break;
                }

                default: break;
            }
        }
    }

    if(pgType == partial_enclosing)
    {
        nlohmann::json* tempA = nullptr;
        std::string tempStr = "IF active = FALSE THEN\n    IF firstActivation THEN\n        firstActivation := FALSE;\n";
        std::string tempAStr = "";

        try
            {tempA = &find_method_by_name(templateData["method_list"], "activate");}
        catch(const std::exception& e)
        {
            initialize_method(templateData["method_list"], "activate", "", "active := TRUE;", "");
            tempA = &find_method_by_name(templateData["method_list"], "activate");
        }

        for (const auto& line : (*tempA)["body"])
            tempAStr += line.get<std::string>();

        for(const auto& str : enclosingInitialStepList)
            tempStr.append("        " + str);

        if(enclosingInitialStepList.empty())
        {
            for(const auto& str : enclosingActLinkStepList)
                tempStr.append("        " + str);
        }

        tempStr.append("    ELSE\n");

        for(const auto& str : enclosingActLinkStepList)
            tempStr.append("        " + str);

        tempStr.append("    END_IF;\nEND_IF;\n");
        tempStr.append(tempAStr);

        (*tempA)["body"].clear();
        (*tempA)["body"].push_back(tempStr);
    }

    #ifdef DEBUG
    std::cout << "build_steps_template(): Step template for POU named '" << templateData["name"].dump() << "' built." << std::endl;
    #endif

    return;
}

// Builds translation template data structure for POU "templateData" for actions from a given
// connected grafcet list "cgList"
void build_actions_template(nlohmann::json& templateData, std::vector<ConnectedGrafcet*>& cgList)
{
    #ifdef DEBUG
    std::cout << "build_steps_template(): Building action template for POU named '" << templateData["name"].dump() << "'..." << std::endl;
    #endif

    try
        {find_method_by_name(templateData["method_list"], "pullDownSignals");}
    catch(const std::exception& e)
        {initialize_method(templateData["method_list"], "pullDownSignals", "", "", "");}

    try
        {find_method_by_name(templateData["method_list"], "continuousActions");}
    catch(const std::exception& e)
        {initialize_method(templateData["method_list"], "continuousActions", "", "", "");}

    try
        {find_method_by_name(templateData["method_list"], "storedActionHandler");}
    catch(const std::exception& e)
        {initialize_method(templateData["method_list"], "storedActionHandler", "", "", "");}

    // Super nesting from hell ahead
    for(const auto& cg : cgList)
    {
        for(const auto& elem : cg->elements)
        {
            // This double switch-case makes me want to die, but given the structure and the required order
            // it must be done
            switch(elem->mainType)
            {
                case e_NormalStep:
                case e_InitialStep:
                case e_MacroStep:
                case e_EnclosedStep:
                case e_EnclosedInitialStep:
                {
                    for(const auto& actionElem : elem->actionList)
                    {
                        switch(actionElem->mainType)
                        {
                            case e_ContinuousAction: // Rule 9
                            case e_ContinuousActionWCondition: // Rule 10
                            {
                                nlohmann::json* tempJsonPD = &find_method_by_name(templateData["method_list"], "pullDownSignals");
                                nlohmann::json* tempJsonCA = &find_method_by_name(templateData["method_list"], "continuousActions");
                                std::string tempStrPDBody = (*tempJsonPD)["body"].dump(4);

                                // If not present in "pullDownSignals" body, add
                                if(tempStrPDBody.find(actionElem->action) == std::string::npos)
                                    (*tempJsonPD)["body"].push_back(actionElem->action + " := 0;\n");

                                    // If not present in "continuousActions" body, add, according to "mainType"
                                if(actionElem->mainType == e_ContinuousAction)
                                {
                                    (*tempJsonCA)["body"].push_back("IF " + 
                                    element_name_trim(elem->name, elem->mainType) + 
                                    " THEN\n    " + 
                                    actionElem->action + 
                                    " := 1;\nEND_IF;\n");

                                    #ifdef DEBUG
                                    std::cout << "build_actions_template(): POU '" << templateData["name"] << "', applied rule 9." << std::endl;
                                    std::cout << "                          Step named '" << elem->name << "'." << std::endl;
                                    std::cout << "                          Action named '" << actionElem->name << "'." << std::endl;
                                    #endif
                                }
                                else
                                {
                                    (*tempJsonCA)["body"].push_back("IF " + 
                                    element_name_trim(elem->name, elem->mainType) + 
                                    " THEN\n    " +
                                    actionElem->action +
                                    " := 1 AND (" +
                                    parse_conditional_statement(actionElem->condition, templateData) +
                                    ");\nEND_IF;\n");

                                    #ifdef DEBUG
                                    std::cout << "build_actions_template(): POU '" << templateData["name"] << "', applied rule 10." << std::endl;
                                    std::cout << "                          Step named '" << elem->name << "'." << std::endl;
                                    std::cout << "                          Action named '" << actionElem->name << "'." << std::endl;
                                    #endif
                                }

                                break;
                            }

                            case e_StoredAction: // Rules 11.a and 11.b
                            case e_EventAction: // Rule 11.c
                            {
                                nlohmann::json* tempJsonSA = &find_method_by_name(templateData["method_list"], "storedActionHandler");

                                // If not present in "storedActionHandler" body, add, according to "mainType"
                                if(actionElem->mainType == e_StoredAction)
                                {
                                    (*tempJsonSA)["body"].push_back("IF " +
                                    add_trigger(element_name_trim(elem->name, elem->mainType), 
                                    ((actionElem->actionType == ActivationAction) ? "RTRIG" : "FTRIG"),
                                    true, templateData) +
                                    " THEN\n    " +
                                    parse_stored_action(actionElem->action, templateData) +
                                    ";\nEND_IF;\n");

                                    #ifdef DEBUG
                                    std::cout << "build_actions_template(): POU '" << templateData["name"] << "', applied rule 11.a/11.b." << std::endl;
                                    std::cout << "                          Step named '" << elem->name << "'." << std::endl;
                                    std::cout << "                          Action named '" << actionElem->name << "'." << std::endl;
                                    #endif
                                }
                                else
                                {
                                    (*tempJsonSA)["body"].push_back("IF " +
                                    element_name_trim(elem->name, elem->mainType) +
                                    " AND (" +
                                    add_trigger(parse_conditional_statement(actionElem->condition, templateData),
                                    "RTRIG", false, templateData) +
                                    ") THEN\n    " +
                                    parse_stored_action(actionElem->action, templateData["method_list"]) +
                                    ";\nEND_IF;\n");

                                    #ifdef DEBUG
                                    std::cout << "build_actions_template(): POU '" << templateData["name"] << "', applied rule 11.c." << std::endl;
                                    std::cout << "                          Step named '" << elem->name << "'." << std::endl;
                                    std::cout << "                          Action named '" << actionElem->name << "'." << std::endl;
                                    #endif
                                }

                                break;
                            }

                            case e_ForcingOrder: // Rules 12, 13, 14 and 15
                            {
                                nlohmann::json* tempJsonCA = &find_method_by_name(templateData["method_list"], "continuousActions");
                                std::string forcedFlagMVLStr = templateData["misc_variable_list"].dump();
                                std::string forcingOrderStr = parse_forcing_action(actionElem->action, actionElem->actionType, templateData);

                                if(forcedFlagMVLStr.find("forcedFlag") == std::string::npos)
                                    add_to_variable_list(templateData["misc_variable_list"], build_variable_template("forcedFlag", "BOOL", "FALSE"));

                                (*tempJsonCA)["body"].push_back("IF " +
                                element_name_trim(elem->name, elem->mainType) +
                                " THEN\n    " + forcingOrderStr +
                                "\nEND_IF;\n");

                                #ifdef DEBUG
                                std::cout << "build_actions_template(): POU '" << templateData["name"] << "', applied rule 12/13/14/15." << std::endl;
                                std::cout << "                          Step named '" << elem->name << "'." << std::endl;
                                std::cout << "                          Action named '" << actionElem->name << "'." << std::endl;
                                #endif

                                break;
                            }

                            default: break;
                        }
                    }

                    break;
                }

                default: break;
            }
        }
    }
    
    #ifdef DEBUG
    std::cout << "build_actions_template(): Action template for POU named '" << templateData["name"].dump() << "' built." << std::endl;
    #endif
    
    return;
}

// Generates a random UUID
std::string generate_UUID() 
{
    // Static random engine seeded once with high-resolution clock
    static std::mt19937 rng(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    ));
    static std::uniform_int_distribution<int> dist(0, 255); // byte range

    // Generate 16 random bytes (128 bits)
    std::array<unsigned char, 16> bytes;
    for (auto& b : bytes) {
        b = static_cast<unsigned char>(dist(rng));
    }

    // Set version (4) in the 7th byte (index 6) -> upper nibble = 0100
    bytes[6] = (bytes[6] & 0x0F) | 0x40;   // 0x40 = binary 01000000
    // Set variant (RFC 4122) in the 9th byte (index 8) -> upper nibble = 10xx
    bytes[8] = (bytes[8] & 0x3F) | 0x80;   // 0x80 = binary 10000000

    // Convert to hex string with hyphens
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            ss << '-';
        }
    }
    return ss.str();
}

// Parses conditional statements "statement" to fit CODESYS format
std::string parse_conditional_statement(std::string statement, nlohmann::json& fb)
{
    if(statement.empty())
    {
        std::cout << "ERROR: parse_conditional_statement(): Statement string empty. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string returnStr = "";
    std::string spacedString = add_spaces_to_statements(statement);
    bool isInsideSqrBrckt = false;
    std::string mChar; // Using as single char. std::string because of UTF-8 having 3 chars per symbol
    std::map<std::string, std::string> charMap = {
        {"+", "OR"},
        {"*", "AND"},
        {"!", "NOT"},
        {"[", "("},
        {"]", ")"},
        {">", "&gt;"},
        {"<", "&lt;"}
    };

    for(int i=0 ; i<spacedString.size() ; i++)
    {
        mChar = spacedString[i];

        try 
        {
            if(mChar == "[") isInsideSqrBrckt = true;
            if(mChar == "]") isInsideSqrBrckt = false;
            if((mChar == "*" || mChar == "+") && isInsideSqrBrckt)
            {
                returnStr.append(mChar);
                continue;
            }
            // Had to add this crap afterwards, forgot that steps can be referenced with 'X'
            if(mChar == "X")
                if((spacedString[i+1] == '0' || spacedString[i+1] == '1' || spacedString[i+1] == '2' || spacedString[i+1] == '3' || spacedString[i+1] == '4'
                || spacedString[i+1] == '5' || spacedString[i+1] == '6' || spacedString[i+1] == '7' || spacedString[i+1] == '8' || spacedString[i+1] == '9'))
                {
                    returnStr.append("S");
                    continue;
                }
                else if(spacedString[i+1] == 'E' || spacedString[i+1] == 'S' && (spacedString[i+2] == '0' || spacedString[i+2] == '1' || spacedString[i+2] == '2' || spacedString[i+2] == '3' || spacedString[i+2] == '4'
                || spacedString[i+2] == '5' || spacedString[i+2] == '6' || spacedString[i+2] == '7' || spacedString[i+2] == '8' || spacedString[i+2] == '9'))
                {
                    returnStr.append("S");
                    continue;
                }
            returnStr.append(charMap.at(mChar));
        }
        catch(const std::out_of_range& e)
        {
            returnStr.append(mChar);
        }
    }
    
    parse_trigger_statement(returnStr, fb); // Handles rising/falling edges
    parse_time_statement(returnStr, fb); // Handles timed variables

    return returnStr;
}

// Parses rising/falling trigger statements "statement" to fit CODESYS format
// Declares and defines trigger variables on function block "fb"
void parse_trigger_statement(std::string& statement, nlohmann::json& fb)
{
    if(statement.empty())
    {
        std::cout << "ERROR: parse_trigger_statement(): Statement string empty. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> trigTypeVec = {"RTRIG", "FTRIG"};
    size_t trigPos = 0;
    size_t skipPos = 0;
    std::string trigStr = "";
    std::string trigName = "";
    char endCheck;
    int kInitVal;
    std::string arrowType;
    bool hasNOTstatement = false;

    for(std::string trigType : trigTypeVec)
    {
        if(trigType == "RTRIG") arrowType = "↑";
        else arrowType = "↓";

        while(true)
        {
            trigPos = statement.find(arrowType, trigPos);

            if(trigPos == std::string::npos)
                break;

            if(statement[trigPos+1] == '(')
            {
                kInitVal = trigPos+4; // 4 = 2 spaces + 2 char spaces because of UTF-8
                endCheck = ')';
            }
            else
            {
                kInitVal = trigPos+3; // 4 = 2 spaces + 2 char spaces because of UTF-8
                endCheck = ' ';                
                if(statement.find("NOT") == trigPos + 3) hasNOTstatement = true; // 3 because of "NOT"
            }

            for(int k=kInitVal ; k<statement.size() ; k++)
            {
                skipPos = k;
                char c = statement[k];
                if(c == endCheck)
                {
                    if(hasNOTstatement)
                    {
                        hasNOTstatement = false;
                        trigStr.push_back(c);
                        continue;
                    }

                    break;
                }
                trigStr.push_back(c);
            }

            // If a variable starts with 'S' and has a digit right after,
            // then this structure will mistake it for a step
            if(trigStr[0] == 'S'
            && (trigStr[1] == '0' || trigStr[1] == '1' || trigStr[1] == '2' || trigStr[1] == '3' || trigStr[1] == '4'
            || trigStr[1] == '5' || trigStr[1] == '6' || trigStr[1] == '7' || trigStr[1] == '8' || trigStr[1] == '9'))
                trigName = add_trigger(trigStr, trigType, true, fb);
            else
                trigName = add_trigger(trigStr, trigType, false, fb);

            // Substitute new declaration in return string
            int eraseLenght = skipPos-trigPos;
            if(eraseLenght == statement.size()-1) eraseLenght++; // Stupid fkn fix that somehow works, brain tired, no want thinky
            statement.erase(trigPos, eraseLenght);
            statement.insert(trigPos, trigName);

            // Reset/update vars
            trigPos = skipPos;
            skipPos = 0;
            trigStr = "";
            trigName = "";
            if(trigPos != statement.size()-1) trigPos++;
            hasNOTstatement = false;
        }

        trigPos = 0; // Reset for FTRIG
    }
    
    return;
}

// If "varName" is a step, adds to function block "fb" declaration and to "stepTriggerHandler" method,
// else add it only to "triggerHandler" method
// Assumes "varName" is already trimmed, shortened and processed (eg1.: Step1 -> S1 ; eg2.: A + B -> A OR B)
// trigType == RTRIG || trigType == FTRIG
// "isStep" ensures whether "trigName" represents a step variable (== true) or another condition (== false)
std::string add_trigger(std::string varName, std::string trigType, bool isStep, nlohmann::json& fb)
{
    if(varName.empty())
    {
        std::cout << "ERROR: add_trigger(): trigName empty. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }

    if(trigType != "RTRIG" && trigType != "FTRIG")
    {
        std::cout << "ERROR: add_trigger(): trigType incorrect. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::string methodName = "";
    std::string trigName = "";
    std::string tempStr = "";
    std::string returnStr = "";

    if(isStep)
        methodName = "stepTriggerHandler";
    else
        methodName = "triggerHandler";

    try
    {
        nlohmann::json& methodPtr = find_method_by_name(fb["method_list"], methodName);
        returnStr = find_trigger_variable_by_name(methodPtr["body"], varName, trigType);
        if(!returnStr.empty()) return returnStr.append(".Q");
    }
    catch(const std::runtime_error& e)
        {initialize_method(fb["method_list"], methodName, "", "", "");}

    if(trigType == "RTRIG")
    {
        trigName = "RTRIG_var" + std::to_string(rTriggerNumber);
        rTriggerNumber++;
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(trigName, "R_TRIG"));
    }
    else
    {
        trigName = "FTRIG_var" + std::to_string(fTriggerNumber);
        fTriggerNumber++;
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(trigName, "F_TRIG"));
    }

    tempStr = trigName + "(CLK:=" + varName + ");\n";
    find_method_by_name(fb["method_list"], methodName)["body"].push_back(tempStr);
    
    return trigName.append(".Q");
}

// Parses time conditioned statements "statement" to fit CODESYS format
// Declares and defines time related variables on function block "fb"
void parse_time_statement(std::string& statement, nlohmann::json& fb)
{
    std::string tonTime = "";
    std::string tofTime = "";
    std::string condition = "";
    size_t index = statement.find("/", 0);
    std::vector<size_t> indexList;

    if(statement.empty())
    {
        std::cout << "ERROR: parse_time_statement(): Statement string empty. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Since having more than one timed condition in Grafcet-Studio is essencialy impossible, this will only
    // have one condition in mind
    while(index != std::string::npos)
    {
        indexList.push_back(index);

        if(statement[index-1] == 's')
        {
            // check left for number until " " or index = 0 and save
            for(int k=index-2 ; k>=0 ; k--)
            {
                std::string tempStatementChar = "";
                tempStatementChar.push_back(statement[k]);
                if(tempStatementChar == " ") break;
                tonTime.insert(0, tempStatementChar);
                tempStatementChar.clear();
            }
        }
        else // check right for other "/" until " "
        {
            // check for numbers until "s"
            for(int j=index+1 ; j<statement.size() ; j++)
            {
                std::string tempStatementChar = "";
                tempStatementChar.push_back(statement[j]);
                
                if(tempStatementChar != "0" && tempStatementChar != "1" && tempStatementChar != "2" && tempStatementChar != "3" && tempStatementChar != "4"
                && tempStatementChar != "5" && tempStatementChar != "6" && tempStatementChar != "7" && tempStatementChar != "8" && tempStatementChar != "9")
                    break;

                if(tempStatementChar == "s") break;
                
                tofTime.append(tempStatementChar);
                tempStatementChar.clear();
            }
        }
        
        if(index+1 > statement.size()) break;
        index = statement.find("/", index+1);
    }

    if(indexList.size() == 2)
    {
        for(size_t p=indexList[0]+1 ; p<=indexList[1]-indexList[0]+1 ; p++)
        {
            std::string tempStrP = "";
            tempStrP.push_back(statement[p]);
            condition.append(tempStrP);
            tempStrP.clear();
        }
        if(condition[0] == '(') condition.erase(0, 1);
        if(condition[condition.size()-1] == ')') condition.erase(condition.size()-1, 1);
    }
    else if(!tonTime.empty())
    {
        std::string endStr;
        
        if(statement[indexList[0]+1] == '(')
            endStr = ")";
        else endStr = " ";

        for(int p=indexList[0]+1 ; p<statement.size() ; p++)
        {
            std::string tempStrP = "";
            tempStrP.push_back(statement[p]);
            if(tempStrP == endStr) break;
            condition.append(tempStrP);
            tempStrP.clear();
        }
    }
    else if(!tofTime.empty())
    {
        std::string endStr;
        
        if(statement[indexList[0]-1] == ')')
            endStr = "(";
        else endStr = " ";

        for(int p=indexList[0]-1 ; p>=0 ; p--)
        {
            std::string tempStrP = "";
            tempStrP.push_back(statement[p]);
            if(tempStrP == endStr) break;
            condition.insert(0, tempStrP);
            tempStrP.clear();
        }
    }
    else return; // Found nothing

    add_time_condition(statement, fb, tonTime, tofTime, condition, indexList);

    return;
}

// Modifies and returns string "statement" given the TON time "tonTime", TOF time "tofTime", condition "condition"
// and list of string::find("/") indexes "indexList"
// Adds TON, TOF and TCOND variables to "fb" "misc_variable_list"
// Adds TON, TOF and TCOND variable declarations to "fb"'s "timedConditionHandler" method's "body"
std::string add_time_condition(std::string& statement, nlohmann::json& fb, std::string& tonTime,
                               std::string& tofTime, std::string& condition, std::vector<size_t>& indexList)
{
    nlohmann::json* tempTimeMethod = nullptr;

    try
        {tempTimeMethod = &find_method_by_name(fb["method_list"], "timedConditionHandler");}
    catch(const std::exception& e)
    {
        initialize_method(fb["method_list"], "timedConditionHandler", "", "", "");
        tempTimeMethod = &find_method_by_name(fb["method_list"], "timedConditionHandler");
    }

    if(!tonTime.empty() && !tofTime.empty())
    {
        std::string tOnName = "TON_var" + std::to_string(tOnNumber++);
        std::string tOfName = "TOF_var" + std::to_string(tOfNumber++);
        std::string tCondName = "TCOND_var" + std::to_string(tCondNumber++);
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tOnName, "TON"));
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tOfName, "TOF"));
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tCondName, "BOOL"));
        (*tempTimeMethod)["body"].push_back(tOnName + "(IN:=" + condition + " , PT:=T#" + tonTime + "S);\n");
        (*tempTimeMethod)["body"].push_back(tOfName + "(IN:=" + tOnName + ".Q , PT:=T#" + tofTime + "S);\n");
        (*tempTimeMethod)["body"].push_back(tCondName + " := " + tOfName + ".Q;\n");
        int startPos = indexList[0] - tonTime.length() - 1; // +1 because of 's'
        int endPos = indexList[1] + tofTime.length() + 2; // +2 because of "/s"
        int fullConditionLenght = endPos - startPos;
        statement.erase(startPos, fullConditionLenght);
        statement.insert(startPos, tCondName);
    }
    else if(!tonTime.empty())
    {
        std::string tOnName = "TON_var" + std::to_string(tOnNumber++);
        std::string tCondName = "TCOND_var" + std::to_string(tCondNumber++);
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tOnName, "TON"));
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tCondName, "BOOL"));
        (*tempTimeMethod)["body"].push_back(tOnName + "(IN:=" + condition + " , PT:=T#" + tonTime + "S);\n");
        (*tempTimeMethod)["body"].push_back(tCondName + " := " + tOnName + ".Q;\n");
        int startPos = indexList[0] - tonTime.length() - 1; // +1 because of 's'
        int endPos = indexList[0] + 1 + condition.length(); // +1 because of '/'
        int fullConditionLenght = endPos - startPos;
        statement.erase(startPos, fullConditionLenght);
        statement.insert(startPos, tCondName);
    }
    else if(!tofTime.empty())
    {
        std::string tOfName = "TOF_var" + std::to_string(tOfNumber++);
        std::string tCondName = "TCOND_var" + std::to_string(tCondNumber++);
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tOfName, "TOF"));
        add_to_variable_list(fb["misc_variable_list"], build_variable_template(tCondName, "BOOL"));
        (*tempTimeMethod)["body"].push_back(tOfName + "(IN:=" + condition + " , PT:=T#" + tofTime + "S);\n");
        (*tempTimeMethod)["body"].push_back(tCondName + " := " + tOfName + ".Q;\n");
        int endPos = indexList[0] + tofTime.length() + 2; // +2 because of "/s"
        int startPos = indexList[0] - condition.length();
        int fullConditionLenght = endPos - startPos;
        statement.erase(startPos, fullConditionLenght);
        statement.insert(startPos, tCondName);
    }

    return statement;
}

// build_variable_template()
#pragma region
// Returns string containing the full variable template, populated with a name(, type and initial value)
// type string for type of variable, if no string, assume function block derived type value
// initValue string for initial value
std::string build_variable_template(const std::string &name)
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("variable");
    std::stringstream ss;
    std::string tempStr = name;
    tempStr.erase(0, 3); // Erase "fb_"

    root.append_attribute("name") = name.c_str();

    pugi::xml_node typeNode = root.append_child("type");
    pugi::xml_node deriveNode = typeNode.append_child("derived");
    deriveNode.append_attribute("name") = tempStr.c_str();
    
    doc.save(ss, "  ", pugi::format_indent | pugi::format_no_declaration);

    std::string result = ss.str();
    if(result.back() == '\n') result.pop_back(); // Remove newline char if present

    return result;
}

std::string build_variable_template(const std::string &name, const std::string &type)
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("variable");
    std::stringstream ss;

    root.append_attribute("name") = name.c_str();

    pugi::xml_node typeNode = root.append_child("type");

    if(type == "R_TRIG" || type == "F_TRIG" || type == "TON" || type == "TOF")
    {
        pugi::xml_node deriveNode = typeNode.append_child("derived");
        deriveNode.append_attribute("name") = type.c_str();
    }
    else typeNode.append_child(type.c_str());
    
    doc.save(ss, "  ", pugi::format_indent | pugi::format_no_declaration);

    std::string result = ss.str();
    if(result.back() == '\n') result.pop_back(); // Remove newline char if present

    return result;
}

std::string build_variable_template(const std::string &name, const std::string &type, const std::string &initValue)
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("variable");
    std::stringstream ss;

    root.append_attribute("name") = name.c_str();

    pugi::xml_node typeNode = root.append_child("type");
    
    if(type == "R_TRIG" || type == "F_TRIG" || type == "TON" || type == "TOF")
    {
        pugi::xml_node deriveNode = typeNode.append_child("derived");
        deriveNode.append_attribute("name") = type.c_str();
    }
    else typeNode.append_child(type.c_str());
    
    pugi::xml_node initNode = root.append_child("initialValue");
    pugi::xml_node simpleValue = initNode.append_child("simpleValue"); // Not sure if works with every type
    simpleValue.append_attribute("value") = initValue.c_str();

    doc.save(ss, "  ", pugi::format_indent | pugi::format_no_declaration);

    std::string result = ss.str();
    if(result.back() == '\n') result.pop_back(); // Remove newline char if present

    return result;
}
#pragma endregion

// Initializes the pou template with name "pgName" and type "pgType" for the data strcuture "templateData"
void build_pou_template(nlohmann::json& templateData, std::string pgName, PartialGrafcetType pgType)
{
    if(pgName.empty()) return;
    templateData["pou_list"].push_back({{"name", pgName}, {"UUID", generate_UUID()}});
    nlohmann::json& pouJson = templateData["pou_list"].back();
    templateData["gvl"]["variable_list"]["function_blocks"].push_back(build_variable_template("fb_" + pgName));
    pouJson["misc_variable_list"] = nlohmann::json::array();

    switch(pgType)
    {
        case partial_normal:
        {
            initialize_common_methods(pouJson["method_list"], pgType);
            break;
        }
    
        case partial_macro:
        {
            std::vector<std::string> accessSteps; // indexes: 0 -> Entry ; 1 -> Exit

            add_to_variable_list(pouJson["misc_variable_list"], build_variable_template("finishedFlag", "BOOL"));
            add_to_variable_list(pouJson["misc_variable_list"], build_variable_template("active", "BOOL"));

            initialize_common_methods(pouJson["method_list"], pgType);

            accessSteps = find_macro_grafcet_entry_and_exit(pgName);

            find_method_by_name(pouJson["method_list"], "initializeAndStop")["body"].push_back(
                "IF active THEN\n    pullDownSignals();\n    active := FALSE;\n    finishedFlag := FALSE;\nEND_IF;\n"
            );

            break;
        }
        
        case partial_enclosing:
        {
            std::vector<std::string> activationLinkSteps;

            add_to_variable_list(pouJson["misc_variable_list"], build_variable_template("firstActivation", "BOOL", "TRUE"));
            add_to_variable_list(pouJson["misc_variable_list"], build_variable_template("active", "BOOL"));

            initialize_common_methods(pouJson["method_list"], pgType);

            activationLinkSteps = find_enclosing_grafcet_activation_links(pgName);

            find_method_by_name(pouJson["method_list"], "initializeAndStop")["body"].push_back(
                "IF active THEN\n    pullDownSignals();\n    active := FALSE;\nEND_IF;\n"
            );

            break;
        }
            
        default: break;
    }   

    return;
}

// Initializes the common methods for a given POU method list "methodList" of type "pgType"
void initialize_common_methods(nlohmann::json &methodList, PartialGrafcetType pgType)
{   
    std::vector<std::vector<std::string>> vec = {
            {"evaluateTransitions", "", "", ""},
            {"evaluateFireableTransitions", "", "", ""},            
            {"pullDownSignals", "", "", ""},
            {"continuousActions", "", "", ""},
            {"storedActionHandler", "", "", ""},
            };

    switch(pgType)
    {
        case partial_macro:
        {
            vec.push_back({"stateEvolutionHandler", build_variable_template("stableState", "BOOL"), "IF NOT forcedFlag AND active THEN\n", "BOOL"}); 
            vec.push_back({"initializeAndStop", "", "", ""});
            vec.push_back({"activate", "", "active := TRUE;", ""});
            vec.push_back({"getFinishedFlag", "", "getFinishedFlag := finishedFlag;", "BOOL"});
            break;
        }

        case partial_enclosing:
        {
            vec.push_back({"stateEvolutionHandler", build_variable_template("stableState", "BOOL"), "IF NOT forcedFlag AND active THEN\n", "BOOL"});
            vec.push_back({"initializeAndStop", "", "", ""});
            vec.push_back({"activate", "", "active := TRUE;", ""});
            break;
        }
    
        case partial_normal:
        {
           vec.push_back({"stateEvolutionHandler", build_variable_template("stableState", "BOOL"), "IF NOT forcedFlag THEN\n", "BOOL"}); 
           break;
        }
        default: break;
    }
    
    // Had to do it this way (check initialize_method()) because for some fucking reason it decides to shit 
    // itself with a simple command like this ({"body", arr[3].empty() ? nullptr : arr[3]}) :)))))
    for(const auto& arr : vec)
        initialize_method(methodList, arr[0], arr[1], arr[2], arr[3]);

    return;
}

// Initializes a specific method on "methodList", with name "methodName", variable list "methodVarList",
// body "methodBody" and type "methodReturnType"
void initialize_method(nlohmann::json &methodList, std::string methodName, std::string methodVarList, 
                       std::string methodBody, std::string methodReturnType)
{
    methodList.push_back(nlohmann::json::object());
    methodList.back().push_back({"variable_list", nlohmann::json::array()});
    methodList.back().push_back({"body", nlohmann::json::array()});
    methodList.back().push_back({"return_type", nlohmann::json::object()});
    
    methodList.back().push_back({"name", methodName});
    methodList.back().push_back({"UUID", generate_UUID()});
    if(!methodVarList.empty()) methodList.back()["variable_list"].push_back(methodVarList);
    if(!methodBody.empty()) methodList.back()["body"].push_back(methodBody);
    if(!methodReturnType.empty()) methodList.back()["return_type"] = methodReturnType;

    return;
}

// Finds and returns the pou from "pouList" with name "pouName"
nlohmann::json& find_pou_by_name(nlohmann::json &pouList, const std::string& pouName)
{
    for(auto& pou : pouList)
        if(pou["name"] == pouName)
            return pou;
    
    throw std::runtime_error("ERROR: find_pou_by_name(): POU " + pouName + " not found.");
}

// Finds and returns the method from "methodList" with name "methodName"
nlohmann::json& find_method_by_name(nlohmann::json &methodList, const std::string& methodName)
{    
    for(auto& method : methodList)
        if(method["name"] == methodName)
            return method;
    throw std::runtime_error("ERROR: find_method_by_name(): Method " + methodName + " not found.");
}

// Finds trigger variable associated to the variable "varName" of type "trigType" 
// in the body of a given method "methodBody".
// If it exists returns the already existing trigger variable, otherwise return null string
std::string find_trigger_variable_by_name(nlohmann::json &methodBody, const std::string& varName, const std::string& trigType)
{
    std::string returnStr = "";
    std::string tempStr = "";
    std::string tempStr2 = "";
    size_t varIndex = 0;

    if(varName.empty())
    {
        std::cout << "ERROR: find_trigger_variable_by_name(): varName empty. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    if(trigType != "RTRIG" && trigType != "FTRIG")
    {
        std::cout << "ERROR: find_trigger_variable_by_name(): trigType incorrect. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }

    for(auto& var : methodBody)
    {
        tempStr = var;
        if((tempStr.find("RTRIG") == std::string::npos && trigType == "RTRIG")
        || (tempStr.find("FTRIG") == std::string::npos && trigType == "FTRIG"))
            continue;
        varIndex = tempStr.find(':');

        if(varIndex != std::string::npos)
        {
            for(int i=varIndex+2 ; i<tempStr.size()-2 ; i++) // +2 because of ":=" and -2 because of ");"
                tempStr2 = tempStr2 + tempStr[i];

            if(varName == tempStr2)
            {
                for(int k=0 ; k<=varIndex-5 ; k++) // -5 because of "(CLK:"
                    returnStr = returnStr + tempStr[k];

                return returnStr;
            }
        }

        tempStr2.clear();
    }

    return "";
}

// Finds and returns the entry and exit step elements from the partial grafcet "pgName"
std::vector<std::string> find_macro_grafcet_entry_and_exit(std::string pgName)
{
    std::string entryStep = "", exitStep = "";

    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        if(pg->name != pgName) continue;

        for(const auto& cg : pg->connectedGrafcets)
        {
            for(const auto& elem : cg->elements)
            {
                switch(elem->mainType)
                {
                    case e_NormalStep:
                    case e_InitialStep:
                    case e_MacroStep:
                    case e_EnclosedStep:
                    case e_EnclosedInitialStep:
                    {
                        std::string tempStep = element_name_trim(elem->name, elem->mainType);

                        if(tempStep[1] == 'E')
                            entryStep = tempStep;
                        
                        if(tempStep[1] == 'S')
                            exitStep = tempStep;

                        break;
                    }
                
                    default: break;
                }

            }
        }
    }

    return {entryStep, exitStep};
}

// Finds and returns all the step elements with a true activation link from the partial grafcet "pgName"
std::vector<std::string> find_enclosing_grafcet_activation_links(std::string pgName)
{
    std::vector<std::string> activationLinkSteps;

    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        if(pg->name != pgName) continue;

        for(const auto& cg : pg->connectedGrafcets)
        {
            for(const auto& elem : cg->elements)
            {
                switch(elem->mainType)
                {
                    case e_NormalStep:
                    case e_InitialStep:
                    case e_MacroStep:
                    case e_EnclosedStep:
                    case e_EnclosedInitialStep:
                    {
                        if(elem->activationLink)
                            activationLinkSteps.push_back(element_name_trim(elem->name, elem->mainType));

                        break;
                    }
                
                    default: break;
                }

            }
        }
    }

    return activationLinkSteps;
}

// Finds the type of the associated partial grafcet (if any) and returns it based on the id "identifier" of the given node
PartialGrafcetType find_pg_type_by_node_id(int identifier)
{
    for(const auto& pg : mainGrafcet.partialGrafcets)
    {
        for(const auto& cg : pg->connectedGrafcets)
        {
            for(const auto& elem : cg->elements)
            {
                if(elem->id == identifier)
                    return pg->type;
            }
        }
    }

    return partial_normal;
}

// Checks if a certain variable of name "varName" is in a given variable list "varList"
// return true -> is in list
// return false -> is NOT in list
bool check_if_in_variable_list(nlohmann::json &varList, std::string varName)
{
    std::string tempStr = "";
    std::string attrStr = "name\"" + varName + "\""; // Attribute

    for(const auto& var : varList)
    {
        tempStr = var; // Cannot use "var" directly, otherwise it uses the nlohmann::find instead of cpps
        if(tempStr.find(attrStr) != std::string::npos) // Checks for 'name=\"varName"\'
            return true;
    }
        
    return false;
}

// Adds a already built variable template "varBuiltTemplate" to a variable list "varList"
// Assumes variable does not already exist in list
void add_to_variable_list(nlohmann::json &varList, std::string varBuiltTemplate)
{
    varList.push_back(varBuiltTemplate);
    return;
}

// Trims and capitalizes element name "elementName" in order to be used in templates according to its "elementType"
std::string element_name_trim(std::string elementName, ElementType elementType)
{
    std::string elementNumber = "";    

    for(int i=elementName.find('_')+1 ; i<elementName.size() ; i++)
        elementNumber.push_back(elementName[i]);

    if(elementNumber.empty())
    {
        std::cout << "ERROR: element_name_trim(): Element name provided has no number." << std::endl;
        exit(EXIT_FAILURE);
    }

    switch(elementType)
    {
        case e_NormalStep:
        case e_InitialStep:
        case e_EnclosedStep:
        case e_EnclosedInitialStep:
            return ("S" + elementNumber);

        case e_MacroStep:
            return (elementNumber);
        
        case e_Transition:
            return ("T" + elementNumber);

        default: 
        {
            std::cout << "ERROR: element_name_trim(): Element with name " + elementName + " of type " + grafcetTypesR.at(elementType) + " has incorrect type." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

// Adds spaces (' ') before/after certain characters of a condition/action statement
// "statement" to be better integrated into CODESYS (also gives better readability)
std::string add_spaces_to_statements(std::string statement)
{
    std::string returnStr = "";
    char lChar, mChar, rChar;
    bool timeStatementFlag;

    // Appends first "statement" char
    returnStr.push_back(statement[0]);
    if(statement[0] == '!' && statement[1] != '(') returnStr.push_back(' ');

    // Only checks chars between first and last, three char sized window into "statement" analysis
    for(int i=1 ; i<statement.size()-1 ; i++)
    {
        lChar = statement[i-1];
        mChar = statement[i];
        rChar = statement[i+1];
        timeStatementFlag = false;

        if(mChar == '+' 
        || mChar == '-'
        || mChar == '*'
        || mChar == '/')
        {
            if(mChar == '/')
            {
                if(lChar == 's') 
                {
                    returnStr.push_back(mChar);
                    continue;
                }

                for(int k=i+1 ; k<statement.size() ; k++)
                {
                    char mChar2 = statement[k];

                    if(mChar2 == 's')
                    {
                        returnStr.push_back(mChar);
                        timeStatementFlag = true;
                        break;
                    }

                    if(mChar2 != '0' && mChar2 != '1' && mChar2 != '2' && mChar2 != '3' && mChar2 != '4'
                    && mChar2 != '5' && mChar2 != '6' && mChar2 != '7' && mChar2 != '8' && mChar2 != '9')
                        break;

                    if(mChar2 == ' ') break;
                }

                if(timeStatementFlag == true)
                    continue;
            }

            if(lChar != ' ') returnStr.push_back(' ');
            returnStr.push_back(mChar);
            if(rChar != ' ') returnStr.push_back(' ');
            continue;
        }

        if(mChar == '='
        || mChar == '<'
        || mChar == '>')
        {
            if(lChar != '='
            && lChar != '<'
            && lChar != '>'
            && lChar != ' ')
                returnStr.push_back(' ');

            returnStr.push_back(mChar);

            if(rChar != '='
            && rChar != '<'
            && rChar != '>'
            && rChar != ' ')
                returnStr.push_back(' ');

            continue;
        }

        if(mChar == '!'
        && rChar != '(')
        {
            returnStr.push_back(mChar);
            returnStr.push_back(' ');
            continue;
        }

        returnStr.push_back(mChar);
    }

    if(statement.size() > 1) returnStr.push_back(statement[statement.size()-1]); // Appends last "statement" char

    return returnStr;
}

// Parse stored action "storedAction"
std::string parse_stored_action(std::string storedAction, nlohmann::json& fb)
{
    if(storedAction.empty())
    {
        std::cout << "ERROR: parse_stored_action(): Stored action string empty. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string returnStr = "";
    std::string secondHalfStr = "";
    int progressIndex = -1;
    char lChar, mChar;

    returnStr.push_back(storedAction[0]); // Appends first "statement" char

    // Before association, including ":="
    for(int i=1 ; i<storedAction.size()-1 ; i++)
    {
        lChar = storedAction[i-1];
        mChar = storedAction[i];

        if(mChar == ':'
        && lChar != ' ')
        {
            returnStr.push_back(' '); returnStr.push_back(mChar);
            continue;
        }
        
        // Does not need to check if there is a space after, as is the last char to consider for first half
        if(mChar == '=')
        {
            returnStr.push_back(mChar); returnStr.push_back(' ');
            progressIndex = i+1;
            break;
        }

        returnStr.push_back(mChar);
    }

    // Aux string with text remainder
    for(int k=progressIndex ; k<storedAction.size() ; k++)
        secondHalfStr.push_back(storedAction[k]);

    // After association
    //return returnStr + parse_conditional_statement(secondHalfStr, fb); // Creates errors with simple addition and multiplication (elements get swapped to logic)
    return returnStr + add_spaces_to_statements(secondHalfStr); // May create problems down the line where logic is required, given that it is a store action, this is unlikely, I hope :)
}

// Parses forcing orders with action "forceAction" of type "forceType" for the function block "fb"
std::string parse_forcing_action(std::string forceAction, ActionType forceType, nlohmann::json& fb)
{
    char c;
    int i = 0;
    std::string returnStr = "";
    std::string affectedGrafcetName = "";
    nlohmann::json* affectedGrafcetObject = nullptr;

    while(true)
    {
        c = forceAction[i++];
        if(c == '{') break;
        affectedGrafcetName.push_back(c);
    }

    try
        {affectedGrafcetObject = &find_pou_by_name(data["pou_list"], affectedGrafcetName);}
    catch(const std::exception& e)
    {
        std::cout << "ERROR: parse_forcing_action(): POU " << affectedGrafcetName << " does not exist. Terminating program..." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    nlohmann::json* tempJsonPDS = &find_method_by_name(fb["method_list"], "pullDownSignals");
    std::string forcedFlagPDSSearchStr = "fb_" + affectedGrafcetName + ".setForcedFlag(FALSE);";
    std::string forcedFlagPDSStr = "";
    bool gotForcedFlagStr = false;

    for (const auto& line : (*tempJsonPDS)["body"])
        if(line.get<std::string>() == forcedFlagPDSSearchStr)
            gotForcedFlagStr = true;
    
    if(!gotForcedFlagStr)
        (*tempJsonPDS)["body"].push_back(forcedFlagPDSSearchStr);

    // No idea if setting forced before deactivation bricks the ST program, playing it safe by using the same line
    // of code four times :)
    switch(forceType)
    {
        case explicitSituation:
        {
            nlohmann::json* tempDAS = nullptr;

            try
                {tempDAS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "deactivateAllSteps");}
            catch(const std::exception& e)
            {
                initialize_method((*affectedGrafcetObject)["method_list"], "deactivateAllSteps", "", "", "");
                tempDAS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "deactivateAllSteps");
            }

            for(const auto& line : (*affectedGrafcetObject)["step_variable_list"])
            {
                std::string tempStr = line;
                std::string svlStr = "";
                bool gotStepInDAS = false;
                size_t index = (tempStr.find('S') < tempStr.find('M')) ? tempStr.find('S') : tempStr.find('M');

                for(int k=index ; tempStr[k] != '"' ; k++)
                    svlStr.push_back(tempStr[k]);

                std::string resultStr = svlStr + " := FALSE;\n";

                for(const auto& line : (*tempDAS)["body"])
                    if(line.get<std::string>() == resultStr)
                        gotStepInDAS = true;

                if(!gotStepInDAS)
                    (*tempDAS)["body"].push_back(resultStr);
            }

            returnStr.append("fb_" + affectedGrafcetName + ".deactivateAllSteps();");
            
            returnStr.append("\n    fb_" + affectedGrafcetName + ".setForcedFlag(TRUE);");
            
            std::string stepNumber = "";

            for(int k=forceAction.find('{')+1 ; k<forceAction.size() ; k++)
            {
                if(forceAction[k] == ',' || forceAction[k] == '}')
                {
                    initialize_method((*affectedGrafcetObject)["method_list"], "activateStateS" + stepNumber, "", "S" + stepNumber + " := TRUE;", "");
                    returnStr.append("\n    fb_" + affectedGrafcetName + ".activateStateS" + stepNumber + "();");
                    stepNumber.clear();
                    continue;
                }

                stepNumber.push_back(forceAction[k]);
            }

            break;
        }

        case currentSituation:
        {
            returnStr.append("fb_" + affectedGrafcetName + ".setForcedFlag(TRUE);");
            break;
        }

        case emptySituation:
        {
            nlohmann::json* tempDAS = nullptr;

            try
                {tempDAS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "deactivateAllSteps");}
            catch(const std::exception& e)
            {
                initialize_method((*affectedGrafcetObject)["method_list"], "deactivateAllSteps", "", "", "");
                tempDAS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "deactivateAllSteps");
            }
            
            for(const auto& line : (*affectedGrafcetObject)["step_variable_list"])
            {
                std::string tempStr = line;
                std::string svlStr = "";
                bool gotStepInDAS = false;
                size_t index = (tempStr.find('S') < tempStr.find('M')) ? tempStr.find('S') : tempStr.find('M');

                for(int k=index ; tempStr[k] != '"' ; k++)
                    svlStr.push_back(tempStr[k]);

                std::string resultStr = svlStr + " := FALSE;\n";

                for(const auto& line : (*tempDAS)["body"])
                    if(line.get<std::string>() == resultStr)
                        gotStepInDAS = true;

                if(!gotStepInDAS)
                    (*tempDAS)["body"].push_back(resultStr);
            }

            returnStr.append("fb_" + affectedGrafcetName + ".deactivateAllSteps();");

            returnStr.append("\n    fb_" + affectedGrafcetName + ".setForcedFlag(TRUE);");

            break;
        }

        case initialSituation:
        {
            std::string svlStr = "";
            std::vector<std::string> svlInitVec;
            nlohmann::json* tempDAS = nullptr;
            nlohmann::json* tempAIS = nullptr;

            try
                {tempDAS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "deactivateAllSteps");}
            catch(const std::exception& e)
            {
                initialize_method((*affectedGrafcetObject)["method_list"], "deactivateAllSteps", "", "", "");
                tempDAS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "deactivateAllSteps");
            }

            try
                {tempAIS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "activateInitSituation");}
            catch(const std::exception& e)
            {
                initialize_method((*affectedGrafcetObject)["method_list"], "activateInitSituation", "", "", "");
                tempAIS = &find_method_by_name((*affectedGrafcetObject)["method_list"], "activateInitSituation");
            }
            
            for(const auto& line : (*affectedGrafcetObject)["step_variable_list"])
            {
                std::string tempStr = line;
                std::string svlStr = "";
                bool gotStepInDAS = false;
                bool gotStepInDAS2 = false;
                size_t index = (tempStr.find('S') < tempStr.find('M')) ? tempStr.find('S') : tempStr.find('M');

                for(int k=index ; tempStr[k] != '"' ; k++)
                    svlStr.push_back(tempStr[k]);

                std::string resultStr = svlStr + " := FALSE;\n";

                for(const auto& line : (*tempDAS)["body"])
                    if(line.get<std::string>() == resultStr)
                        gotStepInDAS = true;

                if(!gotStepInDAS)
                    (*tempDAS)["body"].push_back(resultStr);

                if(tempStr.find("TRUE") != std::string::npos)
                {
                    std::string resultStr2 = svlStr + " := TRUE;\n";

                    for(const auto& line : svlInitVec)
                        if(line.find(svlStr) != std::string::npos)
                            gotStepInDAS2 = true;
                    
                    if(!gotStepInDAS2)
                        svlInitVec.push_back(resultStr2);
                }
            }

            returnStr.append("fb_" + affectedGrafcetName + ".deactivateAllSteps();");

            returnStr.append("\n    fb_" + affectedGrafcetName + ".setForcedFlag(TRUE);");

            for(const auto& svlInitStr : svlInitVec)
                (*tempAIS)["body"].push_back(svlInitStr);

            returnStr.append("\n    fb_" + affectedGrafcetName + ".activateInitSituation();");

            break;
        }
    
        default:
        {
            std::cout << "ERROR: parse_forcing_action(): Forced action type incorrect (" << forceType << "). Terminating program..." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    try // If found its definetly populated, so skip
        {find_method_by_name((*affectedGrafcetObject)["method_list"], "setForcedFlag");}
    catch(const std::exception& e)
    {
        initialize_method((*affectedGrafcetObject)["method_list"], "setForcedFlag",
                          build_variable_template("flag", "BOOL"), "forcedFlag := flag;", "");
    }

    try // If found its definetly populated, so skip
        {find_method_by_name((*affectedGrafcetObject)["method_list"], "getForcedFlag");}
    catch(const std::exception& e)
    {
        initialize_method((*affectedGrafcetObject)["method_list"], "getForcedFlag",
                          "", "getForcedFlag := forcedFlag;", "BOOL");
    }

    return returnStr;
}

// Adds symbols read from the dedicated XML file and adds them to the GVL "gvl"
void add_symbols_to_gvl(nlohmann::json &gvl)
{
    std::string xmlSymbolFileSourceName = "SymbolSave.xml";
    std::string xmlSymbolFileSource = std::string("../../shared") + "/" + xmlSymbolFileSourceName;

    pugi::xml_document docIN;
    pugi::xml_parse_result result = docIN.load_file(xmlSymbolFileSource.c_str());

    if(!result)
    {
        std::cout << "ERROR - add_symbols_to_gvl(): [" << result.status << "] " << result.description() << "." << std::endl;
        return;
    }

    std::cout << "add_symbols_to_gvl(): [" << result.status << "] " 
    << result.description() << " - " << xmlSymbolFileSourceName << " opened." << std::endl;

    pugi::xml_node symbolListNode = docIN.child("SymbolList");

    std::cout << "add_symbols_to_gvl(): Reading " << xmlSymbolFileSourceName << "..." << std::endl;

    for(pugi::xml_node symbolNode : symbolListNode.children("Symbol"))
    {
        std::string symName = symbolNode.child("Name").text().as_string();
        std::string symDataType = symbolDataTypeTranslation.at(symbolNode.child("DataType").text().as_string());

        if(!check_if_in_variable_list(gvl["variable_list"]["others"], symName))
            add_to_variable_list(gvl["variable_list"]["others"], build_variable_template(symName, symDataType));
    }

    std::cout << "add_symbols_to_gvl(): All symbols read." << std::endl;

    return;
}

// Iterates throught the a list of connected grafcets "connectedGrafcetList", adding the partial grafcet
// names to the list "orderedPOUList" iteratively (calls self)
void order_POUs(std::vector<std::string>& orderedPOUList, std::vector<ConnectedGrafcet*>& connectedGrafcetList)
{
    bool isInOrderedList = false;

    for(const auto& cg : connectedGrafcetList)
    {
        for(const auto& elem : cg->elements)
        {
            if(elem->mainType == e_MacroStep
            || elem->mainType == e_EnclosedStep
            || elem->mainType == e_EnclosedInitialStep)
            {
                for(const auto& exp : elem->expansions)
                {
                    for(const auto& pouName : orderedPOUList)
                        if(pouName == exp->name) isInOrderedList = true;

                    if(!isInOrderedList) orderedPOUList.push_back(exp->name);

                    isInOrderedList = false;
                    
                    order_POUs(orderedPOUList, exp->connectedGrafcets);
                }
            }
        }
    }

    return;
}

// Categorizes POUs present in the vector "pouList" from the structure "partialGrafcetList" into the map "pouTypeMap"
void categorize_POUs(std::vector<std::string>& pouNameList, std::vector<PartialGrafcet*>& partialGrafcetList, std::map<std::string, PartialGrafcetType>& pouTypeMap)
{
    for(const auto& pouName : pouNameList)
        for(const auto& pg : partialGrafcetList)
            if(pg->name == pouName)
                pouTypeMap.insert({pouName, pg->type});

    return;
}

// Get timestamp
std::string get_timestamp()
{
    using namespace std::chrono;

    auto now = system_clock::now();

    // Split into whole seconds + fractional part
    auto secs = time_point_cast<seconds>(now);
    auto micros = duration_cast<microseconds>(now - secs);

    // Convert to calendar time (UTC; use localtime if you prefer)
    std::time_t tt = system_clock::to_time_t(secs);
    std::tm tm = *std::gmtime(&tt);

    std::ostringstream oss;

    // Format: YYYY-MM-DDTHH:MM:SS
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

    // Append .ffffff (microseconds, zero-padded)
    oss << '.' << std::setw(6) << std::setfill('0') << micros.count();

    return oss.str();
}

// Builds symbol configuration template data structure "data"
void build_symbol_configuration_template()
{
    std::string varName = "";
    std::string varType = "";

    // Data declarations
    data["symConfig"] = nlohmann::json::object();
    data["symConfig"]["entries"] = nlohmann::json::array();
    data["symConfig"]["randomUUIDs"] = nlohmann::json::array();
    data["symConfig"]["randomUUIDs"] = { {"Guid", generate_UUID()},
                                         {"ParentGuid", generate_UUID()},
                                         {"SignGuid", generate_UUID()},
                                         {"ChangePouGuid", generate_UUID()},
                                         {"PreoareOnlineChangePouGuid", generate_UUID()}
                                        };
    // Handle "entries"
    // GVL
    data["symConfig"]["entries"].push_back(nlohmann::json::object());
    data["symConfig"]["entries"].back().push_back({"name", data["gvl"]["name"]});
    data["symConfig"]["entries"].back()["variable_list"] = nlohmann::json::array();
    for(std::string gvlFBVar : data["gvl"]["variable_list"]["function_blocks"])
    {
        size_t index = gvlFBVar.find("\"");
        if(index == std::string::npos)
        {
            std::cout << "ERROR: build_symbol_configuration_template(): Found no name for GVL function block variable. Variable print:"
            << std::endl << gvlFBVar << std::endl;
            varName.clear();
            varType.clear();
            continue;
        }
        for(int i=index+1 ; i<gvlFBVar.find("\"", index+1) ; i++)
            varName.push_back(gvlFBVar[i]);

        index = gvlFBVar.find("\"", index+varName.size()+2);
        if(index == std::string::npos)
        {
            std::cout << "ERROR: build_symbol_configuration_template(): Found no type for GVL function block variable. Variable print:"
            << std::endl << gvlFBVar << std::endl;
            varName.clear();
            varType.clear();
            continue;
        }
        for(int i=index+1 ; i<gvlFBVar.find("\"", index+1) ; i++)
            varType.push_back(gvlFBVar[i]);

        data["symConfig"]["entries"].back()["variable_list"].push_back(symbol_configuration_build_variable( varName,
                                                                                                            varType,
                                                                                                            "1" ));
        varName.clear();
        varType.clear();
    }
    for(std::string gvlFBVar : data["gvl"]["variable_list"]["others"])
    {
        size_t index = gvlFBVar.find("\"");
        if(index == std::string::npos)
        {
            std::cout << "ERROR: build_symbol_configuration_template(): Found no name for GVL function block variable. Variable print:"
            << std::endl << gvlFBVar << std::endl;
            varName.clear();
            varType.clear();
            continue;
        }
        for(int i=index+1 ; i<gvlFBVar.find("\"", index+1) ; i++)
            varName.push_back(gvlFBVar[i]);

        index = gvlFBVar.find("<", index);
        if(index != std::string::npos) index = gvlFBVar.find("<", index+1);
        if(index == std::string::npos)
        {
            std::cout << "ERROR: build_symbol_configuration_template(): Found no type for GVL function block variable. Variable print:"
            << std::endl << gvlFBVar << std::endl;
            varName.clear();
            varType.clear();
            continue;
        }
        for(int i=index+1 ; i<gvlFBVar.find(" ", index+1) ; i++)
            varType.push_back(gvlFBVar[i]);
        
        data["symConfig"]["entries"].back()["variable_list"].push_back(symbol_configuration_build_variable( varName,
                                                                                                            varType,
                                                                                                            "1" ));
        varName.clear();
        varType.clear();
    }

    // PLC_PRG
    data["symConfig"]["entries"].push_back(nlohmann::json::object());
    data["symConfig"]["entries"].back().push_back({"name", data["plcprg"]["name"]});
    data["symConfig"]["entries"].back()["variable_list"] = nlohmann::json::array();
    data["symConfig"]["entries"].back()["variable_list"].push_back(symbol_configuration_build_variable( "globalStableState",
                                                                                                        "BOOL",
                                                                                                        "1" ));
    data["symConfig"]["entries"].back()["variable_list"].push_back(symbol_configuration_build_variable( "startProgram",
                                                                                                        "BOOL",
                                                                                                        "1" ));

    return;
}

// Builds variable XML formated string for symbol configuration
// Uses variable name "variableName", type "variableType" and access "variableAccess"
std::string symbol_configuration_build_variable(std::string variableName, std::string variableType, std::string variableAccess)
{
    pugi::xml_document doc;

    // Root node
    pugi::xml_node root = doc.append_child("Single");
    root.append_attribute("Type") = "{d8f96a30-175d-4274-9fd9-f7db39255531}";
    root.append_attribute("Method") = "IArchivable";

    // Child nodes
    pugi::xml_node varAccess = root.append_child("Single");
    varAccess.append_attribute("Name") = "VarAccess";
    varAccess.append_attribute("Type") = "int";
    varAccess.text().set(variableAccess.c_str());

    pugi::xml_node maxVarAccess = root.append_child("Single");
    maxVarAccess.append_attribute("Name") = "MaxVarAccess";
    maxVarAccess.append_attribute("Type") = "int";
    maxVarAccess.text().set("3");

    pugi::xml_node varType = root.append_child("Single");
    varType.append_attribute("Name") = "VarType";
    varType.append_attribute("Type") = "string";
    varType.text().set(variableType.c_str());

    pugi::xml_node varName = root.append_child("Single");
    varName.append_attribute("Name") = "VarName";
    varName.append_attribute("Type") = "string";
    varName.text().set(variableName.c_str());

    // Save to string
    std::ostringstream oss;
    doc.save(oss, "  ", pugi::format_no_declaration);
    
    return oss.str();
}

// Finds and returns name of Grafcet-Studio project file if present
std::string get_project_file_name()
{
    const std::filesystem::path folder = R"(..\..\shared)";

    if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder))
    {
        std::cout << "ERROR: get_project_file_name(): Grafcet-Studio project file not found. Using fallback name." << std::endl;
        return "PROJECT_TEMPLATE_NAME";
    }

    for (const auto& entry : std::filesystem::directory_iterator(folder))
        if (entry.is_regular_file() && entry.path().extension() == ".grafcet")
            return entry.path().stem().string();

    std::cout << "ERROR: get_project_file_name(): Grafcet-Studio project file not found. Using fallback name." << std::endl;
    return "PROJECT_TEMPLATE_NAME";
}