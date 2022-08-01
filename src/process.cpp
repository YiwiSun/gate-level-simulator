#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <iterator>
#include "VCDParser.h"
#include "PreProcess.h"
#include "Instance.h"
#include "Function.h"
#include "VCDTypes.h"
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
//#include <cuda.h>
class inter
{
public:
    inter(){}
    ~inter(){}

    /* data */
    PreProcess p;
    std::vector<std::vector<int> > levels;
    std::map<std::string, int> initial_net_map;

    std::vector<unsigned int> data_in_num_start;
    std::vector<unsigned int> data_out_num_start;
    std::vector<unsigned int> delay_start;
    std::vector<unsigned int> delay_width;
    std::vector<unsigned int> functions_start;
    std::vector<unsigned int> functions_width;

    std::vector<short> data_in_num;
    std::vector<short> data_out_num;
    std::vector<unsigned int> val_num_start;
    std::vector<short> delay_val_num;
    std::vector<short> functions_func_num;
    std::vector<short> functions_val_num;
    std::vector<short> host_delay_edges;
    std::vector<short> host_in_bit;
    std::vector<short> host_out_bit;
    std::vector<float> host_rise_val;
    std::vector<float> host_fall_val;

    std::vector<short> host_functions;
    std::vector<std::map<std::string, std::vector<unsigned int> > > OutMaps;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) //
    {            //ar.register_type(static_cast<ModuleType *>(NULL));
        ar & p;
        ar & levels;
        ar & initial_net_map;
        ar & data_in_num_start;
        ar & data_out_num_start;
        ar & delay_start;
        ar & delay_width;
        ar & functions_start;
        ar & functions_width;
        ar & data_in_num;
        ar & data_out_num;
        ar & val_num_start;
        ar & delay_val_num;
        ar & functions_func_num;
        ar & functions_val_num;
        ar & host_delay_edges;
        ar & host_in_bit;
        ar & host_out_bit;
        ar & host_rise_val;
        ar & host_fall_val;
        ar & host_functions;
        ar & OutMaps;
    }
};
using namespace std;
//void ProcessVertices(std::unordered_map<std::string, Instance*> &instances, std::unordered_map<std::string, std::string> &name_hash_pair, std::vector<string> &inst_names, std::vector<string> &start_vec);
void GenerateAdjacencyList(std::map<std::string, Instance> &instances, std::vector<string> &inst_names, 
    std::map<std::string, std::vector<int> > net_instance_map, std::vector<int> &adjacencyList, std::vector<int> &edgesOffset, std::vector<int> &edgesSize);
void SetInDegrees(std::vector<int> adjacencyList, std::vector<int> edgesOffset, std::vector<int> edgesSize, std::vector<int>& indegrees);
std::vector<std::vector<int> > TopologyOrder(std::vector<int> adjacencyList, std::vector<int> edgesOffset, 
    std::vector<int> edgesSize, std::vector<int> indegrees, int flag, std::vector<std::vector<int> > levels, int n);

void GenerateAdjacencyList(std::map<std::string, Instance> &instances, std::vector<string> &inst_names, 
    std::map<std::string, std::vector<int> > net_instance_map, std::vector<int> &adjacencyList, std::vector<int> &edgesOffset, std::vector<int> &edgesSize)
{
    int n = inst_names.size();
    std::vector<std::vector<int> > adjecancyLists(n);
    for (unsigned i = 0; i < inst_names.size(); ++i)
    {
        Instance cur_inst = (instances.find(inst_names[i]))->second;
        std::map<std::string, std::string> out_ = cur_inst.get_out();
        for (std::map<std::string, std::string>::iterator j = out_.begin(); j != out_.end(); ++j)
        {
            string cur_out = j->second;
            if (net_instance_map.find(cur_out) != net_instance_map.end())
            {
                adjecancyLists[i].insert(adjecancyLists[i].end(), net_instance_map[cur_out].begin(), net_instance_map[cur_out].end());
            }
        }
        sort(adjecancyLists[i].begin(),adjecancyLists[i].end());  
        adjecancyLists[i].erase(unique(adjecancyLists[i].begin(),adjecancyLists[i].end()),adjecancyLists[i].end());
        edgesOffset.push_back(adjacencyList.size());
        edgesSize.push_back(adjecancyLists[i].size());
        adjacencyList.insert(adjacencyList.end(), adjecancyLists[i].begin(), adjecancyLists[i].end());
    }
}

