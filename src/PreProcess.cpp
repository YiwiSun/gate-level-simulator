
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#include "PreProcess.h"
        
using namespace std;      
//! Instance a new VCD file container.
PreProcess::PreProcess(){

}
        
//! Destructor
PreProcess::~PreProcess(){

    // Delete signal values.
    /*
    for(std::unordered_map<std::string, ModuleType*>::iterator hash_val = module_types.begin();
             hash_val != module_types.end();
             ++hash_val)
    {
        for(auto vals = hash_val . second . begin();
                 vals != hash_val . second . end();
                 ++vals)
        {
            delete (*vals) . value;
            delete *vals;
        }

        delete hash_val . second;
    }*/

}


/*!
@brief Add a new scope object to the VCD file
*/

void PreProcess::parse_vlib(std::string& vlib_path){
    ifstream inf(vlib_path);
    if (!inf)
    {
        cout << "File " << vlib_path << " Open Error!" << endl;
        cout << "Exit!" << endl;
        exit(-1);
    }
    
    while(!inf.eof())
    {
        string line;
        getline(inf, line);
        line.erase(0, line.find_first_not_of(" "));
        if (line == "")
        {
            continue;
        }
        std::vector<std::string> tmp;
        std::string delims = " \t\r\n,;()";
        tmp = Split(line, delims);
        if (tmp.size() == 0)
            continue;
        if (tmp[0] == "")
        {
            cout << "ERROR: string " << line << endl;
            exit(-1);
        }
        transform(tmp[0].begin(), tmp[0].end(), tmp[0].begin(), ::tolower);
        //parse module
        if (tmp[0] == "module")
        {
            
            assert(tmp.size()>1);
            string module_name = tmp[1];
            ModuleType cur_module;
            cur_module.name = module_name;
            while(getline(inf, line))
            {
                line.erase(0, line.find_first_not_of(" "));
                if (line == "")
                {
                    continue;
                }
                delims = " \t\r\n,;()";
                tmp = Split(line, delims);
                if (tmp.size() == 0)
                    continue;
                if (tmp[0] == "")
                {
                    cout << "ERROR: string " << line << endl;
                    exit(-1);
                }
                transform(tmp[0].begin(), tmp[0].end(), tmp[0].begin(), ::tolower);
                // parse Pin
                if (tmp[0] == "input" || tmp[0] == "output" || tmp[0] == "wire" || tmp[0] == "reg" || tmp[0] == "inout")
                {
                    assert(tmp.size()>1);
                    
                    if (tmp[1].find("[") != std::string::npos)
                    {
                        assert(tmp.size() > 2);
                        Pin cur_pin;
                        cur_pin.type = tmp[0];
                        cur_pin.name = tmp[2];
                        cur_pin.lindex = stoi(tmp[1].substr(tmp[1].find("[")+1, tmp[1].find(":")-tmp[1].find("[")-1));
                        cur_pin.rindex = stoi(tmp[1].substr(tmp[1].find(":")+1, tmp[1].find("]")-tmp[1].find(":")-1));
                        cur_pin.size = max(cur_pin.lindex, cur_pin.rindex) - min(cur_pin.lindex, cur_pin.rindex) + 1;
                        (cur_module.ports)[tmp[2]] = cur_pin;
                        if (tmp[0] == "input" || tmp[0] == "output" || tmp[0] == "inout")
                        {
                            cur_module.in_out_ports.push_back(tmp[2]);
                        }
                    }
                    else
                    {
                        for (unsigned i = 1; i < tmp.size(); ++i)
                        {
                            Pin cur_pin;
                            cur_pin.type = tmp[0];
                            cur_pin.name = tmp[i];
                            cur_pin.lindex = -1;
                            cur_pin.rindex = -1;
                            cur_pin.size = 1;
                            (cur_module.ports)[tmp[i]] = cur_pin;
                            if (tmp[0] == "input" || tmp[0] == "output" || tmp[0] == "inout")
                            {
                                cur_module.in_out_ports.push_back(tmp[i]);
                            }
                        }
                    }
                }
                // parse module function -- and
                else if (tmp[0] == "and")
                {
                    assert(tmp.size()>3);
                    Function cur_func("and",0);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- or
                else if (tmp[0] == "or")
                {
                    assert(tmp.size()>3);
                    Function cur_func("or",1);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- xor
                else if (tmp[0] == "xor")
                {
                    assert(tmp.size()>3);
                    Function cur_func("xor",2);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- xnor
                else if (tmp[0] == "xnor")
                {
                    assert(tmp.size()>3);
                    Function cur_func("xnor",3);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- nor
                else if (tmp[0] == "nor")
                {
                    assert(tmp.size()>3);
                    Function cur_func("nor",4);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- nand
                else if (tmp[0] == "nand")
                {
                    assert(tmp.size()>3);
                    Function cur_func("nand",5);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- buf
                else if (tmp[0] == "buf")
                {
                    assert(tmp.size()>2);
                    Function cur_func("buf",6);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module function -- not
                else if (tmp[0] == "not")
                {
                    assert(tmp.size()>2);
                    Function cur_func("not",7);
                    cur_func.set_out(tmp[1]);
                    for (unsigned i = 2; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module primitive function -- udp_xbuf
                else if (tmp[0] == "udp_xbuf")
                {
                    assert(tmp.size() == 5);
                    Function cur_func("udp_xbuf",8);
                    cur_func.set_out(tmp[2]);
                    for (unsigned i = 3; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse module primitive function -- udp_mux2
                else if (tmp[0] == "udp_mux2")
                {
                    assert(tmp.size() == 6);
                    Function cur_func("udp_mux2",9);
                    cur_func.set_out(tmp[2]);
                    for (unsigned i = 3; i < tmp.size(); ++i)
                    {
                        if (tmp[i] == "1\'b1")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply1_vec).push_back(tmp_name);
                        }
                        else if (tmp[i] == "1\'b0")
                        {
                            string tmp_name;
                            do{
                                srand((int)time(0));
                                tmp_name = "zc_net" + to_string(rand()%1000);
                            }while((cur_module.ports).find(tmp_name) != (cur_module.ports).end());
                            cur_func.set_in(tmp_name);
                            Pin tmp_pin;
                            tmp_pin.type = "wire";
                            tmp_pin.name = tmp_name;
                            tmp_pin.size = 1;
                            tmp_pin.lindex = -1;
                            tmp_pin.rindex = -1;
                            (cur_module.ports)[tmp_name] = tmp_pin;
                            (cur_module.supply0_vec).push_back(tmp_name);
                        }
                        else{
                            cur_func.set_in(tmp[i]);
                        }
                        
                    }
                    (cur_module.function).push_back(cur_func);
                }
                // parse supply0
                else if (tmp[0] == "supply0")
                {
                    for (unsigned i = 0; i < tmp.size(); ++i)
                    {
                        cur_module.supply0_vec.push_back(tmp[i]);
                    }
                }
                // parse supply1
                else if (tmp[0] == "supply1")
                {
                    for (unsigned i = 0; i < tmp.size(); ++i)
                    {
                        cur_module.supply1_vec.push_back(tmp[i]);
                    }
                }
                //ignored module --- for parse gv
                else if (tmp[0] == "udp_dff" || tmp[0] == "udp_tlat" || tmp[0] == "always")
                {
                    ignored_modules.push_back(module_name);
                    break;
                }
                else if(tmp[0] == "endmodule")
                {
                    vlib_add_module(cur_module.name, cur_module);
                    break;
                }
            }
        }

    }
    cout << "INFO: vlib file's parsing done!" << endl;
    inf.close();
}

void PreProcess::parse_gv(std::string& gv_path){
    ifstream inf(gv_path);
    if (!inf)
    {
        cout << "File " << gv_path << " Open Error!" << endl;
        cout << "Exit!" << endl;
        exit(-1);
    }
    string full_line = "";
    bool end_of_line_found = false;
    bool process_mode = false;
    while(!inf.eof())
    {
        string line;
        getline(inf, line);
        line.erase(0, line.find_first_not_of(" "));
        line.erase(line.find_last_not_of(" ")+1);
        if (line == "")
        {
            continue;
        }
        std::vector<std::string> tmp;
        std::string delims = " \t\r\n";
        tmp = Split(line, delims);
        if (tmp.size() == 0)
        {
            continue;
        }
        if (tmp[0] == "")
        {
            cout << "ERROR: string " << line << endl;
            exit(-1);
        }
        transform(tmp[0].begin(), tmp[0].end(), tmp[0].begin(), ::tolower);
        if (tmp[0] == "module")
        {
            process_mode = true;
            top_module_name = tmp[1];
            continue;
        }
        else if (tmp[0] == "endmodule")
        {
            process_mode = false;
            break;
        }
        //parse start
        if (process_mode && !end_of_line_found)
        {
            // combine lines
            line.erase(0, line.find_first_not_of(" "));
            line.erase(line.find_last_not_of(" ")+1);
            string new_line = line;
            full_line = full_line + " " + new_line;
            if (new_line.size() > 0 && new_line[new_line.size()-1] == ';')
            {
                tmp = Split(full_line, " \t\r\n");
                full_line = "";
                end_of_line_found = false;
                if (tmp[0] == "input" || tmp[0] == "output" || tmp[0] == "wire" || tmp[0] == "inout")
                {
                    if (tmp[1].find("[") != std::string::npos)
                    {
                        assert(tmp.size() > 2);
                        Pin cur_pin;
                        cur_pin.type = tmp[0];
                        string tmp_name;
                        if(tmp[2][tmp[2].size()-1] == ',' || tmp[2][tmp[2].size()-1] == ';')
                            tmp_name = tmp[2].substr(0,tmp[2].size()-1);
                        else
                            tmp_name = tmp[2];
                        //cout << "pin name: " << tmp_name << "   ---";
                        cur_pin.name = tmp_name;
                        cur_pin.lindex = stoi(tmp[1].substr(tmp[1].find("[")+1, tmp[1].find(":")-tmp[1].find("[")-1));
                        //cout << "lindex: " << cur_pin.lindex << " ";
                        cur_pin.rindex = stoi(tmp[1].substr(tmp[1].find(":")+1, tmp[1].find("]")-tmp[1].find(":")-1));
                        //cout << "rindex: " << cur_pin.rindex << " ";
                        cur_pin.size = max(cur_pin.lindex, cur_pin.rindex) - min(cur_pin.lindex, cur_pin.rindex) + 1;
                        //cout << "size: " << cur_pin.size << " ";
                        pins[tmp_name] = cur_pin;
                        int i = cur_pin.lindex;
                        for (int j = 0; j < cur_pin.size; ++j)
                        {
                            string _name = tmp_name + "[" + to_string(i) + "]";
                            pin_bits.push_back(_name);
                            if (cur_pin.lindex > cur_pin.rindex)
                            {
                                i--;
                            }
                            else{
                                i++;
                            }
                        }
                        //initialize pinbitValue
                        /*for (unsigned i = min(cur_pin.lindex, cur_pin.rindex); i < cur_pin.size; ++i)
                        {
                            string _cur_pin = tmp_name + "[" + to_string(i) + "]";
                            //cout << " " << _cur_pin;
                            if (pinbitValues.find(_cur_pin) == pinbitValues.end())
                            {
                                pinbitValues[_cur_pin] = new TimedValues();
                            }
                        }*/
                        //cout << endl;
                    }
                    else
                    {
                        for (unsigned i = 1; i < tmp.size(); ++i)
                        {
                            Pin cur_pin;
                            cur_pin.type = tmp[0];
                            string tmp_name;
                            if(tmp[i][tmp[i].size()-1] == ',' || tmp[i][tmp[i].size()-1] == ';')
                                tmp_name = tmp[i].substr(0,tmp[i].size()-1);
                            else
                                tmp_name = tmp[i];
                            cur_pin.name = tmp_name;
                            cur_pin.lindex = -1;
                            cur_pin.rindex = -1;
                            cur_pin.size = 1;
                            pins[tmp_name] = cur_pin;
                            pin_bits.push_back(tmp_name);
                            //initialize pinbitValue
                            /*if (pinbitValues.find(tmp_name) == pinbitValues.end())
                            {
                                pinbitValues[tmp_name] = new TimedValues();
                            }*/
                        }
                    }
                }
                //use pinbitValue
                else if(tmp[0] == "assign")
                {
                    if (tmp.size() == 2)
                    {
                        assert(tmp[1].find("=") != std::string::npos);
                        assert(tmp[1][tmp[1].size()-1] == ';');
                        string left = tmp[1].substr(0, tmp[1].find("="));
                        string right = tmp[1].substr(tmp[1].find("=")+1, tmp[1].size()-tmp[1].find("=")-2);
                        if (right == "1\'b1" || right == "1\'b0")  //pinbit value
                        {
                            TimedValues* tvs = new TimedValues();
                            TimedValue t_val;
                            t_val.t = 0;
                            t_val.value = (right == "1\'b1"?VCD_1:VCD_0);
                            tvs->push_back(t_val);
                            pinbitValues[left] = tvs;
                            std::vector<unsigned long long> v(3, 0);
                            v[0] = (right == "1\'b1"?0:1);
                            v[1] = (right == "1\'b1"?1:0);
                            Result[left] = v;
                            //pinbitValues[left].push_back(t_val);

                        }
                        else{
                            assign_pairs[left] = right;
                        }
                    }
                    else if (tmp.size() == 3)
                    {
                        string left,right;
                        assert(((tmp[1].find("=") != std::string::npos) || (tmp[2].find("=") != std::string::npos)) && tmp[2][tmp[2].size()-1] == ';');
                        if (tmp[1].find("=") != std::string::npos)
                        {
                            left = tmp[1].substr(0, tmp[1].find("="));
                            right = tmp[2].substr(0,tmp[2].size()-1);
                        }
                        else if (tmp[2].find("=") != std::string::npos)
                        {
                            left = tmp[1];
                            right = tmp[2].substr(tmp[2].find("=")+1,tmp[2].size()-tmp[2].find("=")-2);
                        }
                        if (right == "1\'b1" || right == "1\'b0")  //pinbit value
                        {
                            TimedValues* tvs = new TimedValues();
                            TimedValue t_val;
                            t_val.t = 0;
                            t_val.value = (right == "1\'b1"?VCD_1:VCD_0);
                            tvs->push_back(t_val);
                            pinbitValues[left] = tvs;
                            std::vector<unsigned long long> v(3, 0);
                            v[0] = (right == "1\'b1"?0:1);
                            v[1] = (right == "1\'b1"?1:0);
                            Result[left] = v;
                            //pinbitValues[left].push_back(t_val);

                        }
                        else{
                            assign_pairs[left] = right;
                        }
                    }
                    else if (tmp.size() >= 4)
                    {
                        assert(tmp[2] == "=" && tmp[tmp.size()-1][tmp[tmp.size()-1].size()-1] == ';');
                        string left = tmp[1];
                        string right = (tmp.size() == 4)?tmp[3].substr(0, tmp[3].size()-1):tmp[3];
                        if (right == "1\'b1" || right == "1\'b0")  //pinbit value
                        {
                            TimedValues* tvs = new TimedValues();
                            TimedValue t_val;
                            t_val.t = 0;
                            t_val.value = (right == "1\'b1"?VCD_1:VCD_0);
                            tvs->push_back(t_val);
                            pinbitValues[left] = tvs;
                            std::vector<unsigned long long> v(3, 0);
                            v[0] = (right == "1\'b1"?0:1);
                            v[1] = (right == "1\'b1"?1:0);
                            Result[left] = v;
                             //pinbitValues[left].push_back(t_val);
                        }
                        else{
                            assign_pairs[left] = right;
                        }
                    }
                }
                //parse instance call
                else
                {
                    if (module_types.find(tmp[0]) == module_types.end())
                    {
                        cout << "WARNING: skip ignored module " << tmp[0] <<endl;
                        continue;
                    }
                    string ins_type = tmp[0];
                    ModuleType cur_module = module_types[ins_type];
                    std::vector<string> module_name_vec;
                    std::map<std::string, Pin>::iterator iter;
                    for (unsigned iter = 0; iter < cur_module.in_out_ports.size(); iter++){
                        module_name_vec.push_back((cur_module.in_out_ports)[iter]);
                    }
                    string tmp_line = "";
                    for (unsigned i = 1; i < tmp.size(); ++i)
                    {
                        tmp_line += tmp[i];
                    }
                    std::vector<string> instance_vec = Split(tmp_line, ".");
                    string instance_name = instance_vec[0].replace(instance_vec[0].find("("),1,"");
                    //cout << "instance name: " << instance_name << endl;
                    Instance cur_inst(instance_name, cur_module);
                    instance_name_vec.push_back(instance_name);
                    for (unsigned i = 1; i < instance_vec.size(); ++i)
                    {
                        string temp = instance_vec[i];
                        string final_str;
                        // not combined pins
                        if (temp.find("{")==std::string::npos && temp.find("}")==std::string::npos)
                        {
                            final_str = temp.replace(temp.find("("),1," ");
                            while(final_str.find(")")!=std::string::npos)
                                final_str = final_str.replace(final_str.find(")"),1,"");
                            if (temp.find(";")!=std::string::npos)
                            {
                                final_str = final_str.replace(final_str.find(";"),1,"");
                            }
                            else
                                final_str = final_str.replace(final_str.find(","),1,"");
                            std::vector<string> new_vec = Split(final_str, " \t\r\n");
                            string cur_type_pin = new_vec[0];
                            string cur_inst_pin = new_vec[1];
                            assert(cur_module.ports.find(cur_type_pin)!=cur_module.ports.end());
                            //del
                            std::vector<string>::iterator it = find(module_name_vec.begin(),module_name_vec.end(),cur_type_pin);
                            module_name_vec.erase(it);

                            unsigned type_pin_size = (cur_module.ports)[cur_type_pin].size;
                            int type_pin_lindex = (cur_module.ports)[cur_type_pin].lindex;
                            int type_pin_rindex = (cur_module.ports)[cur_type_pin].rindex;
                            // multiple bits pin
                            if (type_pin_size > 1)
                            {
                                int k = 0;
                                int ll = -1;
                                int rr = -1;
                                if (cur_inst_pin.find("[")!=std::string::npos)
                                {
                                    ll = stoi(cur_inst_pin.substr(cur_inst_pin.find("[")+1, cur_inst_pin.find(":")-cur_inst_pin.find("[")-1));
                                    rr = stoi(cur_inst_pin.substr(cur_inst_pin.find(":")+1, cur_inst_pin.find("]")-cur_inst_pin.find(":")-1));
                                    cur_inst_pin = cur_inst_pin.substr(0, cur_inst_pin.find("["));
                                    k = min(ll, rr);
                                }
                                assert(pins.find(cur_inst_pin)!=pins.end());
                                if (ll == -1 && rr == -1)
                                {
                                    ll = pins[cur_inst_pin].lindex;
                                    rr = pins[cur_inst_pin].rindex;
                                    k = min(ll, rr);
                                }
                                assert((max(ll,rr)-min(ll,rr)+1)==type_pin_size);
                                assert((ll-type_pin_lindex) == (rr-type_pin_rindex));
                                for (unsigned j = min(type_pin_lindex, type_pin_rindex); j < type_pin_size; ++j, ++k)
                                {
                                    string cur_port = cur_type_pin + "[" + to_string(j) + "]";
                                    string cur_net = cur_inst_pin + "[" + to_string(k) + "]";
                                    if ((cur_module.ports)[cur_type_pin].type == "input")
                                    {
                                        cur_inst.set_in(cur_port, cur_net);
                                        int flag = instance_name_vec.size() - 1;
                                        net_instance_map[cur_net].push_back(flag);
                                        cur_inst.in_net.push_back(cur_net);
                                    }
                                    else if ((cur_module.ports)[cur_type_pin].type == "output")
                                    {
                                        cur_inst.set_out(cur_port, cur_net);
                                        cur_inst.out_net.push_back(cur_net);
                                        int flag = instance_name_vec.size() - 1;
                                        out_net_from_id[cur_net] = flag;
                                    }
                                    else{
                                        cout << "ERROR: Pin " << cur_type_pin << " must be input OR output." <<endl;
                                        exit(-1);
                                    }
                                }
                            }
                            // single bit pin
                            else
                            {
                                assert(type_pin_size == 1);
                                assert(type_pin_lindex == -1);
                                assert(type_pin_rindex == -1);
                                if ((cur_module.ports)[cur_type_pin].type == "input")
                                {
                                    if (cur_inst_pin == "1\'b1" || cur_inst_pin == "1\'b0")
                                    {
                                        string tmp_name;
                                        do{
                                            srand((int)time(0));
                                            tmp_name = "gv_net" + to_string(rand()%1000);
                                        }while(pins.find(tmp_name) != pins.end());
                                        cur_inst.set_in(cur_type_pin, tmp_name);
                                        cur_inst.in_net.push_back(tmp_name);
                                        int flag = instance_name_vec.size() - 1;
                                        net_instance_map[tmp_name].push_back(flag);
                                        Pin tmp_pin;
                                        tmp_pin.type = "wire";
                                        tmp_pin.name = tmp_name;
                                        tmp_pin.size = 1;
                                        tmp_pin.lindex = -1;
                                        tmp_pin.rindex = -1;
                                        pins[tmp_name] = tmp_pin;
                                        //add pinbitValue
                                        TimedValues* tvs = new TimedValues();
                                        TimedValue t_val;
                                        t_val.t = 0;
                                        t_val.value = (cur_inst_pin == "1\'b1"?VCD_1:VCD_0);
                                        tvs->push_back(t_val);
                                        pinbitValues[tmp_name] = tvs;
                                    }
                                    else{
                                        assert(pins.find(cur_inst_pin)!=pins.end() || cur_inst_pin.find("[")!=std::string::npos);
                                        cur_inst.set_in(cur_type_pin, cur_inst_pin);
                                        cur_inst.in_net.push_back(cur_inst_pin);
                                        int flag = instance_name_vec.size() - 1;
                                        net_instance_map[cur_inst_pin].push_back(flag);
                                    }
                                    
                                }
                                else if ((cur_module.ports)[cur_type_pin].type == "output")
                                {
                                    if (cur_inst_pin == "1\'b1" || cur_inst_pin == "1\'b0")
                                    {
                                        string tmp_name;
                                        do{
                                            srand((int)time(0));
                                            tmp_name = "gv_net" + to_string(rand()%1000);
                                        }while(pins.find(tmp_name) != pins.end());
                                        cur_inst.set_out(cur_type_pin, tmp_name);
                                        cur_inst.out_net.push_back(tmp_name);
                                        int flag = instance_name_vec.size() - 1;
                                        out_net_from_id[tmp_name] = flag;
                                        Pin tmp_pin;
                                        tmp_pin.type = "wire";
                                        tmp_pin.name = tmp_name;
                                        tmp_pin.size = 1;
                                        tmp_pin.lindex = -1;
                                        tmp_pin.rindex = -1;
                                        pins[tmp_name] = tmp_pin;
                                        //add pinbitValue
                                        TimedValues* tvs = new TimedValues();
                                        TimedValue t_val;
                                        t_val.t = 0;
                                        t_val.value = (cur_inst_pin == "1\'b1"?VCD_1:VCD_0);
                                        tvs->push_back(t_val);
                                        pinbitValues[tmp_name] = tvs;
                                    }
                                    else{
                                        assert(pins.find(cur_inst_pin)!=pins.end() || cur_inst_pin.find("[")!=std::string::npos);
                                        cur_inst.set_out(cur_type_pin, cur_inst_pin);
                                        cur_inst.out_net.push_back(cur_inst_pin);
                                        int flag = instance_name_vec.size() - 1;
                                        out_net_from_id[cur_inst_pin] = flag;
                                    }
                                }
                                else{
                                    cout << "ERROR: Pin " << cur_type_pin << " must be input OR output." <<endl;
                                    exit(-1);
                                }
                            }
                        }
                        // combined pins with "{" and "}"
                        else
                        {
                            final_str = temp.replace(temp.find("("),1," ");
                            while(final_str.find(")")!=std::string::npos)
                                final_str = final_str.replace(final_str.find(")"),1,"");
                            if (temp.find(";")!=std::string::npos)
                            {
                                final_str = final_str.replace(final_str.find(";"),1,"");
                            }
                            else
                                final_str = final_str.replace(final_str.rfind(","),1,"");
                            std::vector<string> new_vec = Split(final_str, " \t\r\n");
                            string cur_type_pin = new_vec[0];
                            string cur_inst_pin = new_vec[1];
                            assert(cur_module.ports.find(cur_type_pin)!=cur_module.ports.end());
                            //del
                            std::vector<string>::iterator it = find(module_name_vec.begin(),module_name_vec.end(),cur_type_pin);
                            module_name_vec.erase(it);
                            
                            unsigned type_pin_size = (cur_module.ports)[cur_type_pin].size;
                            int type_pin_lindex = (cur_module.ports)[cur_type_pin].lindex;
                            int type_pin_rindex = (cur_module.ports)[cur_type_pin].rindex;
                            assert(type_pin_size > 1);
                            // multiple bits pin
                            if (type_pin_size > 1)
                            {
                                cur_inst_pin = cur_inst_pin.replace(cur_inst_pin.find("{"),1,"");
                                cur_inst_pin = cur_inst_pin.replace(cur_inst_pin.find("}"),1,"");
                                std::vector<string> cur_inst_pin_vec = Split(cur_inst_pin, ",");
                                int l = type_pin_lindex;
                                for (unsigned j = 0; j < cur_inst_pin_vec.size(); ++j)
                                {
                                    string _pin = cur_inst_pin_vec[j];
                                    if (_pin.find("[")!=std::string::npos)
                                    {
                                        string _final_inst_pin = _pin.substr(0, _pin.find("["));
                                        assert(pins.find(_final_inst_pin)!=pins.end());
                                        int _l = stoi(_pin.substr(_pin.find("[")+1, _pin.find(":")-_pin.find("[")-1));
                                        int _r = stoi(_pin.substr(_pin.find(":")+1, _pin.find("]")-_pin.find(":")-1));
                                        assert(_l!=_r);
                                        while(1)
                                        {
                                            string final_pin = cur_type_pin + "[" + to_string(l) + "]";
                                            string final_inst_pin = _final_inst_pin + "[" + to_string(_l) + "]";
                                            //cout << "\t" << final_pin << "=" << final_inst_pin <<endl;
                                            if((cur_module.ports)[cur_type_pin].type == "output"){
                                                cur_inst.set_out(final_pin, final_inst_pin);
                                                cur_inst.out_net.push_back(final_inst_pin);
                                                int flag = instance_name_vec.size() - 1;
                                                out_net_from_id[final_inst_pin] = flag;
                                            }
                                            else{
                                                cur_inst.set_in(final_pin, final_inst_pin);
                                                cur_inst.in_net.push_back(final_inst_pin);
                                                int flag = instance_name_vec.size() - 1;
                                                net_instance_map[final_inst_pin].push_back(flag);
                                            }
                                            if(_l >= _r){
                                                _l--;
                                                if(_l<_r)
                                                    break;
                                            }
                                            else{
                                                _l++;
                                                if(_l>_r)
                                                    break;
                                            }
                                            if(type_pin_lindex > type_pin_rindex)
                                                l--;
                                            else
                                                l++;
                                        }
                                    }
                                    else if(_pin == "1\'b0" || _pin == "1\'b1")
                                    {
                                        string final_pin = cur_type_pin + "[" + to_string(l) + "]";
                                        string tmp_name;
                                        do{
                                            srand((int)time(0));
                                            tmp_name = "gv_net" + to_string(rand()%1000);
                                        }while(pins.find(tmp_name) != pins.end());
                                        if((cur_module.ports)[cur_type_pin].type == "output"){
                                            cur_inst.set_out(final_pin, tmp_name);
                                            cur_inst.out_net.push_back(tmp_name);
                                            int flag = instance_name_vec.size() - 1;
                                            out_net_from_id[tmp_name] = flag;
                                        }
                                        else{
                                            cur_inst.set_in(final_pin, tmp_name);
                                            cur_inst.in_net.push_back(tmp_name);
                                            int flag = instance_name_vec.size() - 1;
                                            net_instance_map[tmp_name].push_back(flag);
                                        }
                                        Pin tmp_pin;
                                        tmp_pin.type = "wire";
                                        tmp_pin.name = tmp_name;
                                        tmp_pin.size = 1;
                                        tmp_pin.lindex = -1;
                                        tmp_pin.rindex = -1;
                                        pins[tmp_name] = tmp_pin;
                                        //add pinbitValue
                                        TimedValues* tvs = new TimedValues();
                                        TimedValue t_val;
                                        t_val.t = 0;
                                        t_val.value = (cur_inst_pin == "1\'b1"?VCD_1:VCD_0);
                                        tvs->push_back(t_val);
                                        pinbitValues[tmp_name] = tvs;
                                    }
                                    else
                                    {
                                        assert(pins.find(_pin)!=pins.end());
                                        string final_pin = cur_type_pin + "[" + to_string(l) + "]";
                                        //cout << "\t" << final_pin << "=" << _pin <<endl;
                                        if((cur_module.ports)[cur_type_pin].type == "output"){
                                            cur_inst.set_out(final_pin, _pin);
                                            cur_inst.out_net.push_back(_pin);
                                            int flag = instance_name_vec.size() - 1;
                                            out_net_from_id[_pin] = flag;
                                        }
                                        else{
                                            cur_inst.set_in(final_pin, _pin);
                                            cur_inst.in_net.push_back(_pin);
                                            int flag = instance_name_vec.size() - 1;
                                            net_instance_map[_pin].push_back(flag);
                                        }
                                    }
                                    if(type_pin_lindex > type_pin_rindex)
                                        l--;
                                    else
                                        l++;
                                }
                            }
                        }
                    }
                    //check unknown pin --- must be output
                    if ((cur_module.in_out_ports).size() > instance_vec.size()-1)
                    {
                        assert(module_name_vec.size()>0);
                        for (unsigned i = 0; i < module_name_vec.size(); ++i)
                        {
                            cur_inst.set_unknown_out(module_name_vec[i]);
                        }                       
                    }
                    //function pin's replace
                    std::vector<Function> _function_vec = cur_module.function;
                    std::vector<Function> function_vec;
                    std::map<string, int> function_map = {
                        {"and", 0}, {"or", 1}, {"xor", 2},{"xnor", 3},{"nor",4},
                        {"nand",5},{"buf",6},{"not",7},{"udp_xbuf",8},{"udp_mux2",9}
                    };
                    int pin_id = cur_inst.in_net.size() + cur_inst.out_net.size() + cur_inst.unknown_out.size();
                    std::map<string, int> inter_wires;
                    for (std::vector<Function>::iterator i = _function_vec.begin(); i != _function_vec.end(); ++i)
                    {
                        std::vector<int> _temp;
                        _temp.push_back(function_map[(*i).name]);
                        for (unsigned j = 0; j < ((*i).input_pins).size(); ++j)
                        {
                            string cur_in = ((*i).input_pins)[j];
                            if (cur_inst.in_port_net.find(cur_in)!=cur_inst.in_port_net.end())
                            {
                                string cur_net = cur_inst.in_port_net[cur_in];
                                replace(((*i).input_pins).begin(), ((*i).input_pins).end(), cur_in, cur_net);
                                auto found = find((cur_inst.in_net).begin(), (cur_inst.in_net).end(), cur_net);
                                _temp.push_back(distance(cur_inst.in_net.begin(), found));
                            }
                            else if(inter_wires.find(cur_in)!=inter_wires.end())
                            {
                                _temp.push_back(inter_wires[cur_in]);
                            }
                            else if (find(cur_module.supply1_vec.begin(), cur_module.supply1_vec.end() ,cur_in)!=cur_module.supply1_vec.end())
                            {
                                _temp.push_back(17);
                            }
                            else if (find(cur_module.supply0_vec.begin(), cur_module.supply0_vec.end() ,cur_in)!=cur_module.supply0_vec.end())
                            {
                                _temp.push_back(16);
                            }
                            else
                            {
                                inter_wires.insert(make_pair(cur_in, pin_id));
                                _temp.push_back(pin_id);
                                pin_id++;
                            }
                        }
                        for (unsigned j = 0; j < ((*i).output_pins).size(); ++j)
                        {
                            string cur_out = ((*i).output_pins)[j];
                            if (cur_inst.out_port_net.find(cur_out)!=cur_inst.out_port_net.end())
                            {
                                string cur_net = cur_inst.out_port_net[cur_out];
                                replace(((*i).output_pins).begin(), ((*i).output_pins).end(), cur_out, cur_net);
                                auto found = find((cur_inst.out_net).begin(), (cur_inst.out_net).end(), cur_net);
                                _temp.push_back(distance(cur_inst.out_net.begin(), found)+cur_inst.in_net.size());
                            }
                            else if(inter_wires.find(cur_out)!=inter_wires.end())
                            {
                                _temp.push_back(inter_wires[cur_out]);
                            }
                            else if (find(cur_module.supply1_vec.begin(), cur_module.supply1_vec.end() ,cur_out)!=cur_module.supply1_vec.end())
                            {
                                _temp.push_back(17);
                            }
                            else if (find(cur_module.supply0_vec.begin(), cur_module.supply0_vec.end() ,cur_out)!=cur_module.supply0_vec.end())
                            {
                                _temp.push_back(16);
                            }
                            else
                            {
                                inter_wires.insert(make_pair(cur_out, pin_id));
                                _temp.push_back(pin_id);
                                pin_id++;
                            }
                        }
                        function_vec.push_back(*i);
                        cur_inst.function_id_vec.push_back(_temp);
                    }
                    cur_inst.set_function(function_vec);
                    //parse done
                    instances.insert(make_pair(instance_name, cur_inst));
                }
            }
        }
    }
    cout << "INFO: gate level file's parsing done!" << endl;
    inf.close();
}

void PreProcess::parse_sdf(std::string& sdf_path){
    double factor = 1e3;
    
    ifstream inf(sdf_path);
    if (!inf)
    {
        cout << "File " << sdf_path << " Open Error!" << endl;
        cout << "Exit!" << endl;
        exit(-1);
    }
    while(!inf.eof())
    {
        string line;
        getline(inf, line);
        line.erase(0, line.find_first_not_of(" "));
        line.erase(line.find_last_not_of(" ")+1);
        if (line == "")
        {
            continue;
        }
        std::vector<std::string> tmp;
        std::string delims = " \t\r\n\"\'()";
        tmp = Split(line, delims);
        if (tmp.size() == 0)
            continue;
        if (tmp[0] == "")
        {
            cout << "ERROR: string " << line << endl;
            exit(-1);
        }
        transform(tmp[0].begin(), tmp[0].end(), tmp[0].begin(), ::toupper);
        //parse TIMESCALE
        if (tmp[0] == "TIMESCALE")
        {
            if (tmp[1] == "1ns")
                factor = 1e3;
            else if (tmp[1] == "1ps")
                factor = 1.0;
            else if (tmp[1] == "1us")
                factor = 1e6;
            else if (tmp[1] == "1ms")
                factor = 1e9;
            else if (tmp[1] == "1s")
                factor = 1e12;
            else
                cout << "WARNING: Timescale " << tmp[1] << " is not supported, use 1ps" <<endl;
        }
        //add delay info
        else if (tmp[0] == "INSTANCE")
        {
            if (tmp.size() > 1 && instances.find(tmp[1])!= instances.end())
            {
                string inst_name = tmp[1];
                //cout << "Instance name: " << inst_name << endl;
                while(getline(inf, line))
                {
                    line.erase(0, line.find_first_not_of(" "));
                    line.erase(line.find_last_not_of(" ")+1);
                    if (line == "")
                    {
                        continue;
                    }
                    if (line == ")")
                    {
                        break;
                    }
                    delims = " \t\r\n\"\'()";
                    tmp = Split(line, delims);
                    if (tmp.size() == 0)
                        continue;
                    if (tmp[0] == "")
                    {
                        cout << "ERROR: string " << line << endl;
                        exit(-1);
                    }
                    transform(tmp[0].begin(), tmp[0].end(), tmp[0].begin(), ::toupper);
                    // only support ABSOLUTE-IOPATH, and only consider 2 kinds of delayValue: rise and fall
                    if (tmp[0] == "IOPATH")
                    {
                        assert(tmp.size() >= 4);
                        string tt = tmp[1];
                        transform(tmp[1].begin(), tmp[1].end(), tmp[1].begin(), ::toupper);
                        if (tmp[1] == "POSEDGE" || tmp[1] == "NEGEDGE")
                        {
                            assert(tmp.size() >= 5);
                            Delay delay;
                            delay.edge = tmp[1] == "POSEDGE"?0:1;
                            //cout << "\tedge: " << delay.edge;
                            delay.in_bit = (((instances.find(inst_name))->second).in_port_net)[tmp[2]];
                            //cout << "\tinbit: " << delay.in_bit;
                            if ((((instances.find(inst_name))->second).out_port_net).find(tmp[3])!=(((instances.find(inst_name))->second).out_port_net).end())
                            {
                                delay.out_bit = (((instances.find(inst_name))->second).out_port_net)[tmp[3]];
                                //cout << "\toutbit: " << delay.out_bit;
                            }
                            else if (find(((instances.find(inst_name))->second).unknown_out.begin(), ((instances.find(inst_name))->second).unknown_out.end(), tmp[3])!=((instances.find(inst_name))->second).unknown_out.end())
                            {
                                std::vector<std::string>::iterator it = find(((instances.find(inst_name))->second).unknown_out.begin(), ((instances.find(inst_name))->second).unknown_out.end(), tmp[3]);
                                delay.out_bit = *it;
                                //cout << "\toutbit: " << delay.out_bit;
                            }
                            else{
                                cout << "ERROR:"<< tmp[1] <<"---Instance(" << inst_name << ")\'s output pin " << tmp[3] << " is not found." << endl;
                                exit(-1);
                            }
                            std::vector<string> value_vec = Split(tmp[4], ":");
                            assert(value_vec.size() == 3);
                            stringstream num_str;
                            double rise_val, fall_val;
                            num_str << value_vec[1];
                            num_str >> rise_val;
                            rise_val = rise_val*factor;
                            if (tmp.size() == 5)
                            {
                                fall_val = rise_val;
                            }
                            else
                            {
                                value_vec = Split(tmp[5], ":");
                                assert(value_vec.size() == 3);
                                stringstream num_str_fall;
                                num_str_fall << value_vec[1];
                                num_str_fall >> fall_val;
                                fall_val = fall_val*factor;
                            }
                            delay.rise_val = rise_val;
                            delay.fall_val = fall_val;
                            ((instances.find(inst_name))->second).set_delay(delay);
                        }
                        // edge = 2
                        else
                        {
                            assert(tmp.size() >= 4);
                            //transform(tmp[1].begin(), tmp[1].end(), tmp[1].begin(), ::tolower);
                            tmp[1] = tt;
                            Delay delay;
                            delay.edge = 2;
                            //cout << "\tedge: " << delay.edge;
                            delay.in_bit = (((instances.find(inst_name))->second).in_port_net)[tmp[1]];
                            //cout << "\tinbit: " << delay.in_bit;
                            if ((((instances.find(inst_name))->second).out_port_net).find(tmp[2])!=(((instances.find(inst_name))->second).out_port_net).end())
                            {
                                delay.out_bit = (((instances.find(inst_name))->second).out_port_net)[tmp[2]];
                                //cout << "\toutbit: " << delay.out_bit;
                            }
                            else if (find(((instances.find(inst_name))->second).unknown_out.begin(), ((instances.find(inst_name))->second).unknown_out.end(), tmp[2])!=((instances.find(inst_name))->second).unknown_out.end())
                            {
                                std::vector<std::string>::iterator it = find(((instances.find(inst_name))->second).unknown_out.begin(), ((instances.find(inst_name))->second).unknown_out.end(), tmp[2]);
                                delay.out_bit = *it;
                                //cout << "\toutbit: " << delay.out_bit;
                            }
                            else{
                                cout << "ERROR: Both---Instance(" << inst_name << ")\'s output pin " << tmp[2] << " is not found." << endl;
                                exit(-1);
                            }
                            std::vector<string> value_vec = Split(tmp[3], ":");
                            assert(value_vec.size() == 3);
                            stringstream num_str;
                            double rise_val, fall_val;
                            num_str << value_vec[1];
                            num_str >> rise_val;
                            rise_val = rise_val*factor;
                            if (tmp.size() == 4)
                            {
                                fall_val = rise_val;
                            }
                            else
                            {
                                value_vec = Split(tmp[4], ":");
                                assert(value_vec.size() == 3);
                                stringstream num_str_fall;
                                num_str_fall << value_vec[1];
                                num_str_fall >> fall_val;
                                fall_val = fall_val*factor;
                            }
                            delay.rise_val = rise_val;
                            delay.fall_val = fall_val;
                            ((instances.find(inst_name))->second).set_delay(delay);
                        }
                    }

                }
                //cout << endl;
            }
        }
    }
    cout << "INFO: SDF file's parsing done!" << endl;
    inf.close();
}

std::vector<string> PreProcess::Split(const string &str, const string &delim)
{
    vector<string> res;
    if(str == "")
        return res;
    char *strs = new char[str.length() + 1];
    strcpy(strs, str.c_str());
    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());
 
    char *p = strtok(strs, d);
    while(p)
    {
        string s = p;
        res.push_back(s);
        p = strtok(NULL, d);
    }
    return res;
}

