#include "aux_funcs.hpp"

// Finds and returns the step tempStep associated to a particular action act in a page pg
void check_action(Page &pg, Component &act)
{
    Component* tempStep = nullptr;

    for(const auto& c : pg.all)
    {        
        switch(c->type)
        {
            case e_NormalStep:
            case e_InitialStep:
            case e_MacroStep:
            case e_EnclosedStep:    
            case e_EnclosedInitialStep:
            {
                if((c->positionY + (c->height / 2)) != (act.positionY + (act.height / 2))) continue;
                if((c->positionX + c->width) > act.positionX) continue;
                if((tempStep == nullptr) || ((c->positionX + c->width) > (tempStep->positionX + tempStep->width))) tempStep = c;
                break;
            }   
            
            default: break;
        }
    }

    act.associatedStep = tempStep;
    tempStep->hasAction = true;
    return;
}

// Associates components c (associatedMacroStep or associatedEnclosedStep) belonging to a particular
// group border gb of a page pg
void check_groupBorder(std::vector<Page*> &pgs, Page &pg, Component &gb)
{
    Component* tempStep = &groupborder_associated_step(pgs, gb); // Searches steps in all pages in pgs

    for(const auto& c : pg.all)
    {
        switch(c->type)
        {
            // Ignore own type and other which will not be used in remaining toolchain
            case e_GroupBorder:
            case e_ConnectionLine:
            case e_Jump:
            {
                break;
            }   

            default:
            {
                if((c->positionX < gb.positionX) || (c->positionX > (gb.positionX + gb.width))) continue;
                if((c->positionY < gb.positionY) || (c->positionY > (gb.positionY + gb.height))) continue;
                
                c->partialGrafcetName = gb.name;

                if(tempStep == nullptr) break; // No associated step

                if(gb.stepIdentifier[0] == 'M') // If group border belongs to macro step
                {
                    c->associatedMacroStep = tempStep;
                }
                else // Must belong to enclosed step
                {
                    c->associatedEnclosedStep = tempStep;
                }

                break;
            }
        }        
    }

    return;
}

// Finds and returns the step component tempStep of any pgs associated with the group border's
// (gb) .stepIdentifier
Component& groupborder_associated_step(std::vector<Page*> &pgs, Component &gb)
{
    Component* tempStep = nullptr;

    if(gb.stepIdentifier == "") return *tempStep; // Simple partial grafcet (no step associated)

    std::string tempStr;

    for(const auto& pg : pgs)
    {
        for(const auto& c : pg->all)
        {
            switch(c->type)
            {
                case e_NormalStep:
                case e_InitialStep:
                case e_MacroStep:
                case e_EnclosedStep:
                case e_EnclosedInitialStep:
                {
                    tempStr = "";
                    
                    for(int i = 5 ; i < c->name.size() ; i++) // i=5 because it checks text after "Step_"
                        tempStr.push_back(c->name[i]);
                    
                    if(gb.stepIdentifier == tempStr) tempStep = c;

                    break;
                }
            
                default: break;
            }
        }
    }
    return *tempStep;
}