void SetInDegrees(std::vector<int> adjacencyList, std::vector<int> edgesOffset, std::vector<int> edgesSize, std::vector<int>& indegrees)
{
    unsigned num = edgesOffset.size();
    for (unsigned i = 0; i < num; ++i)
    {
        for (int j = edgesOffset[i]; j < edgesOffset[i] + edgesSize[i]; j++)
        {
            int v = adjacencyList[j];
            indegrees[v] += 1;
        }
    }
}

std::vector<std::vector<int> > TopologyOrder(std::vector<int> adjacencyList, std::vector<int> edgesOffset, 
    std::vector<int> edgesSize, std::vector<int> indegrees, int flag, std::vector<std::vector<int> > levels, int n)
{
    if(n <= 0)
        return levels;
    std::vector<int> Q;
    for (unsigned i = 0; i < indegrees.size(); ++i)
    {
        if (indegrees[i] == 0)
        {
            indegrees[i]--;
            Q.push_back(i);
        }
    }
    //no indegrees = 0
    if (Q.empty())
    {
        std::vector<int> _edgesOffset = edgesOffset;
        for (unsigned i = 0; i < _edgesOffset.size(); ++i)
        {
            std::vector<int> tmp;
            tmp.push_back(i);
            levels.push_back(tmp);
            flag++;
            n--;
        }
        return TopologyOrder(adjacencyList, edgesOffset, edgesSize, indegrees, flag, levels, n);
    }
    //
    else
    {
        levels.push_back(Q);
        flag++;
        for (unsigned i = 0; i < Q.size(); ++i)
        {
            int u = Q[i];
            for (int j = edgesOffset[u]; j < edgesOffset[u] + edgesSize[u]; j++)
            {
                int v = adjacencyList[j];
                indegrees[v] -= 1;
            }
        }
        for (unsigned i = 0; i < Q.size(); ++i)
        {
            n--;
        }
        return TopologyOrder(adjacencyList, edgesOffset, edgesSize, indegrees, flag, levels, n);
    }
}

