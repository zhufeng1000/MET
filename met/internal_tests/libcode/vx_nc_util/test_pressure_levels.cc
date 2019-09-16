// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
// ** Copyright UCAR (c) 1992 - 2019
// ** University Corporation for Atmospheric Research (UCAR)
// ** National Center for Atmospheric Research (NCAR)
// ** Research Applications Lab (RAL)
// ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
// *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
////////////////////////////////////////////////////////////////////////

using namespace std;

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <cmath>

#include "vx_util.h"
#include "vx_tc_nc_util.h"

////////////////////////////////////////////////////////////////////////

static ConcatString program_name;

////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

    program_name = get_short_name(argv[0]);

    map<string, vector<string> > variable_levels;

    vector<string> u_levels;
    u_levels.push_back("P1000");
    u_levels.push_back("P800");
    u_levels.push_back("P500");
    variable_levels["U"] = u_levels;

    vector<string> v_levels;
    v_levels.push_back("P1000");
    v_levels.push_back("P900");
    v_levels.push_back("P700");
    v_levels.push_back("P500");
    variable_levels["V"] = v_levels;

    for (map<string, vector<string> >::iterator i = variable_levels.begin();
        i != variable_levels.end(); ++i) {
        cout << i->first << ": ";
        vector<string> levels = variable_levels[i->first];
        for (int j = 0; j < levels.size(); j++) {
            cout << levels[j] << " ";
        }
        cout << endl;
    }

    set<float> all_levels
        = pressure_levels(variable_levels);

    for (set<float>::iterator i = all_levels.begin();
        i != all_levels.end(); ++i) {
            cout << *i << " ";
    }
    cout << endl;

    return 0;
}

////////////////////////////////////////////////////////////////////////