// Finds and associates previous and next components c for a step s in a page pg
void check_step(Page &pg, Component &s)
{
    Component* tempComponent = nullptr;
    Component* tempConnectionLine = nullptr;
    std::vector<int> tempIdVector; 

    for(const auto& c : pg.all)
    {
        switch(c->type)
        {
            case e_Transition:
            {
                // Check previous
                if(c->isHorizontal)
                {
                    // Left
                    if((c->positionX + c->width) == s.positionX
                    && (c->positionY + 16) == s.positionY) // 16 because GRAFCET-Studio's horizontal transitions are not centered ...
                    {
                        make_association(s, *c);
                        break;
                    }

                    // Right
                    if(!s.hasAction)
                    {
                        if((c->positionX  == (s.positionX + s.width))
                        && (c->positionY + 16) == s.positionY) // 16 because GRAFCET-Studio's horizontal transitions are not centered ...
                        {
                            make_association(s, *c);
                            break;
                        }
                    }
                }
                else
                {
                    // Top
                    if((c->positionX) == (s.positionX) // X's align (no need for middle; also GRAFCET-Studio's values for width and height are weird ...)
                    && (c->positionY + c->height) == s.positionY)
                    {
                        make_association(s, *c);
                        break;
                    }
                    
                }

                if(c->isHorizontal) break; // Dont need to check horizontals for nextComponent

                // Check next
                if((c->positionX) == (s.positionX) // See "Top" transition comment
                && c->positionY == (s.positionY + s.height))
                {
                    make_association(*c, s);
                    break;
                }

                break;
            }

            case e_ConnectionLine:
            {
                tempConnectionLine = nullptr;
                tempIdVector.clear();

                // Check previous
                if(c->isHorizontal)
                {
                    // Left
                    if(s.positionX == (c->positionX + c->width)
                    && c->positionY == (s.positionY + (s.height / 2)))
                    {
                        tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                        tempComponent = &check_connection_line(pg, *tempConnectionLine, s);                    
                        make_association(s, *tempComponent);
                        break;
                    }

                    // Right
                    if(!s.hasAction)
                    {
                        if(c->positionX == (s.positionX + s.width)
                        && c->positionY == (s.positionY + (s.height / 2)))
                        {
                            tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                            tempComponent = &check_connection_line(pg, *tempConnectionLine, s);                    
                            make_association(s, *tempComponent);
                            break;
                        }
                    }                   
                }

                // Top
                if(c->positionX == (s.positionX + (s.width / 2))
                && s.positionY == (c->positionY + c->height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, s);                    
                    make_association(s, *tempComponent);
                    break;
                }

                // Check next
                // Bottom
                if(c->positionX == (s.positionX + (s.width / 2))
                && c->positionY == (s.positionY + s.height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, s);                    
                    make_association(*tempComponent, s);
                    break;
                }

                break;
            }

            case e_Synchronization: // Only check previous
            {
                if((s.positionX + (s.width / 2)) >= c->positionX
                && (s.positionX + (s.width / 2)) <= (c->positionX + c->width)
                && (s.positionY + s.height) == c->positionY)
                {
                    make_association(*c, s);
                    break;
                }
                
                break;
            }

            case e_Parallelization: // Only check next
            {
                if((s.positionX + (s.width / 2)) >= c->positionX
                && (s.positionX + (s.width / 2)) <= (c->positionX + c->width)
                &&  s.positionY == (c->positionY + c->height))
                {
                    make_association(s, *c);
                    break;
                }

                break;
            }
            
            default: break;
        }
    }

    return;
}

void recheck_transition(Page &pg, Component &t)
{
    if(t.isHorizontal) return;
    if(!t.previousComponent.empty()) return;

    Component* tempComponent = nullptr;

    for(const auto& c : pg.all)
    {
        switch (c->type)
        {
            case e_NormalStep:
            case e_InitialStep:
            case e_MacroStep:
            case e_EnclosedStep:
            case e_EnclosedInitialStep:
            {
                if(t.positionX == c->positionX
                && t.positionY > c->positionY)
                {
                    if(tempComponent != nullptr)
                    {
                        if(tempComponent->positionY < c->positionY)
                            tempComponent = c;
                    }
                    else
                        tempComponent = c;
                }

                break;
            }

            default: break;
        }
    }

    if(tempComponent != nullptr)
        make_association(t, *tempComponent);

    return;
}