int main(int argc, char **argv)
{
	std::string vlib_path, gv_path, sdf_path, intermediate_file;
	PreProcess processor;

	if (argc == 5)
	{
        vlib_path = argv[1];
        gv_path = argv[2];
        sdf_path = argv[3];
        intermediate_file = argv[4];
	}
	else
	{
		std::cout << "[USAGE] ./process vlib_path gv_path sdf_path [intermediate_file]" << std::endl;
        exit(-1);
	}
    /** pre process **/
    processor.parse_vlib(vlib_path);
    processor.parse_gv(gv_path);
    processor.parse_sdf(sdf_path);
    //all signals and instances
    std::map<std::string, Instance> instances = processor.get_instances();
    std::map<std::string, TimedValues*> pinbitValues = processor.get_pinbitValues();
    std::map<std::string, std::string> assign_pairs = processor.get_assign_pairs();
    /*cout << "instances size: " << instances.size() << endl;
    for(auto inst:instances)
    {
        cout << "inst name: " << inst.second.name << endl;
        for(auto in_net:(inst.second.in_port_net))
        {
            cout << "in: " << in_net.first << "(" << in_net.second << ")" <<endl;
        }
        for(auto delay:(inst.second._delay))
        {
            cout << "edge: " << delay.edge << " in: " <<delay.in_bit << " out: " << delay.out_bit << " rise: " << delay.rise_val << " fall: " << delay.fall_val << endl; 
        }
    }*/
    
    //for gragh generation
    std::vector<int> adjacencyList; // all edges
    std::vector<int> edgesOffset; // offset to adjacencyList for every vertex
    std::vector<int> edgesSize; //number of edges for every vertex
    std::vector<string> _inst_name_vec = processor.get_instance_names();   // position is the same as the above 3 vector's position
    std::map<std::string, std::vector<int> > net_instance_map = processor.net_instance_map;
    

    //generate adjacency list
    cout << "start generating graph..." << endl;
    auto start_graph = std::chrono::steady_clock::now();
    GenerateAdjacencyList(instances, _inst_name_vec, net_instance_map, adjacencyList, edgesOffset, edgesSize);
    //Graph* graph = new Graph(edgesOffset.size());
    //graph->generateGraph(adjacencyList, edgesOffset, edgesSize);
    auto end_graph = std::chrono::steady_clock::now();
    long duration_graph = std::chrono::duration_cast<std::chrono::milliseconds>(end_graph - start_graph).count();
    cout << "total time of graph generating: " << duration_graph << "ms" << endl;

    //BFS
    //runCudaBFS(start_vec, graph, );
        /** CPU topology order **/
    
    cout << "start topology order..." << endl;
    auto start_top = std::chrono::steady_clock::now();
    std::vector<int> indegrees(edgesOffset.size());
    SetInDegrees(adjacencyList, edgesOffset, edgesSize, indegrees);
    int flag = 1;
    int n = edgesOffset.size();
    std::vector<std::vector<int> > initial_levels;
    std::vector<std::vector<int> > levels = TopologyOrder(adjacencyList, edgesOffset, edgesSize, indegrees, flag, initial_levels, n);
    auto end_top = std::chrono::steady_clock::now();
    long duration_top = std::chrono::duration_cast<std::chrono::milliseconds>(end_top - start_top).count();
    cout << "total time of topology order: " << duration_top << "ms" << endl;
    cout << "levels size: " << levels.size() << endl;

    // process levels
    std::vector<int> inst_level(_inst_name_vec.size());
    std::vector<int> inst_pos_at_level(_inst_name_vec.size());
    std::vector<std::map<std::string, std::vector<unsigned int> > > OutMaps(levels.size());
    for (unsigned i = 0; i < levels.size(); ++i)
    {
        std::vector<int> cur_level = levels[i];
        for (unsigned j = 0; j < cur_level.size(); ++j)
        {
            int cur_id = cur_level[j];
            Instance cur_inst = (instances.find(_inst_name_vec[cur_id]))->second;
            std::vector<string> _out_net = cur_inst.out_net;
            for(unsigned k = 0; k < _out_net.size(); ++k){
                string net_name = _out_net[k];
                if(OutMaps[i].find(net_name) == OutMaps[i].end()){
                    std::vector<unsigned int> v = {j, k};
                    (OutMaps[i])[net_name] = v;
                }
            }
            inst_level[cur_id] = i;
            inst_pos_at_level[cur_id] = j;
        }
    }

    // generate 'out net from' info
    std::map<std::string, int> out_net_from_id = processor.out_net_from_id;
    std::map<std::string, int> initial_net_map;
    int initial_id = 0;
    for (unsigned i = 0; i < _inst_name_vec.size(); ++i)
    {
        Instance cur_inst = (instances.find(_inst_name_vec[i]))->second;
        std::vector<string> _in_net = cur_inst.in_net;
        for (unsigned j = 0; j < _in_net.size(); ++j)
        {
            string cur_in = _in_net[j];
            if (out_net_from_id.find(cur_in) != out_net_from_id.end())
            {
                int _id = out_net_from_id[cur_in];
                Instance _cur_inst = (instances.find(_inst_name_vec[_id]))->second;
                std::vector<string> _out_net = _cur_inst.out_net;
                int out_order;
                if (_out_net.size() <= 1)   // only one output
                {
                    out_order = 0;
                }
                else{
                    auto pos = find(_out_net.begin(), _out_net.end(), cur_in);
                    out_order = distance(_out_net.begin(), pos);
                }
                processor.add_in_net_from(_id, _inst_name_vec[_id], inst_level[_id], inst_pos_at_level[_id], _inst_name_vec[i], out_order);
                //(processor.instances)[_inst_name_vec[i]].in_net_from_id.push_back(out_net_from_id[cur_in]);
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_info.push_back(_inst_name_vec[out_net_from_id[cur_in]]);
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_level.push_back(inst_level[out_net_from_id[cur_in]]);
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_pos_at_level.push_back(inst_pos_at_level[out_net_from_id[cur_in]]);
            }
            else if(pinbitValues.find(cur_in) != pinbitValues.end())
            {
                
                //regard it as a constant
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_id.push_back(-2);
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_info.push_back(cur_in);
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_level.push_back(-2);
                // at this case, 'in net from pos at level' means value
                TimedValues *tvs = pinbitValues[cur_in];
                int cur_val = (*(tvs->begin())).value;
                //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_pos_at_level.push_back(cur_val);
                processor.add_in_net_from(-2, cur_in, -2, cur_val, _inst_name_vec[i], 0);    
            }
            else if (assign_pairs.find(cur_in) != assign_pairs.end())
            {
                string temp = assign_pairs[cur_in];
                if (out_net_from_id.find(temp) != out_net_from_id.end())
                {
                    int _id = out_net_from_id[temp];
                    Instance _cur_inst = (instances.find(_inst_name_vec[_id]))->second;
                    std::vector<string> _out_net = _cur_inst.out_net;
                    int out_order;
                    if (_out_net.size() <= 1)   // only one output
                    {
                        out_order = 0;
                    }
                    else{
                        auto pos = find(_out_net.begin(), _out_net.end(), temp);
                        out_order = distance(_out_net.begin(), pos);
                    }
                    processor.add_in_net_from(_id, _inst_name_vec[_id], inst_level[_id], inst_pos_at_level[_id], _inst_name_vec[i], out_order);
                }
                else if (pinbitValues.find(temp) != pinbitValues.end())
                {
                    // at this case, 'in net from pos at level' means value
                    TimedValues *tvs = pinbitValues[temp];
                    int cur_val = (*(tvs->begin())).value;
                    //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_pos_at_level.push_back(cur_val);    
                    processor.add_in_net_from(-2, temp, -2, cur_val, _inst_name_vec[i], 0); 
                }
                else
                {
                    /*(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_id.push_back(-1);
                    (((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_info.push_back(temp);
                    (((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_level.push_back(-1);*/
                    if (initial_net_map.find(temp) == initial_net_map.end())
                    {
                        initial_net_map[temp] = initial_id;
                        initial_id++;
                    }
                    //(((processor.instances).find(_inst_name_vec[i]))->second).in_net_from_pos_at_level.push_back(initial_net_map[temp]);
                    
                    processor.add_in_net_from(-1, temp, -1, initial_net_map[temp], _inst_name_vec[i], 0); 
                }
            }
            else
            {
                if(initial_net_map.find(cur_in) == initial_net_map.end())
                {
                    initial_net_map[cur_in] = initial_id;
                    initial_id++;
                }
                processor.add_in_net_from(-1, cur_in, -1, initial_net_map[cur_in], _inst_name_vec[i], 0); 
            }
        }
    }

    //std::vector<std::vector<int> > in_net_from_levels(levels.size());
    //std::vector<std::vector<int> > in_net_from_pos_at_levels(levels.size());
    std::vector<std::vector<int> > in_net_from_levels(levels.size());
    std::vector<std::vector<int> > in_net_from_levels_num(levels.size());
    for (unsigned i = 0; i < levels.size(); ++i)
    {
        std::vector<int> cur_level = levels[i];
        for (unsigned j = 0; j < cur_level.size(); ++j)
        {
            int cur_id = cur_level[j];
            Instance cur_inst = (processor.instances.find(_inst_name_vec[cur_id]))->second;
            std::vector<int> _in_net_from_level = cur_inst.in_net_from_level;
            for (unsigned k = 0; k < _in_net_from_level.size(); ++k)
            {
                int tmp = _in_net_from_level[k];
                if (find(in_net_from_levels[i].begin(), in_net_from_levels[i].end(), tmp) == in_net_from_levels[i].end())
                {
                    in_net_from_levels[i].push_back(tmp);
                    in_net_from_levels_num[i].push_back(1);
                }
                else
                {
                    auto it = find(in_net_from_levels[i].begin(), in_net_from_levels[i].end(), tmp);
                    int pos = distance(in_net_from_levels[i].begin(), it);
                    in_net_from_levels_num[i][pos] += 1;
                }
            }
        }
    }
    //debug
    /*for (unsigned i = 0; i < levels.size(); ++i)
    {
        cout << "level: " << i << endl;
        std::vector<int> cur = in_net_from_levels[i];
        std::vector<int> cur_num = in_net_from_levels_num[i];
        cout << "\tlevel:";
        for (unsigned j = 0; j < cur.size(); ++j)
        {
            cout << cur[j] << " ";
        }
        cout << endl;
        cout << "\nnum:";
        for (unsigned j = 0; j < cur_num.size(); ++j)
        {
            cout << cur_num[j] << " ";
        }
        cout << endl;
    }*/

    // pre process for mapping to GPU
    int _size = _inst_name_vec.size();
    int total_size = levels.size();

    std::vector<unsigned int> data_in_num_start(total_size);
    std::vector<unsigned int> data_out_num_start(total_size);
    std::vector<unsigned int> delay_start(total_size);
    std::vector<unsigned int> delay_width(total_size);
    std::vector<unsigned int> functions_start(total_size);
    std::vector<unsigned int> functions_width(total_size);

    std::vector<short> data_in_num(_size);
    std::vector<short> data_out_num(_size);
    std::vector<unsigned int> val_num_start(_size);
    std::vector<short> delay_val_num(_size);
    std::vector<short> functions_func_num(_size);
    std::vector<short> functions_val_num(_size);

    int _sum = 0;
    int _delay_start = 0;
    int _functions_start = 0;
    
    for (unsigned i = 0; i < levels.size(); ++i)
    {
        std::vector<int> cur_level = levels[i];
        const int inst_num = cur_level.size();

        int val_num_width = 0;
        int delay_width_ = 0;
        int functions_width_ = 0;
        data_in_num_start[i] = _sum;
        data_out_num_start[i] = _sum;
        functions_start[i] = _functions_start;
        delay_start[i] = _delay_start;
        for (int j = 0; j < inst_num; ++j)
        {
            Instance cur_inst = instances[_inst_name_vec[cur_level[j]]];
            std::vector<string> cur_in = cur_inst.in_net;
            data_in_num[j+_sum] = cur_in.size();
            std::vector<string> cur_out = cur_inst.out_net;
            data_out_num[j+_sum] = cur_out.size();

            val_num_start[j+_sum] = val_num_width;
            unsigned int _val_num_width = cur_in.size();
            val_num_width += _val_num_width;

            std::vector<Delay> delays = cur_inst.get_delay();
            int _delay_width = delays.size();
            delay_val_num[j+_sum] = _delay_width;
            delay_width_ = max(_delay_width, delay_width_);

            std::vector<std::vector<int> > functions = cur_inst.function_id_vec;
            int func_height = functions.size();
            int func_width = 0;
            for (unsigned jjj = 0; jjj < functions.size(); ++jjj)
            {
                int _func_width = functions[jjj].size();
                func_width = max(_func_width,func_width);
            }
            functions_width_ = max(func_width*func_height, functions_width_);
            functions_func_num[j+_sum] = func_height;
            functions_val_num[j+_sum] = func_width;

        }
        _sum += inst_num;
        _delay_start += inst_num*delay_width_;
        _functions_start += inst_num*functions_width_;
        delay_width[i] = delay_width_;
        functions_width[i] = functions_width_;
    }
    //for (int i = 0; i < data_in_num_start.size(); ++i)
    //{
    //    cout << data_out_num_start[i] << endl;
    //}

    std::vector<short> host_delay_edges(_delay_start);
    std::vector<short> host_in_bit(_delay_start);
    std::vector<short> host_out_bit(_delay_start);
    std::vector<float> host_rise_val(_delay_start);
    std::vector<float> host_fall_val(_delay_start);

    std::vector<short> host_functions(_functions_start);

    for (unsigned i = 0; i < levels.size(); ++i)
    {
        std::vector<int> cur_level = levels[i];
        const int inst_num = cur_level.size();

        int delay_start_ = delay_start[i];
        int delay_width_ = delay_width[i];
        int functions_width_ = functions_width[i];
        int functions_start_ = functions_start[i];
        for (int j = 0; j < inst_num; ++j)
        {
            Instance cur_inst = instances[_inst_name_vec[cur_level[j]]];

            std::vector<Delay> delays = cur_inst.get_delay();
            std::vector<string> cur_in = cur_inst.in_net;
            std::vector<string> cur_out = cur_inst.out_net;
            for (unsigned it = 0; it < delays.size(); ++it)
            {
                host_delay_edges[delay_start_+j*delay_width_+it] = delays[it].edge;
                auto pos = find(cur_in.begin(), cur_in.end(), delays[it].in_bit);
                host_in_bit[delay_start_+j*delay_width_+it] = distance(cur_in.begin(), pos);
                pos = find(cur_out.begin(), cur_out.end(), delays[it].out_bit);
                host_out_bit[delay_start_+j*delay_width_+it] = distance(cur_out.begin(), pos);
                host_rise_val[delay_start_+j*delay_width_+it] = delays[it].rise_val;
                host_fall_val[delay_start_+j*delay_width_+it] = delays[it].fall_val;
            }
            if (delays.size() < delay_width_)
            {
                for (int ttt = delays.size(); ttt < delay_width_; ++ttt)
                {
                    host_delay_edges[delay_start_+j*delay_width_+ttt] = -1;
                    host_in_bit[delay_start_+j*delay_width_+ttt] = -1;
                    host_out_bit[delay_start_+j*delay_width_+ttt] = -1;
                    host_rise_val[delay_start_+j*delay_width_+ttt] = -1.0;
                    host_fall_val[delay_start_+j*delay_width_+ttt] = -1.0;
                }
            }

            std::vector<std::vector<int> > functions = cur_inst.function_id_vec;
            int func_height = functions.size();
            int func_width = functions_width_ / func_height;
            for (int iii = 0; iii < func_height; ++iii)
            {
                for (int kkk = 0; kkk < func_width; ++kkk)
                {
                    if(kkk >= functions[iii].size())
                        host_functions[functions_start_+j*functions_width_+iii*func_width+kkk] = -1;
                    else{
                        host_functions[functions_start_+j*functions_width_+iii*func_width+kkk] = functions[iii][kkk];
                    }
                }
            }
            if (func_width*func_height < functions_width_)
            {
                for (int ttt = func_width*func_height; ttt < functions_width_; ++ttt)
                {
                    host_functions[functions_start_+j*functions_width_+ttt] = -1;
                }
            }
        }
    }

    //debug
    /**
    for(unsigned i = 0; i < levels.size(); i++)
    {
        cout << "level: " << i << "(size:" << levels[i].size() << ")" << endl;
        cout << "\t";
        for (unsigned j = 0; j < (levels[i]).size(); ++j)
        {
            cout << levels[i][j] << "(" <<_inst_name_vec[levels[i][j]] << "), ";
        }
        cout << endl;
    }
    /**  **/
    inter _inter;
    _inter.p = processor;
    _inter.levels = levels;
    _inter.data_in_num_start = data_in_num_start;
    _inter.data_out_num_start = data_out_num_start;
    _inter.delay_start = delay_start;
    _inter.delay_width = delay_width;
    _inter.functions_start = functions_start;
    _inter.functions_width = functions_width;
    _inter.data_in_num = data_in_num;
    _inter.data_out_num = data_out_num;
    _inter.val_num_start = val_num_start;
    _inter.delay_val_num = delay_val_num;
    _inter.functions_func_num = functions_func_num;
    _inter.functions_val_num = functions_val_num;
    _inter.host_delay_edges = host_delay_edges;
    _inter.host_in_bit = host_in_bit;
    _inter.host_out_bit = host_out_bit;
    _inter.host_rise_val = host_rise_val;
    _inter.host_fall_val = host_fall_val;
    _inter.host_functions = host_functions;
    _inter.OutMaps = OutMaps;
    //template <class Instance>
    ofstream ofs(intermediate_file.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa & _inter;
    cout << "store database into " << intermediate_file << endl;
    ofs.close();
    
}
