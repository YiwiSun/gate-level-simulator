#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "simulate.h"
#include <algorithm>
#include <limits>
#include <cstddef>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
//#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/access.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>


class inter
{
public:
    inter(){}
    ~inter(){}

    /* data */
    PreProcess p;
    std::vector<std::vector<int> > levels;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) //
    {            //ar.register_type(static_cast<ModuleType *>(NULL));
        ar & p;
        ar & levels;
    }
};
using namespace std;

void DumpSaif(std::unordered_map<std::string, TimedValues*> val_map, 
    std::unordered_map<std::string, TimedValues*> out_value, std::map<std::string, TimedValues> pinbitValues, 
    std::string saif_out, VCDTimeUnit time_unit, unsigned time_res, std::string root_name, VCDTime start, VCDTime end)
{
    if (end == 0)
    {
        for(auto it:out_value)
        {
            TimedValues* tvs = it.second;
            TimedValue tv = tvs->back();
            VCDTime cur_t = tv.t;
            if (cur_t>end)
            {
                end = cur_t;
            }
        }
        for(auto it:pinbitValues)
        {
            TimedValues tvs = it.second;
            TimedValue tv = tvs.back();
            VCDTime cur_t = tv.t;
            if (cur_t>end)
            {
                end = cur_t;
            }
        }
        end += 1;
    }
    unsigned long long total_time = end - start;
    /**/
    string tmp_tu;
    if(time_unit == TIME_S)
        tmp_tu = "s";
    else if(time_unit == TIME_MS)
        tmp_tu = "ms";
    else if(time_unit == TIME_US)
        tmp_tu = "us";
    else if(time_unit == TIME_NS)
        tmp_tu = "ns";
    else if(time_unit == TIME_PS)
        tmp_tu = "ps";
    ofstream outf(saif_out);
    if (!outf)
    {
        cout << "File " << saif_out << " Open Error!" << endl;
        cout << "Exit!" << endl;
        exit(-1);
    }
    outf << "(TIMESCALE " << time_res << " " << tmp_tu << ")" << endl;
    outf << "(DURATION " << total_time << ")" << endl;
    outf << "(INSTANCE " << root_name << endl;
    outf << "   (NET" << endl;
    std::map<std::string, TimedValues*> _out_value;
    for(auto it = out_value.begin(); it != out_value.end(); it++)
    {
        string net_name = it->first;
        outf << "      (" << net_name << endl;
        if (net_name.find("[") != std::string::npos)
        {
            string _net_name = net_name.substr(net_name.find("[")+1, net_name.find("]")-net_name.find("[")-1);
            if(!(_net_name[0] >= '0' && _net_name[0] <= '9')){
                _out_value[net_name] = it->second;
                continue;
            }
        }
        unsigned long long time_1=0;
        unsigned long long time_0=0;
        unsigned long long time_x=0;
        unsigned long long time_z=0;
        unsigned long long last_time = start;
        VCDBit last_state = VCD_X;
        TimedValues* tvs = it->second;
        while(!tvs->empty())
        {
            TimedValue tv = tvs->front();
            tvs->pop_front();
            VCDTime cur_t = tv.t;
            if (cur_t <= start)
            {
                last_state = tv.value;
                continue;
            }
            if (cur_t > end)
            {
                break;
            }
            VCDBit cur_val = tv.value;
            if (last_state == VCD_X)
            {
                time_x += cur_t - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += cur_t - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += cur_t - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += cur_t - last_time;
            }
            last_time = cur_t;
            last_state = cur_val;
        }
        if (last_time < end)
        {
            if (last_state == VCD_X)
            {
                time_x += end - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += end - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += end - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += end - last_time;
            }
        }
        outf << "         (T0 " << time_0 << ") (T1 " << time_1 << ") (TX " << time_x+time_z << ")" << endl;
        outf << "      )" << endl;
    }
    for(auto it:pinbitValues)
    {
        string net_name = it.first;
        if (net_name.substr(0,6) == "gv_net")
        {
            continue;
        }
        outf << "      (" << net_name << endl;
        unsigned long long time_1=0;
        unsigned long long time_0=0;
        unsigned long long time_x=0;
        unsigned long long time_z=0;
        unsigned long long last_time = start;
        VCDBit last_state = VCD_X;
        TimedValues tvs = it.second;
        while(!tvs.empty())
        {
            TimedValue tv = tvs.front();
            tvs.pop_front();
            VCDTime cur_t = tv.t;
            if (cur_t <= start)
            {
                last_state = tv.value;
                continue;
            }
            if (cur_t > end)
            {
                break;
            }
            VCDBit cur_val = tv.value;
            if (last_state == VCD_X)
            {
                time_x += cur_t - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += cur_t - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += cur_t - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += cur_t - last_time;
            }
            last_time = cur_t;
            last_state = cur_val;
        }
        if (last_time < end)
        {
            if (last_state == VCD_X)
            {
                time_x += end - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += end - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += end - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += end - last_time;
            }
        }
        outf << "         (T0 " << time_0 << ") (T1 " << time_1 << ") (TX " << time_x+time_z << ")" << endl;
        outf << "      )" << endl;
    }
    for(auto it:val_map)
    {
        string net_name = it.first;
        outf << "      (" << net_name << endl;
        unsigned long long time_1=0;
        unsigned long long time_0=0;
        unsigned long long time_x=0;
        unsigned long long time_z=0;
        unsigned long long last_time = start;
        VCDBit last_state = VCD_X;
        TimedValues* tvs = it.second;
        while(!tvs->empty())
        {
            TimedValue tv = tvs->front();
            tvs->pop_front();
            VCDTime cur_t = tv.t;
            if (cur_t <= start)
            {
                last_state = tv.value;
                continue;
            }
            if (cur_t > end)
            {
                break;
            }
            VCDBit cur_val = tv.value;
            if (last_state == VCD_X)
            {
                time_x += cur_t - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += cur_t - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += cur_t - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += cur_t - last_time;
            }
            last_time = cur_t;
            last_state = cur_val;
        }
        if (last_time < end)
        {
            if (last_state == VCD_X)
            {
                time_x += end - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += end - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += end - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += end - last_time;
            }
        }
        outf << "         (T0 " << time_0 << ") (T1 " << time_1 << ") (TX " << time_x+time_z << ")" << endl;
        outf << "      )" << endl;
    }
    outf << "   )" << endl;
    outf << endl;
    for(auto it = _out_value.begin(); it != _out_value.end(); it++)
    {
        string net_name = it->first;
        string instance_name = net_name.substr(0, net_name.find("["));
        net_name = net_name.substr(net_name.find("[")+1, net_name.find("]")-net_name.find("[")-1);
        outf << "   (INSTANCE " << instance_name << endl;
        outf << "      (NET" << endl;
        outf << "         (" << net_name << endl;
        unsigned long long time_1=0;
        unsigned long long time_0=0;
        unsigned long long time_x=0;
        unsigned long long time_z=0;
        unsigned long long last_time = start;
        VCDBit last_state = VCD_X;
        TimedValues* tvs = it->second;
        while(!tvs->empty())
        {
            TimedValue tv = tvs->front();
            tvs->pop_front();
            VCDTime cur_t = tv.t;
            if (cur_t <= start)
            {
                last_state = tv.value;
                continue;
            }
            if (cur_t > end)
            {
                break;
            }
            VCDBit cur_val = tv.value;
            if (last_state == VCD_X)
            {
                time_x += cur_t - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += cur_t - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += cur_t - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += cur_t - last_time;
            }
            last_time = cur_t;
            last_state = cur_val;
        }
        if (last_time < end)
        {
            if (last_state == VCD_X)
            {
                time_x += end - last_time;
            }
            else if (last_state == VCD_0)
            {
                time_0 += end - last_time;
            }
            else if (last_state == VCD_1)
            {
                time_1 += end - last_time;
            }
            else if (last_state == VCD_Z)
            {
                time_z += end - last_time;
            }
        }
        outf << "            (T0 " << time_0 << ") (T1 " << time_1 << ") (TX " << time_x+time_z << ")" << endl;
        outf << "         )" << endl;
        outf << "      )" << endl;
        outf << "   )" << endl;
    }
    outf << ")" << endl;
    outf.close();
}