void check_jump(std::vector<Page*> &pgs, Page &pg, Component &j)
{
    Component* prevComp = nullptr;
    Component* nextComp = nullptr;
    Component* tempComponent = nullptr;
    Component* tempConnectionLine = nullptr;
    std::vector<int> tempIdVector; 

    // Find previous component
    for(const auto& c : pg.all)
    {
        switch(c->type)
        {
            case e_Transition:            
            {
                if((c->positionX) == (j.positionX) // X's align (no need for middle; also GRAFCET-Studio's values for width and height are weird ...)
                && (c->positionY + c->height) == j.positionY) prevComp = c;

                break;
            }

            case e_ConnectionLine:            
            {
                if(c->positionX == (j.positionX + (j.width / 2))
                && j.positionY == (c->positionY + c->height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, j);
                    prevComp = tempComponent;
                    break;
                }

                break;
            }
            
            default: break;
        }
    }

    std::string tempString;

    // Find step in "stepIdentifier" (need all pages)
    for(const auto& p : pgs)
    {
        for(const auto& c : p->all)
        {
            switch(c->type)
            {
                case e_NormalStep:
                case e_InitialStep:
                case e_MacroStep:
                case e_EnclosedStep:
                case e_EnclosedInitialStep:
                {
                    tempString = "";
                    for(int i = 5; i<c->name.size(); i++) tempString += c->name[i]; // i = 5 because "Step_" is 5 chars long

                    if(tempString == j.stepIdentifier) nextComp = c;

                    break;
                }                    
                
                default: break;
            }
        }
    }

    // Make associations for transition and jump's step in "stepIdentifier"
    // Transition's next component is the step associated to the jump
    // Step's (associated to the jump) previous component is the transition
    make_association(*nextComp, *prevComp);

    return;
}

void check_parallelization(Page &pg, Component &par)
{
    Component* tempComponent = nullptr;
    Component* tempConnectionLine = nullptr;
    std::vector<int> tempIdVector; 

    for(const auto& c : pg.all)
    {
        switch(c->type) // No need to check steps (see check_step())
        {
            case e_ConnectionLine:
            {
                // Check previous
                if(c->positionX >= par.positionX
                && c->positionX <= (par.positionX + par.width)
                && par.positionY == (c->positionY + c->height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, par);    
                    make_association(par, *tempComponent);                    
                    break;
                }

                // Check next
                if(c->positionX >= par.positionX
                && c->positionX <= (par.positionX + par.width)
                && c->positionY == (par.positionY + par.height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, par); 
                    make_association(*tempComponent, par);                    
                    break;
                }

                break;
            }

            case e_Transition: // Only check previous
            {
                // 16 because GRAFCET-Studio's transitions are not centered ...
                if((c->positionX + 16) >= par.positionX 
                && (c->positionX + 16) <= (par.positionX + par.width)
                && (c->positionY + c->height) == par.positionY)
                {
                    make_association(par, *c);
                    break;
                }
                
                break;
            }
        }        
    }

    return;
}

void check_synchronization(Page &pg, Component &syn)
{
    Component* tempComponent = nullptr;
    Component* tempConnectionLine = nullptr;
    std::vector<int> tempIdVector; 

    for(const auto& c : pg.all)
    {
        switch(c->type) // No need to check steps (see check_step())
        {
            case e_ConnectionLine:
            {
                // Check previous
                if(c->positionX >= syn.positionX
                && c->positionX <= (syn.positionX + syn.width)
                && syn.positionY == (c->positionY + c->height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, syn);
                    make_association(syn, *tempComponent);
                    break;
                }

                // Check next
                if(c->positionX >= syn.positionX
                && c->positionX <= (syn.positionX + syn.width)
                && c->positionY == (syn.positionY + syn.height))
                {
                    tempConnectionLine = &connection_line_iteration(pg, *c, tempIdVector);
                    tempComponent = &check_connection_line(pg, *tempConnectionLine, syn);
                    make_association(*tempComponent, syn);
                    break;
                }

                break;
            }

            case e_Transition: // Only check next
            {
                // 16 because GRAFCET-Studio's horizontal transitions are not centered ...
                if((c->positionX + 16) >= syn.positionX 
                && (c->positionX + 16) <= (syn.positionX + syn.width)
                && (c->positionY == (syn.positionY + syn.height)))
                {
                    make_association(*c, syn);
                    break;
                }
                
                break;
            }
        }        
    }

    return;
}

