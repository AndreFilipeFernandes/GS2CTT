#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "../aux_structs/aux_structs.hpp"

// Function declaration
void check_action(Page &pg, Component &act);
void check_groupBorder(std::vector<Page*> &pgs, Page &pg, Component &gb);
Component& groupborder_associated_step(std::vector<Page*> &pgs, Component &gb);
void check_step(Page &pg, Component &s);
void recheck_transition(Page &pg, Component &t);
void check_jump(std::vector<Page*> &pgs, Page &pg, Component &j);
void check_parallelization(Page &pg, Component &par);
void check_synchronization(Page &pg, Component &syn);
Component& connection_line_iteration(Page &pg, Component &cline, std::vector<int> &ignoreIds);
Component& check_connection_line(Page &pg, Component &cline, Component mainComponent);
void make_association(Component &a, Component &b);