void calc(std::string name, std::vector<VCDBit> input_data, VCDBit &output_data)
{
    if (name == "and")
    {
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = input_data[0] & input_data[1];
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[0] == VCD_Z)
            {
                if (input_data[1] == VCD_0)
                {
                    tmp = VCD_0;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else if (input_data[1] == VCD_X || input_data[1] == VCD_Z)
            {
                if (input_data[0] == VCD_0)
                {
                    tmp = VCD_0;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else{
                std::cout << "ERROR: input_data" << input_data[0] << " or " << input_data[1] << std::endl;
                exit(-1);
            }
        }
        if (input_data.size() == 2)
        {
            output_data = VCDBit(tmp);
            return;
        }
        for (unsigned i = 2; i < input_data.size(); ++i)
        {
            switch(tmp)
            {
                case(0):
                    tmp = VCD_0;
                    break;
                case(1):
                    if (input_data[i] == VCD_Z)
                    {
                        tmp = VCD_X;
                    }
                    else{
                        tmp = input_data[i];
                    }
                    break;
                //X
                case(2):
                //Z
                case(3):
                    if (input_data[i] == VCD_0)
                    {
                        tmp = VCD_0;
                    }
                    else{
                        tmp = VCD_X;
                    }
                    break;
                default:
                    tmp =  VCD_X;
                    break;
            }
        }
        output_data = VCDBit(tmp);
    }
    else if (name == "or")
    {
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = input_data[0] | input_data[1];
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[0] == VCD_Z)
            {
                if (input_data[1] == VCD_1)
                {
                    tmp = VCD_1;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else if (input_data[1] == VCD_X || input_data[1] == VCD_Z)
            {
                if (input_data[0] == VCD_1)
                {
                    tmp = VCD_1;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else{
                std::cout << "ERROR: input_data" << input_data[0] << " or " << input_data[1] << std::endl;
                exit(-1);
            }
        }
        if (input_data.size() == 2)
        {
            output_data = VCDBit(tmp);
            return;
        }
        for (unsigned i = 2; i < input_data.size(); ++i)
        {
            switch(tmp)
            {
                case(0):
                    if (input_data[i] == VCD_Z)
                    {
                        tmp = VCD_X;
                    }
                    else{
                        tmp = input_data[i];
                    }
                    break;
                case(1):
                    tmp = VCD_1;
                    break;
                //X
                case(2):
                //Z
                case(3):
                    if (input_data[i] == VCD_1)
                    {
                        tmp = VCD_1;
                    }
                    else{
                        tmp = VCD_X;
                    }
                    break;
                default:
                    tmp = VCD_X;
                    break;
            }
        }
        output_data = VCDBit(tmp);
    }
    else if (name == "xor")
    {
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = input_data[0] ^ input_data[1];
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[1] == VCD_X)
            {
                tmp = VCD_X;
            }
            else if (input_data[0] == VCD_Z)
            {
                tmp = (input_data[1] == VCD_Z)?VCD_0:VCD_X;
            }
            else if (input_data[1] == VCD_Z)
            {
                tmp = (input_data[0] == VCD_Z)?VCD_0:VCD_X;
            }
            else{
                std::cout << "ERROR: input_data" << input_data[0] << " or " << input_data[1] << std::endl;
                exit(-1);
            }
        }
        if (input_data.size() == 2)
        {
            output_data = VCDBit(tmp);
            return;
        }
        for (unsigned i = 2; i < input_data.size(); ++i)
        {
            switch(tmp)
            {
                case(0):
                    if (input_data[i] == VCD_Z)
                    {
                        tmp = VCD_X;
                    }
                    else{
                        tmp = input_data[i];
                    }
                    break;
                case(1):
                    if (input_data[i] == VCD_0 || input_data[i] == VCD_1)
                    {
                        tmp = tmp ^ input_data[i];
                    }
                    else
                    {
                        tmp = VCD_X;
                    }
                    break;
                //X
                case(2):
                    tmp = VCD_X;
                    break;
                //Z
                case(3):
                    tmp = input_data[i] == VCD_Z ? VCD_0:VCD_X;
                    break;
                default:
                    tmp = VCD_X;
                    break;
            }
        }
        output_data = VCDBit(tmp);
    }
    else if (name == "xnor")
    {
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = input_data[0] ^ input_data[1];
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[1] == VCD_X)
            {
                tmp = VCD_X;
            }
            else if (input_data[0] == VCD_Z)
            {
                tmp = (input_data[1] == VCD_Z)?VCD_0:VCD_X;
            }
            else if (input_data[1] == VCD_Z)
            {
                tmp = (input_data[0] == VCD_Z)?VCD_0:VCD_X;
            }
            else{
                std::cout << "ERROR: input_data" << input_data[0] << " or " << input_data[1] << std::endl;
                exit(-1);
            }
        }
        if (input_data.size() == 2)
        {
            tmp = (tmp == 0 || tmp == 1)?(1^tmp):VCD_X;
            output_data = VCDBit(tmp);
            return;
        }
        for (unsigned i = 2; i < input_data.size(); ++i)
        {
            switch(tmp)
            {
                case(0):
                    if (input_data[i] == VCD_Z)
                    {
                        tmp = VCD_X;
                    }
                    else{
                        tmp = input_data[i];
                    }
                    break;
                case(1):
                    if (input_data[i] == VCD_0 || input_data[i] == VCD_1)
                    {
                        tmp = tmp ^ input_data[i];
                    }
                    else
                    {
                        tmp = VCD_X;
                    }
                    break;
                //X
                case(2):
                    tmp = VCD_X;
                    break;
                //Z
                case(3):
                    tmp = input_data[i] == VCD_Z ? VCD_0:VCD_X;
                    break;
                default:
                    tmp = VCD_X;
                    break;
            }
        }
        tmp = (tmp == 0 || tmp == 1)?(1^tmp):VCD_X;
        output_data = VCDBit(tmp);
    }
    else if (name == "nor")
    {
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = input_data[0] | input_data[1];
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[0] == VCD_Z)
            {
                if (input_data[1] == VCD_1)
                {
                    tmp = VCD_1;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else if (input_data[1] == VCD_X || input_data[1] == VCD_Z)
            {
                if (input_data[0] == VCD_1)
                {
                    tmp = VCD_1;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else{
                std::cout << "ERROR: input_data" << input_data[0] << " or " << input_data[1] << std::endl;
                exit(-1);
            }
        }
        if (input_data.size() == 2)
        {
            tmp = (tmp == 0 || tmp == 1)?(1^tmp):VCD_X;
            output_data = VCDBit(tmp);
            return;
        }
        for (unsigned i = 2; i < input_data.size(); ++i)
        {
            switch(tmp)
            {
                case(0):
                    if (input_data[i] == VCD_Z)
                    {
                        tmp = VCD_X;
                    }
                    else{
                        tmp = input_data[i];
                    }
                    break;
                case(1):
                    tmp = VCD_1;
                    break;
                //X
                case(2):
                //Z
                case(3):
                    if (input_data[i] == VCD_1)
                    {
                        tmp = VCD_1;
                    }
                    else{
                        tmp = VCD_X;
                    }
                    break;
                default:
                    tmp = VCD_X;
                    break;
            }
        }
        tmp = (tmp == 0 || tmp == 1)?(1^tmp):VCD_X;
        output_data = VCDBit(tmp);
    }
    else if (name == "nand")
    {
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = input_data[0] & input_data[1];
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[0] == VCD_Z)
            {
                if (input_data[1] == VCD_0)
                {
                    tmp = VCD_0;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else if (input_data[1] == VCD_X || input_data[1] == VCD_Z)
            {
                if (input_data[0] == VCD_0)
                {
                    tmp = VCD_0;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else{
                std::cout << "ERROR: input_data" << input_data[0] << " or " << input_data[1] << std::endl;
                exit(-1);
            }
        }
        if (input_data.size() == 2)
        {
            tmp = (tmp == 0 || tmp == 1)?(1^tmp):VCD_X;
            output_data = VCDBit(tmp);
            return;
        }
        for (unsigned i = 2; i < input_data.size(); ++i)
        {
            switch(tmp)
            {
                case(0):
                    tmp = VCD_0;
                    break;
                case(1):
                    if (input_data[i] == VCD_Z)
                    {
                        tmp = VCD_X;
                    }
                    else{
                        tmp = input_data[i];
                    }
                    break;
                //X
                case(2):
                //Z
                case(3):
                    if (input_data[i] == VCD_0)
                    {
                        tmp = VCD_0;
                    }
                    else{
                        tmp = VCD_X;
                    }
                    break;
                default:
                    tmp =  VCD_X;
                    break;
            }
        }
        tmp = (tmp == 0 || tmp == 1)?(1^tmp):VCD_X;
        output_data = VCDBit(tmp);
    }
    else if (name == "buf")
    {
        output_data = VCDBit(input_data[0] == VCD_Z ? VCD_X:input_data[0]);
    }
    else if (name == "not")
    {
        int tmp;
        tmp = (input_data[0] == VCD_0 || input_data[0] == VCD_1)?(1^input_data[0]):VCD_X;
        output_data = VCDBit(tmp);
    }
    else if (name == "udp_xbuf")
    {
        assert(input_data.size() == 2);
        assert(input_data[1] == VCD_1);
        assert(input_data[0] != VCD_Z);
        if (input_data[1] == VCD_1)
        {
            output_data = VCDBit(input_data[0] == VCD_X ? VCD_1:input_data[0]);
        }
        else
        {
            cout << "WARNING: udp_xbuf's input_data[1] is not 1." << endl;
            output_data = VCD_X;
        }
    }
    else if (name == "udp_mux2")
    {
        assert(input_data.size() == 3);
        assert(input_data[0] != VCD_Z && input_data[1] != VCD_Z && input_data[2] != VCD_Z);
        if (input_data[2] == VCD_0)
        {
            output_data = input_data[0];
        }
        else if (input_data[2] == VCD_1)
        {
            output_data = input_data[1];
        }
        else if (input_data[2] == VCD_X)
        {
            output_data = (input_data[0] == input_data[1]?input_data[0]:VCD_X);
        }
        else
        {
            output_data = VCD_X;
        }
    }
}

void SimulateInst(Instance inst, std::map<std::string, TimedValues*> bitValues, 
    std::unordered_map<std::string, TimedValues*>& _out_value, VCDTimeUnit time_unit)
{
    std::vector<std::string> outputs = inst.out_net;
    std::vector<std::string> inputs = inst.in_net;
    std::vector<std::string> supply1_vec = (inst.type).supply1_vec;
    std::vector<std::string> supply0_vec = (inst.type).supply0_vec;
    for(unsigned i = 0; i < outputs.size(); ++i)
    {
        string _out = outputs[i];
        _out_value[_out] = new TimedValues();
        /** initialize **/
        TimedValue tv;
        tv.t = 0;
        tv.value = VCD_X;
        _out_value[_out]->push_back(tv);
        /** **/
        std::vector<Function> functions = inst.get_function();
        std::vector<Delay> delays = inst.get_delay();
        //std::map<std::string, TimedValues*> inValues = bitValues;
        std::deque<TimedValueSim> inTimedValues;
        Process(bitValues, inTimedValues);   //for simulate
	    //cout << "\t\trecorded time num: " << inTimedValues.size() << endl;
        Calculate(inTimedValues, inputs, functions, delays, _out, _out, _out_value, bitValues, time_unit, supply1_vec, supply0_vec);
	    //cout << "\t\toutput: " << _out << " done." <<endl;
    }
    for (unsigned i = 0; i < inst.unknown_out.size(); ++i)
    {
        string _out = (inst.unknown_out)[i];
        string _out_name = inst.name + "[" + (inst.unknown_out)[i] +"]";
        _out_value[_out_name] = new TimedValues();
        /** initialize **/
        TimedValue tv;
        tv.t = 0;
        tv.value = VCD_X;
        _out_value[_out_name]->push_back(tv);
        /** **/
        std::vector<Function> functions = inst.get_function();
        std::vector<Delay> delays = inst.get_delay();
        //std::map<std::string, TimedValues*> inValues = bitValues;
        std::deque<TimedValueSim> inTimedValues;
        Process(bitValues, inTimedValues);   //for simulate
	    //cout << "\t\trecorded time num: " << inTimedValues.size() << endl;
        Calculate(inTimedValues, inputs, functions, delays, _out, _out_name, _out_value, bitValues, time_unit, supply1_vec, supply0_vec);
	    //cout << "\t\toutput: " << _out << " done." <<endl;
    }
}
void Calculate(std::deque<TimedValueSim> inTimedValues, std::vector<std::string> inputs, std::vector<Function> functions, 
    std::vector<Delay> delays, const std::string _out, const std::string _out_name, std::unordered_map<std::string, TimedValues*>& _out_value,
    std::map<std::string, TimedValues*> bitValues, VCDTimeUnit time_unit, std::vector<std::string> supply1_vec, 
    std::vector<std::string> supply0_vec)
{
    /*if(_out == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_axi4buf_ICCADs_Queue_2_ICCADs_n6")
    {
        for (unsigned j = 0; j < inputs.size(); ++j)
        {
            cout << inputs[j] << "\t";
        }
        cout <<endl;
    }*/

    VCDBit pre_out = VCD_X;
    while(!inTimedValues.empty())
    {
	    TimedValueSim temp = inTimedValues.front();
	    inTimedValues.pop_front();
        VCDTime cur_time = temp.time;
        /*if(_out == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_axi4buf_ICCADs_Queue_2_ICCADs_n6")
        {
            cout << "t=" << cur_time << ":" <<"\t";
        }*/
        std::map<std::string, VCDBit> inValues = temp.values;
        /*cout << "\tchanged net: ";
        for(auto tt:inValues)
        {
            cout << tt.first << " ";
        }
        cout << endl;*/
        //find related delay
        std::vector<Delay> related_delays;
        for (unsigned j = 0; j < delays.size(); ++j)
        {
            if(delays[j].out_bit == _out && inValues.find(delays[j].in_bit) != inValues.end())
            {
                related_delays.push_back(delays[j]);
            }
        }
        // find current and last value of every input
        std::map<std::string, VCDBit> cur_inValues;
        std::map<std::string, VCDBit> last_inValues;
        for (unsigned j = 0; j < inputs.size(); ++j)
        {
            VCDBit cur, last;
            GetValueAt(bitValues, inputs[j], cur_time, cur, last);
            cur_inValues[inputs[j]] = cur;
            last_inValues[inputs[j]] = last;
            //if(_out == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_axi4buf_ICCADs_Queue_2_ICCADs_n6")
            //    cout << cur << "\t";
        }
        //if(_out == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_axi4buf_ICCADs_Queue_2_ICCADs_n6")
        //    cout << "out: ";

        //run functions
        VCDBit cur_out;
        RunFunction(_out, functions, cur_inValues, cur_out, supply1_vec, supply0_vec);//, last_inValues, pre_out);
        //cout << "\tcur out:" << cur_out << "(" << _out << ")" << endl;
        //delay
        int out_sense = CheckDelay(pre_out, cur_out);
        if (out_sense == 0)
        {
            continue;
        }
        VCDTime out_time;
        AddDelay(related_delays, inValues.size(), out_sense, cur_time, cur_inValues, last_inValues, out_time, time_unit);
        /*if(_out == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_axi4buf_ICCADs_Queue_2_ICCADs_n6")
        {
            cout << cur_out << "(t=" << out_time << ")" << endl;
        }*/
        // update outValue
        TimedValue last_tv = _out_value[_out_name]->back();
        while (last_tv.t >= out_time)
        {
            if(_out_value[_out_name]->size() <= 1)
            {
                _out_value[_out_name]->pop_back();
                break;
            }
            _out_value[_out_name]->pop_back();
            last_tv = _out_value[_out_name]->back();
        }
        if (last_tv.value != cur_out)
        {
            TimedValue cur_tv;
            cur_tv.t = out_time;
            cur_tv.value = cur_out;
            _out_value[_out_name]->push_back(cur_tv);
        }
        pre_out = (_out_value[_out_name]->back()).value;
    }
}
void AddDelay(std::vector<Delay> related_delays, unsigned num_in, int out_sense, VCDTime cur_time, 
    std::map<std::string, VCDBit> cur_inValues, std::map<std::string, VCDBit> last_inValues, VCDTime &out_time, VCDTimeUnit time_unit)
{
    double delay_val = 1.0;
    double factor = 1.0;
    if (related_delays.size() == 0)
    {
        delay_val = 1.0;  //default
    }
    //only one input driving out
    else if (num_in == 1)
    {
        assert(related_delays.size() <= 2);
        if (related_delays.size() == 1)
        {
            Delay delay = related_delays[0];
            assert(delay.edge == 2);
            switch(out_sense)
            {
                case(1):
                    delay_val = delay.rise_val;
                    break;
                case(2):
                    delay_val = delay.fall_val;
                    break;
                case(3):
                    delay_val = max(delay.rise_val, delay.fall_val);
                    break;
                case(4):
                    delay_val = min(delay.rise_val, delay.fall_val);
                    break;
                default:
                    delay_val = 1.0;
                    break;
            }
        }
        else
        {
            Delay delay1 = related_delays[0];
            Delay delay2 = related_delays[1];
            assert(delay1.in_bit == delay2.in_bit);
            string name = delay1.in_bit;
            VCDBit cur_in = cur_inValues[name];
            VCDBit last_in = last_inValues[name];
            int in_sense = CheckDelay(last_in, cur_in);
            assert(in_sense != 0);
            int edge;
            if(in_sense == 1)
                edge = 0;
            else if(in_sense == 2)
                edge = 1;
            else{
                cout << "WARNING: input is X or Z, regarded as posedge" << endl;
                edge = 0;
            }
            for (unsigned i = 0; i < related_delays.size(); ++i)
            {
                Delay tmp = related_delays[i];
                if(tmp.edge == edge)
                {
                    switch(out_sense)
                    {
                        case(1):
                            delay_val = tmp.rise_val;
                            break;
                        case(2):
                            delay_val = tmp.fall_val;
                            break;
                        case(3):
                            delay_val = max(tmp.rise_val, tmp.fall_val);
                            break;
                        case(4):
                            delay_val = min(tmp.rise_val, tmp.fall_val);
                            break;
                        default:
                            delay_val = 1.0;
                            break;
                    }
                    break;
                }
            }
        }
    }
    // multiple inputs driving out
    else
    {
        double min_val = std::numeric_limits<double>::max();
        for (unsigned i = 0; i < related_delays.size(); ++i)
        {
            Delay delay = related_delays[i];
            // both pos and neg
            if (delay.edge == 2)
            {
                switch(out_sense)
                {
                    case(1):
                        min_val = min(delay.rise_val, min_val);
                        break;
                    case(2):
                        min_val = min(delay.fall_val, min_val);
                        break;
                    case(3):
                        min_val = min(max(delay.rise_val, delay.fall_val), min_val);
                        break;
                    case(4):
                        min_val = min(min(delay.rise_val, delay.fall_val), min_val);
                        break;
                    default:
                        min_val = 1.0;
                        break;
                }
            }
            else
            {
                string name = delay.in_bit;
                VCDBit cur_in = cur_inValues[name];
                VCDBit last_in = last_inValues[name];
                int in_sense = CheckDelay(last_in, cur_in);
                assert(in_sense != 0);
                int edge;
                if(in_sense == 1)
                    edge = 0;
                else if(in_sense == 2)
                    edge = 1;
                else{
                    cout << "WARNING: input is X or Z, regarded as posedge" << endl;
                    edge = 0;
                }
                if(edge == delay.edge)
                {
                    switch(out_sense)
                    {
                        case(1):
                            min_val = min(delay.rise_val, min_val);
                            break;
                        case(2):
                            min_val = min(delay.fall_val, min_val);
                            break;
                        case(3):
                            min_val = min(max(delay.rise_val, delay.fall_val), min_val);
                            break;
                        case(4):
                            min_val = min(min(delay.rise_val, delay.fall_val), min_val);
                            break;
                        default:
                            min_val = 1.0;
                            break;
                    }
                }
            }
        }
        delay_val = (min_val == std::numeric_limits<double>::max())?1.0:min_val;
    }
    if(time_unit == TIME_S)
        factor = 1e-12;
    else if(time_unit == TIME_MS)
        factor = 1e-9;
    else if(time_unit == TIME_US)
        factor = 1e-6;
    else if(time_unit == TIME_NS)
        factor = 1e-3;
    else if(time_unit == TIME_PS)
        factor = 1.0;
    out_time = cur_time + static_cast<unsigned long long>(delay_val*factor);
}

int CheckDelay(VCDBit pre, VCDBit cur)
{
    /** delay value --- 1:rise 2:fall 3:max(rise, fall) 4:min(rise, fall) 0:unchanged **/
    if ((pre == VCD_0 && cur != VCD_0) || (cur == VCD_1 && pre != VCD_1))
    {
        return 1;
    }
    else if ((pre == VCD_1 && cur != VCD_1) || (cur == VCD_0 && pre != VCD_0))
    {
        return 2;
    }
    else if (pre == VCD_X && cur == VCD_Z)
    {
        return 3;
    }
    else if (pre == VCD_Z && cur == VCD_X)
    {
        return 4;
    }
    else
    {
        return 0;
    }   
}

void RunFunction(const std::string _out, std::vector<Function> functions, std::map<std::string, VCDBit> cur_inValues, VCDBit &cur_out, 
    std::vector<std::string> supply1_vec, std::vector<std::string> supply0_vec)
{
    std::map<std::string, VCDBit> inter_values;
    //std::map<std::string, VCDBit> pre_inter_values;
    for (unsigned i = 0; i < functions.size(); ++i)
    {
        /** current **/
        Function cur_func = functions[i];
        std::vector<string> in_ports = cur_func.input_pins;
        for (unsigned j = 0; j < in_ports.size(); ++j)
        {
            if (cur_inValues.find(in_ports[j]) != cur_inValues.end())
            {
                cur_func.set_in_data(cur_inValues[in_ports[j]]);
            }
            else if (inter_values.find(in_ports[j]) != inter_values.end())
            {
                cur_func.set_in_data(inter_values[in_ports[j]]);
            }
            else if (find(supply1_vec.begin(), supply1_vec.end(), in_ports[j]) != supply1_vec.end())
            {
                cur_func.set_in_data(VCD_1);
            }
            else if (find(supply0_vec.begin(), supply0_vec.end(), in_ports[j]) != supply0_vec.end())
            {
                cur_func.set_in_data(VCD_0);
            }
            else
            {
                cout << "WARNING: unknown function ports " << in_ports[j] << endl;
                cur_func.set_in_data(VCD_X);
            }
        }
        if(cur_func.name == "and")
        {
            //And* _func = (And*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "or")
        {
            //Or* _func = (Or*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "xor")
        {
            //Xor* _func = (Xor*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "xnor")
        {
            //Xnor* _func = (Xnor*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "nor")
        {
            //Nor* _func = (Nor*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "nand")
        {
            //Nand* _func = (Nand*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "buf")
        {
            //Buf* _func = (Buf*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "not")
        {
            //Not* _func = (Not*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "udp_xbuf")
        {
            //Udp_xbuf* _func = (Udp_xbuf*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else if(cur_func.name == "udp_mux2")
        {
            //Udp_mux2* _func = (Udp_mux2*)cur_func;
            VCDBit _out_;
            calc(cur_func.name, cur_func.input_data, _out_);
            if (cur_func.get_output() == _out)
            {
                cur_out = _out_;
                break;
            }
            inter_values[cur_func.get_output()] = _out_;
        }
        else
        {
            cout << "ERROR: function " << cur_func.name << " is not supported." << endl;
            exit(-1);
        }
    }
}

void Process(std::map<std::string, TimedValues*> inValues, std::deque<TimedValueSim>& inTimedValues)
{
    int flag = 1;
    for(auto i:inValues)
    {
        string cur_name = i.first;
        TimedValues* cur_vals = i.second;
        if(flag == 1)
        {
            assert(inTimedValues.empty());
            for (auto it = cur_vals->begin(); it != cur_vals->end(); ++it)
            {
                VCDTime cur_t = (*it).t;
                VCDBit cur_val = (*it).value;
                TimedValueSim _sim;
                _sim.time = cur_t;
                (_sim.values).insert(make_pair(cur_name, cur_val));
                inTimedValues.push_back(_sim);
            }
            flag++;
            continue;
        }
        for (auto it = cur_vals->begin(); it != cur_vals->end(); ++it)
        {
            VCDTime cur_t = (*it).t;
            VCDBit cur_val = (*it).value;
            //binary search
            auto beg = inTimedValues.begin();
            auto end = inTimedValues.end();
            auto mid = beg + (end - beg) / 2;
            bool found = false;
            while(mid != end)
            {
                if((*mid).time < cur_t)
                {
                    beg = mid+1;
                    mid = beg + (end - beg) / 2;
                }
                else if((*mid).time > cur_t)
                {
                    end = mid;
                    mid = beg + (end - beg) / 2;
                }
                else
                {
                    found = true;
                    break;
                }
            }
            // found cur_t
            if (found)
            {
               ((*mid).values)[cur_name] = cur_val;
            }
            // not found
            else
            {
                if (inTimedValues.empty())
                {
                    TimedValueSim _sim;
                    _sim.time = cur_t;
                    (_sim.values).insert(make_pair(cur_name, cur_val));
                    inTimedValues.push_back(_sim);
                }
                else if(inTimedValues.size() == 1)
                {
                    if(inTimedValues[0].time < cur_t){
                        TimedValueSim _sim;
                        _sim.time = cur_t;
                        (_sim.values).insert(make_pair(cur_name, cur_val));
                        inTimedValues.push_back(_sim);
                    }
                    else if(inTimedValues[0].time > cur_t){
                        TimedValueSim _sim;
                        _sim.time = cur_t;
                        (_sim.values).insert(make_pair(cur_name, cur_val));
                        inTimedValues.push_front(_sim);
                    }
                    else{
                        (inTimedValues[0].values)[cur_name] = cur_val;
                    }
                }
                else
                {
                    TimedValueSim _sim;
                    _sim.time = cur_t;
                    (_sim.values).insert(make_pair(cur_name, cur_val));
                    inTimedValues.insert(mid, _sim);
                }
            }
        }
    }
}

void GetValueAt(std::map<std::string, TimedValues*> bitValues, std::string name, VCDTime time, VCDBit &cur, VCDBit &last)
{
    std::map<std::string, TimedValues*>::iterator find = bitValues.find(name); 
    if(find == bitValues.end()) {
        cout << "EEROR: net " << name << " not found in timedValues." << endl;
        exit(-1);
    }
    
    TimedValues * vals = find->second;

    if(vals -> size() == 0) {
        cur = VCD_X;
        last = VCD_X;
        return;
    }

    cur = VCD_X;
    last = VCD_X;
    //binary search
    auto beg = vals->begin();
    auto end = vals->end();
    auto mid = beg + (end - beg) / 2;
    while(mid != end)
    {
        if((*mid).t < time)
        {
            cur = (*mid).value;
            last = (*mid).value;
            beg = mid+1;
            mid = beg + (end - beg) / 2;
        }
        else if((*mid).t > time)
        {
            end = mid;
            mid = beg + (end - beg) / 2;
        }
        else
        {
            cur = (*mid).value;
            if (mid != vals->begin())
            {
                last = (*(mid - 1)).value;
            }
            break;
        }
    }
    //cout << "\t\tget value at -- name: " << name << ", " << (*mid).t << "->" << (*mid).value <<endl;
}


int main(int argc, char const *argv[])
{
    std::string vcdFilePath, database_path, saif_out;
    if (argc == 4)
    {
        vcdFilePath = argv[2];
        database_path = argv[1];
        saif_out = argv[3];
    }
    else
    {
        std::cout << "[USAGE] ./simulate [intermediate_file] vcd_path saif_out" << std::endl;
        exit(-1);
    }
    PreProcess processor;
    std::vector<std::vector<int> > levels;
    inter _inter;
    ifstream ifs(database_path.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia & _inter;
    processor = _inter.p;
    levels =  _inter .levels;
    cout << "new:" << endl;
    //debug
    cout << levels.size() << endl;
    cout << processor.instances.size() << endl;
    //
    ifs.close();

    std::map<std::string, Instance> instances = processor.get_instances();
    std::vector<string> _inst_name_vec = processor.get_instance_names();
    std::map<std::string, std::string> assign_pairs = processor.get_assign_pairs();
    std::map<std::string, TimedValues> pinbitValues = processor.get_pinbitValues();

    cout << "start parsing vcd file..." << endl;
    auto start_vcd = std::chrono::steady_clock::now();
    /** parse vcd & simulate **/
    VCDParser parser;
    if(parser.parse(vcdFilePath))
    {
        auto end_vcd = std::chrono::steady_clock::now();
        long duration_vcd = std::chrono::duration_cast<std::chrono::milliseconds>(end_vcd - start_vcd).count();
        cout << "total time of vcd parse: " << duration_vcd << "ms" << endl;
        //vcd signals
        std::vector<VCDSignal*> signals = parser.get_signals();
        std::vector<VCDTime>    times = parser.get_timestamps();
        VCDTimeUnit time_unit = parser.time_units;
        unsigned time_res = parser.time_resolution;
        VCDScope *root = parser.root_scope;
        string root_name = root->name;
        //VCDTime start_time = times[0];
        //std::unordered_map<std::string, std::string> name_hash_pair = parser.get_name_hash_pair();
        //std::unordered_map<std::string, std::string> hash_name_pair = parser.get_hash_name_pair();
        std::unordered_map<std::string, TimedValues*> val_map = parser.get_val_map();
        
        /** Simulate **/
        cout << "start simulator..." << endl;
        auto start = std::chrono::steady_clock::now();
        //CPU single thread
        std::unordered_map<std::string, TimedValues*> out_value;
        for (unsigned i = 0; i < levels.size(); ++i)
        {
            cout << "level " << i << endl;
            std::vector<int> cur_int = levels[i];
            for (unsigned j = 0; j < cur_int.size(); ++j)
            {
                string _name = _inst_name_vec[cur_int[j]];
                if (j == cur_int.size()/10)
                {
                    cout << "\t\t10" << "%" << " have done!" << endl;
                }
                else if (j == cur_int.size()/5)
                {
                    cout << "\t\t20" << "%" << " have done!" << endl;
                }
                else if (j == 3*cur_int.size()/10)
                {
                    cout << "\t\t30" << "%" << " have done!" << endl;
                }
		        else if (j == 4*cur_int.size()/10)
                {
                    cout << "\t\t40" << "%" << " have done!" << endl;
                }
		        else if (j == 5*cur_int.size()/10)
                {
                    cout << "\t\t50" << "%" << " have done!" << endl;
                }
		        else if (j == 6*cur_int.size()/10)
                {
                    cout << "\t\t60" << "%" << " have done!" << endl;
                }
		        else if (j == 7*cur_int.size()/10)
                {
                    cout << "\t\t70" << "%" << " have done!" << endl;
                }
		        else if (j == 8*cur_int.size()/10)
                {
                    cout << "\t\t80" << "%" << " have done!" << endl;
                }
		        else if (j == 9*cur_int.size()/10)
                {
                    cout << "\t\t90" << "%" << " have done!" << endl;
                }
                else if (j == cur_int.size()-1)
                {
                    cout << "\t\t100" << "%" << " have done!" << endl;
                }
                Instance cur_inst = instances[_name];
                std::map<std::string, TimedValues*> bitValues;
                for(unsigned it = 0; it < cur_inst.in_net.size(); it++)
                {
                    string in_name = (cur_inst.in_net)[it];
                    if (val_map.find(in_name) != val_map.end())
                    {
                        bitValues[in_name] = val_map[in_name];
                    }
                    else if (out_value.find(in_name) != out_value.end())
                    {
                        bitValues[in_name] = out_value[in_name];
                    }
                    else if (pinbitValues.find(in_name) != pinbitValues.end())
                    {
                        TimedValues* cur_tv_deq = new TimedValues();
                        for (auto k = pinbitValues[in_name].begin(); k != pinbitValues[in_name].end(); ++k)
                        {
                            cur_tv_deq->push_back(*k);
                        }
                        bitValues[in_name] = cur_tv_deq;
                    }
                    else if (assign_pairs.find(in_name) != assign_pairs.end())
                    {
                        string temp = assign_pairs[in_name];
                        if (val_map.find(temp) != val_map.end())
                        {
                            bitValues[in_name] = val_map[temp];
                        }
                        else if (out_value.find(temp) != out_value.end())
                        {
                            bitValues[in_name] = out_value[temp];
                        }
                        else if (pinbitValues.find(temp) != pinbitValues.end())
                        {
                            TimedValues* cur_tv_deq = new TimedValues();
                            for (auto k = pinbitValues[temp].begin(); k != pinbitValues[temp].end(); ++k)
                            {
                                cur_tv_deq->push_back(*k);
                            }
                            bitValues[in_name] = cur_tv_deq;
                        }
                    }
                    else
                    {
                        cout << "error." << endl;
                        exit(-1);
                    }
                }
                cout << "\tInstance: " << _name << " start simulating..." << endl;
                SimulateInst(cur_inst, bitValues, out_value, time_unit);
            }
        }
        /*for(auto out:out_value)
        {
            if(out.first == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_tl2axi4_ICCADs_n49"){
            cout << "net: " << out.first << endl;
            for (auto i = (out.second)->begin(); i != (out.second)->end(); ++i)
            {
                cout << "\ttime: " << (*i).t << " val: " << (*i).value << endl;
            }
        }
            
        }
        for(auto out:val_map)
        {
            if(out.first == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_tl2axi4_ICCADs__T_168[0]" ||
                out.first == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_tl2axi4_ICCADs__T_168[1]" || 
                out.first == "sbus_ICCADs_coupler_to_port_named_mmio_port_axi4_ICCADs_tl2axi4_ICCADs__T_168[2]"){
            cout << "net: " << out.first << endl;
            for (auto i = (out.second)->begin(); i != (out.second)->end(); ++i)
            {
                cout << "\ttime: " << (*i).t << " val: " << (*i).value << endl;
            }
            }
        }*/
        VCDTime sim_start = 10000;
        VCDTime sim_end = 100000001;
        DumpSaif(val_map, out_value, pinbitValues, saif_out, time_unit, time_res, root_name, sim_start, sim_end);
        /**CPU thread
        std::unordered_map<std::string, TimedValues*> out_value; 
        for (unsigned i = 0; i < levels.size(); ++i)
        {
            cout << "\tlevel " << i << endl;
            std::vector<int> cur_int = levels[i];
            unsigned long work_size = cur_int.size();
            //unsigned long max_thread = std::thread::hardware_concurrency();
            //unsigned long thread_num = min(work_size, max_thread);
            //unsigned block_num = work_size / thread_num; 
            std::vector<std::thread> thread_vec(work_size);
            for (unsigned j = 0; j < work_size; ++j)
            {
                string _name = _inst_name_vec[cur_int[j]];
                cout << "\t\tinstance name: " << _name << endl;
                Instance* cur_inst = instances[_name];
                std::map<std::string, TimedValues*> bitValues;
                for(auto it = (cur_inst->get_in()).begin(); it != (cur_inst->get_in()).end(); it++)
                {
                    string in_name = it->second;
                    if (val_map.find(in_name) != val_map.end())
                    {
                        bitValues[in_name] = val_map[in_name];
                    }
                    else if (out_value.find(in_name) != out_value.end())
                    {
                        bitValues[in_name] = out_value[in_name];
                    }
                    else if (assign_pairs.find(in_name) != assign_pairs.end())
                    {
                        string temp = assign_pairs[in_name];
                        if (val_map.find(temp) != val_map.end())
                        {
                            bitValues[in_name] = val_map[temp];
                        }
                        else if (out_value.find(temp) != out_value.end())
                        {
                            bitValues[in_name] = out_value[temp];
                        }
                    }
                    else
                    {
                        cout << "error." << endl;
                        exit(-1);
                    }
                }
                thread_vec[j] = std::thread(SimulateInst, cur_inst, bitValues, std::ref(out_value), time_unit);
            }
            for (unsigned j = 0; j < work_size; ++j)
            {
                thread_vec[j].join();
            }
        }*/
        //GPU cuda
        /*std::unordered_map<std::string, TimedValues*> out_value;
        for (unsigned i = 0; i < levels.size(); ++i)
        {
            std::vector<int> cur_level = levels[i];
            int inst_num = cur_level.size();
            int j = 0;
            while(j < inst_num)
            {
                Instance cur_inst = instances[_inst_name_vec[cur_level[i]]];
                std::map<std::string, std::string> _in = cur_inst.get_in();
                std::map<std::string, std::string> _out = cur_inst.get_out();
                std::vector<Delay> delays = cur_inst.get_delay();
                std::vector<string> cur_in = cur_inst.in_net;
                std::vector<string> cur_out = cur_inst.out_net;
                std::map<std::string, TimedValues*> bitValues;
                for(unsigned it = 0; it < cur_in.size(); it++)
                {
                    string in_name = cur_in[it];
                    if (val_map.find(in_name) != val_map.end())
                    {
                        bitValues[in_name] = val_map[in_name];
                    }
                    else if (out_value.find(in_name) != out_value.end())
                    {
                        bitValues[in_name] = out_value[in_name];
                    }
                    else if (assign_pairs.find(in_name) != assign_pairs.end())
                    {
                        string temp = assign_pairs[in_name];
                        if (val_map.find(temp) != val_map.end())
                        {
                            bitValues[in_name] = val_map[temp];
                        }
                        else if (out_value.find(temp) != out_value.end())
                        {
                            bitValues[in_name] = out_value[temp];
                        }
                    }
                    else
                    {
                        cout << "error." << endl;
                        exit(-1);
                    }
                }
                std::deque<TimedValueSim> inTimedValues;
                Process(bitValues, inTimedValues);
                //data to GPU
                int time_unit = time_unit;
                std::vector<unsigned long long> times;
                std::vector<std::vector<int> > in_datas(cur_in.size(), vector<int>(0));
                while(!inTimedValues.empty())
                {
                    TimedValueSim tmp = inTimedValues.front();
                    inTimedValues.pop_front();
                    VCDTime cur_t = tmp.time;
                    std::map<std::string, VCDBit> val = tmp.values;
                    times.push_back(cur_t);
                    for (unsigned k = 0; k < cur_in.size(); ++k)
                    {
                        VCDBit cur, last;
                        GetValueAt(bitValues, cur_in[k], cur_t, cur, last);
                        in_datas[k].push_back(cur);
                    }
                }
                int *host_in_data;
                int height = cur_in.size();
                int width = times.size();
                host_in_data = (int *)malloc(sizeof(int)*width*height);
                for (int iii = 0; iii < height; ++iii)
                {
                    for (int kkk = 0; kkk < width; ++kkk)
                    {
                        host_in_data[iii*width+kkk] = in_datas[iii][kkk];
                    }
                }
                //delay to GPU
                std::vector<int> delay_edge;
                std::vector<int> in_bit;
                std::vector<int> out_bit;
                std::vector<double> rise_val;
                std::vector<double> fall_val;
                for (unsigned it = 0; it < delays.size(); ++it)
                {
                    delay_edge.push_back(delays[it].edge);
                    auto pos = find(cur_in.begin(), cur_in.end(), delays[it].in_bit);
                    in_bit.push_back(distance(cur_in.begin(), pos));
                    pos = find(cur_out.begin(), cur_out.end(), delays[it].out_bit);
                    out_bit.push_back(distance(cur_out.begin(), pos));
                    rise_val.push_back(delays[it].rise_val);
                    fall_val.push_back(delays[it].fall_val);
                }
                //funtion: for every function: [func_id, in1_id, in2_id,...,out1_id, out2_id,-1,-1,...], in_id 1001 means constant 1, 1000 means 0
                std::vector<std::vector<int> > functions = cur_inst.function_id_vec;
                int func_height = functions.size();
                int func_width = 0;
                for (unsigned jjj = 0; jjj < functions.size(); ++jjj)
                {
                    func_width = max(functions[jjj].size(),func_width);
                }
                int *host_functions;
                host_functions=(int *)malloc(sizeof(int)*func_width*func_height);
                for (int iii = 0; iii < func_height; ++iii)
                {
                    for (int kkk = 0; kkk < func_width; ++kkk)
                    {
                        if(kkk >= functions[iii].size())
                            host_functions[iii*width+kkk] = -1;
                        else
                            host_functions[iii*width+kkk] = functions[iii][kkk];
                    }
                }
                //GPU Simulate
                //InitialCuda
                unsigned long long *dev_times;
                int *dev_delay_edges,*dev_in_bit,*dev_out_bit;
                double *dev_rise_val, *dev_fall_val;
                int *dev_in_datas;  // 2D
                size_t pitch_data;
                int dev_time_unit;
                int *dev_functions;  // 2D
                size_t pitch_function;
                err=cudaMalloc((void **)&dev_time_unit, sizeof(int));
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(int) on GPU is failed");
                    exit(-1);
                }
                err=cudaMalloc((void **)&dev_times, sizeof(unsigned long long)*times.size());
                err=cudaMalloc((void **)&dev_delay_edges, sizeof(int)*delay_edge.size());
                err=cudaMalloc((void **)&dev_in_bit, sizeof(int)*in_bit.size());
                err=cudaMalloc((void **)&dev_out_bit, sizeof(int)*out_bit.size());
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(int*) on GPU is failed");
                    exit(-1);
                }
                err=cudaMalloc((void **)&dev_rise_val, sizeof(double)*rise_val.size());
                err=cudaMalloc((void **)&dev_fall_val, sizeof(double)*fall_val.size());
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(double*) on GPU is failed");
                    exit(-1);
                }
                err = cudaMallocPitch((void **)&dev_in_datas, &pitch_data, sizeof(int)*width, height);
                err = cudaMallocPitch((void **)&dev_functions, &pitch_function, sizeof(int)*func_width, func_height);
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(int**) on GPU is failed");
                    exit(-1);
                }
                printf("SUCCESS");
                //CPU to GPU
                cudaMemcpy(dev_times,&times[0],sizeof(unsigned long long)*times.size(),cudaMemcpyHostToDevice);
                cudaMemcpy(dev_delay_edges,&delay_edges[0],sizeof(int)*delay_edge.size(),cudaMemcpyHostToDevice);
                cudaMemcpy(dev_in_bit,&in_bit[0],sizeof(int)*in_bit.size(),cudaMemcpyHostToDevice);
                cudaMemcpy(dev_out_bit,&out_bit[0],sizeof(int)*out_bit.size(),cudaMemcpyHostToDevice);
                cudaMemcpy(dev_rise_val,&rise_val[0],sizeof(double)*rise_val.size(),cudaMemcpyHostToDevice);
                cudaMemcpy(dev_fall_val,&fall_val[0],sizeof(double)*fall_val.size(),cudaMemcpyHostToDevice);
                cudaMemcpy2D(dev_in_datas, pitch_data, host_in_data,sizeof(int)*width, sizeof(int)*width, height, cudaMemcpyHostToDevice);
                cudaMemcpy2D(dev_functions, pitch_function, host_functions,sizeof(int)*func_width, sizeof(int)*func_width, func_height, cudaMemcpyHostToDevice);

                //parallel
                inst_num
                SimulateCuda<<<1,inst_num>>>(dev_a,dev_b,dev_c);

                cudaMemcpy(&host_c,dev_c,sizeof(host_c),cudaMemcpyDeviceToHost);
                for(int i=0;i<512;i++)
                    printf("host_a[%d] + host_b[%d] = %d + %d = %d\n",i,i,host_a[i],host_b[i],host_c[i]);
                cudaFree(dev_a);//释放GPU内存
                cudaFree(dev_b);//释放GPU内存
                cudaFree(dev_c);//释放GPU内存
                j++;
            }
        }*/
        auto end = std::chrono::steady_clock::now();
        long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        cout << "total time of simulating: " << duration << "ms" << endl;
    }
    else
    {
        std::cout << "Parse Failed." << std::endl;
        return 1;
    }
    return 0;
}