Component& connection_line_iteration(Page &pg, Component &cline, std::vector<int> &ignoreIds)
{
    Component* tempConnectionLine = nullptr;

    bool skipFlag = false;
    
    ignoreIds.push_back(cline.identifier);

    for(const auto& c : pg.all)
    {
        // Skip component c if connection line already part of iteration chain    
        for(const auto& id : ignoreIds) if(c->identifier == id) skipFlag = true;
        if(skipFlag) {skipFlag = false; continue;}
        
        if(c->type == e_ConnectionLine)
        {
            if(c->isHorizontal == false && cline.isHorizontal == false)
            {
                if(cline.positionX == c->positionX
                && ((cline.positionY + cline.height) == c->positionY
                || (c->positionY + c->height) == cline.positionY))
                {
                    std::cout << "---" << std::endl;
                    std::cout << c->identifier << " | " << cline.identifier << std::endl;
                    std::cout << c->positionX << "," << c->positionY << " | " << cline.positionX << "," << cline.positionY << std::endl;
                    tempConnectionLine = c;
                    break;
                }
            }
            else if(c->isHorizontal == false && cline.isHorizontal == true)
            {
                if(c->positionX >= cline.positionX
                && c->positionX <= (cline.positionX + cline.width)
                && (c->positionY == cline.positionY
                || (c->positionY + c->height) == cline.positionY))
                {
                    tempConnectionLine = c;
                    break;
                }
            }
            else if(c->isHorizontal == true && cline.isHorizontal == false)
            {
                if(cline.positionX >= c->positionX
                && cline.positionX <= (c->positionX + c->width)
                && (cline.positionY == c->positionY
                || (cline.positionY + cline.height) == c->positionY))
                {
                    tempConnectionLine = c;
                    break;
                }
            }
            else if(c->isHorizontal == true && cline.isHorizontal == true)
            {
                if(cline.positionY == c->positionY
                && ((cline.positionX + cline.width) == c->positionX
                || (c->positionX + c->width) == cline.positionX))
                {
                    tempConnectionLine = c;
                    break;
                }
            }
        }
        else continue;
    }

    if(tempConnectionLine == nullptr) return cline;    
    else return connection_line_iteration(pg, *tempConnectionLine, ignoreIds);    
}

Component& check_connection_line(Page &pg, Component &cline, Component mainComponent)
{
    Component* tempComponent = nullptr;

    for(const auto& c : pg.all)
    {
        if(c->identifier == mainComponent.identifier) continue;
        
        switch(c->type)
        {
            case e_NormalStep:
            case e_InitialStep:
            case e_MacroStep:
            case e_EnclosedStep:
            case e_EnclosedInitialStep:
            {
                if(cline.isHorizontal)
                {
                    // Left
                    if((cline.positionX + cline.width) == c->positionX
                    && (cline.positionY == (c->positionY + (c->height / 2))))
                    {
                        tempComponent = c;
                        break;
                    }

                    // Right
                    if(!c->hasAction)
                    {
                        if(cline.positionX == (c->positionX + c->width)
                        && cline.positionY == (c->positionY + (c->height / 2)))
                        {
                            tempComponent = c;
                            break;
                        }
                    }

                    break;
                }

                // Top
                if(cline.positionX == (c->positionX + (c->width / 2))
                && (cline.positionY + cline.height) == c->positionY)
                {
                    tempComponent = c;
                    break;
                }

                // Bottom
                if(cline.positionX == (c->positionX + (c->width / 2))
                && cline.positionY == (c->positionY + c->height))
                {
                    tempComponent = c;
                    break;
                }
                
                break;
            }

            case e_Transition:
            {
                if(!c->isHorizontal)
                {
                    if(cline.isHorizontal)
                    {                    
                        // Top
                        if((c->positionX + 16) >= cline.positionX // 16 because GRAFCET-Studio's transitions are not centered ...
                        && (c->positionX + 16) <= (cline.positionX + cline.width) // 16 because GRAFCET-Studio's transitions are not centered ...
                        && c->positionY == cline.positionY)
                        {
                            tempComponent = c;
                            break;
                        }

                        // Bottom
                        if((c->positionX + 16) >= cline.positionX // 16 because GRAFCET-Studio's transitions are not centered ...
                        && (c->positionX + 16) <= (cline.positionX + cline.width) // 16 because GRAFCET-Studio's transitions are not centered ...
                        && (c->positionY + c->height) == cline.positionY)
                        {
                            tempComponent = c;
                            break;
                        }

                        break;
                    }

                    // Top
                    if(cline.positionX == (c->positionX + 16) // 16 because GRAFCET-Studio's transitions are not centered ...
                    && (cline.positionY + cline.height) == c->positionY)
                    {
                        tempComponent = c;
                        break;
                    }

                    // Bottom
                    if(cline.positionX == (c->positionX + 16)
                    && cline.positionY == (c->positionY + c->height))
                    {
                        tempComponent = c;
                        break;
                    }

                    break;
                }

                if(cline.isHorizontal)
                {                    
                    // Left
                    if(c->positionX == (cline.positionX + cline.width)
                    && (c->positionY + 32) == cline.positionY) // 32 because GRAFCET-Studio's transitions are not centered ...
                    {
                        tempComponent = c;
                        break;
                    }

                    // Right
                    if((c->positionX + c->width) == cline.positionX
                    && (c->positionY + 32) == cline.positionY) // 32 because GRAFCET-Studio's transitions are not centered ...
                    {
                        tempComponent = c;
                        break;
                    }

                    break;
                }

                // Left
                if(cline.positionX == c->positionX
                && cline.positionY <= (c->positionY + 32) // 32 because GRAFCET-Studio's transitions are not centered ...
                && (cline.positionY + cline.height) >= (c->positionY + 32)) // 32 because GRAFCET-Studio's transitions are not centered ...
                {
                    tempComponent = c;
                    break;
                }

                // Right
                if(cline.positionX == (c->positionX + c->width)
                && cline.positionY <= (c->positionY + 32) // 32 because GRAFCET-Studio's transitions are not centered ...
                && (cline.positionY + cline.height) >= (c->positionY + 32)) // 32 because GRAFCET-Studio's transitions are not centered ...
                {
                    tempComponent = c;
                    break;
                }

                break;
            }

            case e_Parallelization:
            case e_Synchronization:
            {
                if(cline.isHorizontal) continue;

                // Top
                if(cline.positionX >= c->positionX
                && cline.positionX <= (c->positionX + c->width)
                && (cline.positionY + cline.height) == c->positionY)
                {
                    tempComponent = c;
                    break;                    
                }

                // Bottom
                if(cline.positionX >= c->positionX
                && cline.positionX <= (c->positionX + c->width)
                && cline.positionY == (c->positionY + c->height))
                {
                    tempComponent = c;
                    break;   
                }

                break;
            }         
        
            default: break;
        }
    }

    if(tempComponent == nullptr)
    {
        std::cout << "check_connection_line(): tempComponent is nullptr" << std::endl;
        exit(EXIT_FAILURE);
    }

    return *tempComponent;
}

// Makes the previous/next component associations for both component a and b
// Func additionally checks if association already made
// a -> component given previousComponent association
// b -> component given nextComponent association
void make_association(Component &a, Component &b)
{
    bool aFlag = true;
    bool bFlag = true;

    for(const auto& c : a.previousComponent) if(c->identifier == b.identifier) {aFlag = false; break;}
    for(const auto& c : b.nextComponent) if(c->identifier == a.identifier) {bFlag = false; break;}

    if(aFlag) a.previousComponent.push_back(&b);
    if(bFlag) b.nextComponent.push_back(&a);

    return;
}