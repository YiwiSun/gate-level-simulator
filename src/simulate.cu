
#include <iostream>
#include <set>
#include <string>
#include <chrono>
#include <thread>
#include "simulate.cuh"
#include <algorithm>
#include <limits>
#include <cstddef>
#include <thrust/scan.h>
#include <thrust/execution_policy.h>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
//#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/access.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#define COMPRESS_BASE 65536
#define COMPRESS_WIDTH 8
#define UNKNOWN 2
#define STATE_NUM 8
#define MAX_IN_NUM 6
#define MAX_OUT_NUM 2
#define UPDATE_N_THREADS_PER_BLOCK 256
#define N_THREADS_PER_BLOCK 512
#define _N_THREADS_PER_BLOCK 1024
#define N_TIMES_PER_THREAD 32
#define MIN_THREAD_NUM 4
#define MAX_BLOCKS 65535
#define _MAX_BLOCKS 65535
//#define SIM_START 10000
//#define SIM_START 0
//#define SIM_END 100000001 
//#define SIM_END 20000001 
//#define SIM_END 2539945010
//#define SIM_END 2972036001
#define MAX_NUM 100000001
typedef struct {
    int x;
    int y;
} duo;
class inter
{
public:
    inter(){}
    ~inter(){}
    Instance* find_inst(int id){
        return p.find_inst(id);
    }

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
    std::vector<std::map<std::string, std::vector<unsigned int> > > OutMaps;

    std::vector<short> host_functions;
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
std::unordered_map<std::string, std::vector<unsigned long long> > Result;
std::unordered_map<std::string, std::vector<unsigned long long> > OtherResult;
std::unordered_set<std::string> pin_bits_set;
__device__ unsigned int dev_out_width;

void DumpSaif(ofstream &outf){
    for(auto &it:Result){
        string net_name = it.first;
        if(net_name.find("[") != std::string::npos){
            pin_bits_set.erase(net_name);
        }
        std::vector<unsigned long long> v = it.second;
        outf << "      (" << net_name << endl;
        outf << "         (T0 " << v[0] << ") (T1 " << v[1] << ") (TX " << v[2] << ")" << endl;
        outf << "      )" << endl;
    }
}

void DumpSaif(ofstream &outf, std::unordered_map<std::string, std::vector<unsigned long long> >& inResult){
    for(auto &it:inResult){
        string net_name = it.first;
        if(net_name.find("[") != std::string::npos){
            pin_bits_set.erase(net_name);
        }
        std::vector<unsigned long long> v = it.second;
        outf << "      (" << net_name << endl;
        outf << "         (T0 " << v[0] << ") (T1 " << v[1] << ") (TX " << v[2] << ")" << endl;
        outf << "      )" << endl;
    }
}

void DumpSaif(ofstream &outf, std::map<std::string, std::vector<unsigned long long> >& pinResult, VCDTime start, VCDTime end){
    for(auto &it:pinResult){
        string net_name = it.first;
        if(net_name.find("[") != std::string::npos){
            pin_bits_set.erase(net_name);
        }
        std::vector<unsigned long long> v = it.second;
        outf << "      (" << net_name << endl;
        outf << "         (T0 " << (v[0]?(end - start):0) << ") (T1 " << (v[1]?(end - start):0) << ") (TX " << 0 << ")" << endl;
        outf << "      )" << endl;
    }
}

void DumpSaif(ofstream &outf, std::vector<std::map<std::string, std::vector<unsigned int> > > &OutMaps,
    std::vector<unsigned int*> &out_times,std::vector<short*> &out_values,std::vector<unsigned int*> &out_sizes, std::vector<unsigned int*> &out_starts,
    VCDTime start, VCDTime end){
    unsigned size = OutMaps.size();
    for(unsigned i = 0; i < size; ++i){
        for(auto &it:OutMaps[i]){
            string net_name = it.first;
            if(net_name.find("[") != std::string::npos){
                pin_bits_set.erase(net_name);
            }
            outf << "      (" << net_name << endl;
            std::vector<unsigned int> v = it.second;
            unsigned int cur_level = i;
            unsigned int cur_pos = v[0];
            unsigned int cur_out_order = v[1];
            unsigned int cur_size = *(out_sizes[cur_level] + cur_pos * MAX_OUT_NUM + cur_out_order);
            unsigned int tmp_step = *(out_sizes[cur_level] + cur_pos * MAX_OUT_NUM);
            unsigned int cur_start = *(out_starts[cur_level] + cur_pos) + cur_out_order * tmp_step; 

            unsigned int *cur_time = out_times[cur_level] + cur_start;
            short *cur_value = out_values[cur_level] + cur_start / STATE_NUM;
            short offset = cur_start % STATE_NUM;
            unsigned long long time_1=0;
            unsigned long long time_0=0;
            unsigned long long time_x=0;
            unsigned long long time_z=0;
            unsigned long long last_time = start;
            VCDBit last_state = VCD_X;
            // find real start
            unsigned pos_val_start = 0;
            short start_data = cur_value[pos_val_start];
            short data;
            short shift = (3 << 14);
            int o;
            for(o = 0; o < offset; ++o){
                start_data = (start_data << 2);
            }
            //
            for (unsigned l = 0; l < cur_size; ++l)
            {
                short data = ((start_data & shift) >> 14) & 3;
                ++o;
                start_data = (start_data << 2);
                if(o == STATE_NUM){
                    ++pos_val_start;
                    o = 0;
                    start_data = cur_value[pos_val_start];
                }
                VCDTime cur_t = cur_time[l];
                VCDBit cur_val = VCDBit(data);
                if (cur_t <= start)
                {
                    last_state = cur_val;
                    continue;
                }
                if (cur_t > end)
                {
                    break;
                }
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
            std::vector<unsigned long long> temp = {time_0, time_1, time_x + time_z};
            OtherResult[net_name] = temp;
        }
    }
}

void DumpSaif(std::ofstream &outf, std::unordered_set<std::string> &InMaps_bus, std::unordered_set<std::string> &InMaps_bit, 
    std::unordered_map<std::string, std::vector<unsigned int> > &times, std::unordered_map<std::string, std::vector<short> > &values, 
    std::unordered_map<std::string, unsigned int> &sizes,
    std::vector<std::vector<unsigned int> > &vec_times, std::vector<std::vector<short> > &vec_values, std::vector<unsigned int> &vec_sizes,
    std::unordered_map<std::string, std::string> &hash_name_pair, std::unordered_map<std::string, unsigned int> &hash_index_pair,
    VCDTime start, VCDTime end)
{
    for(auto &it:InMaps_bus){
        if(it.find("[") != std::string::npos){
            pin_bits_set.erase(it);
        }
        unsigned cur_size = sizes[it];
        unsigned int *pos = &(*(times[it].begin()));
        short *pos_val = &(*(values[it].begin()));
        outf << "      (" << it << endl;
        unsigned long long time_1=0;
        unsigned long long time_0=0;
        unsigned long long time_x=0;
        unsigned long long time_z=0;
        unsigned long long last_time = start;
        VCDBit last_state = VCD_X;
        //
        for (unsigned l = 0; l < cur_size; ++l)
        {
            VCDTime cur_t = pos[l];
            VCDBit cur_val = VCDBit(pos_val[l]);
            if (cur_t <= start)
            {
                last_state = cur_val;
                continue;
            }
            if (cur_t > end)
            {
                break;
            }
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
        std::vector<unsigned long long> temp = {time_0, time_1, time_x + time_z};
        OtherResult[it] = temp;
    }
    for(auto &it:InMaps_bit){
        if(it.find("[") != std::string::npos){
            pin_bits_set.erase(it);
        }
        string hash = hash_name_pair[it];
        unsigned _idx = hash_index_pair[hash];
        unsigned cur_size = vec_times[_idx].size();//vec_sizes[_idx];//
        unsigned int *pos = &(*(vec_times[_idx].begin()));
        short *pos_val = &(*(vec_values[_idx].begin()));
        outf << "      (" << it << endl;
        unsigned long long time_1=0;
        unsigned long long time_0=0;
        unsigned long long time_x=0;
        unsigned long long time_z=0;
        unsigned long long last_time = start;
        VCDBit last_state = VCD_X;
        //
        for (unsigned l = 0; l < cur_size; ++l)
        {
            VCDTime cur_t = pos[l];
            VCDBit cur_val = VCDBit(pos_val[l]);
            if (cur_t <= start)
            {
                last_state = cur_val;
                continue;
            }
            if (cur_t > end)
            {
                break;
            }
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
        std::vector<unsigned long long> temp = {time_0, time_1, time_x + time_z};
        OtherResult[it] = temp;
    }
}

void DumpSaif(ofstream &outf, std::map<std::string, std::string> &assign_pairs, 
    //std::unordered_map<std::string, std::vector<unsigned long long> >& inResult, 
    std::map<std::string, std::vector<unsigned long long> >& pinResult,
    VCDTime start, VCDTime end){
    for(auto &it:assign_pairs){
        string net_name = it.first;
        if(Result.find(net_name) != Result.end() || OtherResult.find(net_name) != OtherResult.end() || pinResult.find(net_name) != pinResult.end()){
            continue;
        }
        if(net_name.find("[") != std::string::npos){
            pin_bits_set.erase(net_name);
        }
        string des_name = it.second;
        outf << "      (" << net_name << endl;
        if(Result.find(des_name) != Result.end()){
            std::vector<unsigned long long> v = Result[des_name];
            outf << "         (T0 " << v[0] << ") (T1 " << v[1] << ") (TX " << v[2] << ")" << endl;
        }
        else if(OtherResult.find(des_name) != OtherResult.end()){
            std::vector<unsigned long long> v = OtherResult[des_name];
            outf << "         (T0 " << v[0] << ") (T1 " << v[1] << ") (TX " << v[2] << ")" << endl;
        }
        /*else if(inResult.find(des_name) != inResult.end()){
            std::vector<unsigned long long> v = inResult[des_name];
            outf << "         (T0 " << v[0] << ") (T1 " << v[1] << ") (TX " << v[2] << ")" << endl;
        }*/
        else if(pinResult.find(des_name) != pinResult.end()){
            std::vector<unsigned long long> v = pinResult[des_name];
            outf << "         (T0 " << (v[0]?(end - start):0) << ") (T1 " << (v[1]?(end - start):0) << ") (TX " << v[2] << ")" << endl;
        }
        outf << "      )" << endl;
    }
}

void DumpSaif(ofstream &outf, VCDTime start, VCDTime end)
{
    for(auto &it:pin_bits_set){
        outf << "      (" << it << endl;
        outf << "         (T0 " << 0 << ") (T1 " << 0 << ") (TX " << end - start << ")" << endl;
        outf << "      )" << endl;
    }
    outf << "   )" << endl;
}

void DumpSaif(ofstream &outf, const VCDTimeUnit time_unit, const unsigned time_res, const std::string root_name, VCDTime start, VCDTime end)
{
    unsigned int total_time = end - start;
    if (total_time <= 0)
    {
        cout << "dumpoff_time must be lower than dumpon_time" << endl;
        exit(-1);
    }
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
    outf << "(TIMESCALE " << time_res << " " << tmp_tu << ")" << endl;
    outf << "(DURATION " << total_time << ")" << endl;
    outf << "(INSTANCE " << root_name << endl;
    outf << "   (NET" << endl;
}



//GPU
__global__
void SimulateCuda(int cur_level,int blocks_per_inst, short *dev_datas, short *dev_out_datas, /*int *dev_in_times, int *dev_in_data, */unsigned int *dev_times, unsigned int *dev_out_times,//unsigned int *dev_data_val_num, unsigned int *dev_in_times,
                short *dev_functions, short *dev_delay_edges, short *dev_in_bit, short *dev_out_bit, float *dev_rise_val, float *dev_fall_val,
                int dev_time_unit, unsigned int *dev_total_times_start, unsigned int *dev_total_times_sizes, short *dev_data_in_num, short *dev_data_out_num, short *dev_delay_val_num,short *dev_functions_func_num, short *dev_functions_val_num, 
                unsigned int *dev_input_start, //unsigned int *dev_val_num_start,
                //unsigned int *dev_valid_width,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, unsigned int *dev_data_in_num_start,
                unsigned int *dev_data_out_num_start, unsigned int *dev_delay_start, unsigned int *dev_delay_width, unsigned int *dev_functions_start, unsigned int *dev_functions_width,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size)//, int *dev_data_start)
{
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    
    if (_tid < N_THREADS_PER_BLOCK)
    {
        const unsigned int bid = _bid / blocks_per_inst;
        const unsigned int tid = _tid + (_bid % blocks_per_inst) * N_THREADS_PER_BLOCK;
        //const int inval_start = bid*18;
        //const int lastval_start = bid*6;
        //times flag
        const unsigned int time_width = dev_total_times_sizes[bid];
        const unsigned int time_start = dev_total_times_start[bid];
        //int time_start;        
        //__shared__ int dev_in_times[12288];
        //printf("tid:%d, time_width:%d, time_start:%d\n", tid, time_width, time_start);
        const unsigned int _start = dev_data_in_num_start[cur_level];
        //in data flag
        const short in_num = dev_data_in_num[_start+bid];
        //if(_tid == 0) printf("_start:%d\n", _start);
        //out data flag
        const short out_num = dev_data_out_num[_start+bid];
        //delay flag
        const short delay_width = dev_delay_val_num[_start+bid];
        //function flag
        const short function_num = dev_functions_func_num[_start+bid];
        //if (bid == 13 && inst_num == 1822)
        //    printf("funcnum: %hd, global_func_width: %d\n", function_num,global_func_width);
        const short width_every_function = dev_functions_val_num[_start+bid];
        //begin point
        //unsigned long long val_num_start = bid*global_val_num_width; //in datas and in times
        const unsigned int out_start = dev_output_start[bid];//bid*global_out_width;
        const unsigned int out_width_every = dev_output_size[bid];//global_out_width/out_num;
        const unsigned int valid_width_start = dev_valid_width_start[bid];//global_out_width/out_num;
        const unsigned int valid_width_size = dev_valid_width_size[bid];//global_out_width/out_num;
        const unsigned int in_start = dev_input_start[bid];
        //const int time_start = dev_time_start[bid];
        //const unsigned int val_start = dev_val_num_start[bid];

        const int func_start = dev_functions_start[cur_level]+bid*dev_functions_width[cur_level];//bid*global_func_width;
        const int func_width_every = dev_functions_width[cur_level]/function_num;
        const int delay_start = dev_delay_start[cur_level]+bid*dev_delay_width[cur_level];//bid*global_delay_width;
        //if(_tid == 0) printf("func_start:%d, %d\n", func_start,delay_start);
        // calculate
        //printf("%d start calculate\n", tid);
        short _cur_inValues[18];
        short _last_inValues[6];
        //int last_vec[6] = {2,2,2,2,2,2};
        unsigned int left, left_start, left_shift, right, mid;
        unsigned int cur_time, out_time, last_time, _last_time;
        //short cur_out, pre_out, output_data, last_value, cur, last;
        short cur_out, pre_out, output_data, last_value, cur, last;
        unsigned int i,j,k,l;
        short func_flag;
        short input_data, _input_data, input_data1;
        //__shared__ int _time_width[N_THREADS_PER_BLOCK];
        short cur_func;
        short pos, out_sense, in_sense, num_in, changed_id, edge;
        int func_in_num;
        float delay_val, factor, min_val;
        int delay_valid_id1, delay_valid_id2, delay_valid_id;  //one input driving
        unsigned int cur_start, last_start;

        short splited_time_width;
        if (time_width > N_THREADS_PER_BLOCK*N_TIMES_PER_THREAD*blocks_per_inst)
        {
            splited_time_width = time_width / (N_THREADS_PER_BLOCK*blocks_per_inst) + 1;
        }
        else
            splited_time_width = N_TIMES_PER_THREAD;
        dev_out_splited_width[bid] = splited_time_width;// + 1;
        for (i = 0; i < out_num; ++i)
        {
            if (tid*splited_time_width >= time_width)
                break;
            //i :out id
            short flag = 0;
            unsigned int end_flag = tid*(splited_time_width);//+1);
            short *_dev_out_datas = dev_out_datas + out_start + i * out_width_every;
            unsigned int *_dev_out_times = dev_out_times + out_start + i * out_width_every;
            unsigned int *_dev_valid_width = dev_valid_width + valid_width_start + i * valid_width_size;
            //_dev_out_datas[end_flag] = 1;  //VCD_X
            //_dev_out_times[end_flag] = 0;
            if(tid == 0)
            {
                _dev_out_datas[end_flag] = 1;  //VCD_X
                _dev_out_times[end_flag] = 0;
                ++end_flag;
                pre_out = 1;   //VCD_X;
                last_time = 0;
                _last_time = 0;
            }
            //printf("%d -- out:%d start simulate\n", tid, i);
            for (j = time_start+tid*splited_time_width; j < time_start+(tid+1)*splited_time_width; ++j)
            {
                if (j == time_start+time_width)
                {
                    break;
                }
                if (j == time_start && tid == 0)
                {
                    continue;
                }
                if (j == time_start + tid*splited_time_width && tid != 0 && !flag)
                {
                    j--;
                    flag = 1;
                    //if (bid == 299 && inst_num == 355 && tid < 13 && tid > 10)
                    //    printf("cur_time=%d -- %d\n", dev_times[j], tid);
                }
                //cur_time = dev_times[j];
                cur_time = dev_times[j];               
                
                if (!(j == time_start + tid*splited_time_width - 1 && tid != 0 && flag) && cur_time == _last_time)
                {
                    continue;
                }
                _last_time = cur_time;
                // GetValue at cur_time
                cur_start = in_start+in_num*(j - time_start);
                last_start = in_start+in_num*(j - time_start - 1);
                short *_dev_datas = dev_datas + cur_start;
                short *_l_dev_datas = dev_datas + last_start;
                
                for (k = 0; k < in_num; ++k)
                {
                    _cur_inValues[k] = _dev_datas[k];
                    _last_inValues[k] = _l_dev_datas[k];
                    //if (bid == 299 && tid == 1)
                    //    printf("\tinput %d: %hd -- %d\n", k, _cur_inValues[k], j);
                }
                //if (cur_level == 53 && bid == 0)
                //    printf("time: %u val:%hd, %hd, %hd, %hd(%u) -- %d\n", cur_time, _cur_inValues[0], _cur_inValues[1], _cur_inValues[2], _cur_inValues[3], j, tid); 
                
                // Run Function
                cur_out = 1;    //VCD_X;
                //short *inter_values = new short[(width_every_function - in_num - out_num)*function_num];//(short *)malloc(sizeof(short)*(width_every_function - in_num - out_num)*function_num);
                
                //printf("%d -- (%d)t=%d start run func\n", tid, j, cur_time);
                for (k = 0; k < function_num; ++k)
                {
                    short *_dev_functions = dev_functions + func_start + k * func_width_every;
                    cur_func = _dev_functions[0]; // func_id
                    // input_data
                    func_in_num = 0;
                    for (l = 1; l < width_every_function; ++l)
                    {
                        func_in_num++;
                        if(_dev_functions[l] == -1)
                        {
                            func_in_num = l-1;
                            break;
                        }
                    }
                    func_in_num--;
                    func_flag = _dev_functions[func_in_num+1];
                    //short *input_data = new short[func_in_num];//(short *)malloc(sizeof(short)*func_in_num);
                    pos = _dev_functions[1];
                    //input_data = cur_inValues[inval_start+pos];
                    input_data = _cur_inValues[pos];
                    for (l = 1; l < func_in_num; ++l)
                    {
                        if (cur_func == 9 && l == 2) // udp_mux2
                        {
                            _input_data = input_data1;
                            
                        }
                        pos = _dev_functions[l+1];
                        //input_data1 = cur_inValues[inval_start+pos];
                        input_data1 = _cur_inValues[pos];
                        
                        if (cur_func == 0 || cur_func == 5)  // and
                        {
                            input_data = input_data & input_data1;
                        }
                    
                        else if (cur_func == 1 || cur_func == 4)
                        {
                            input_data = input_data | input_data1;
                        }
                        else if (cur_func == 2 || cur_func == 3)
                        {
                            if (input_data == 1 || input_data1 == 1)
                            {
                                input_data = 1;
                            }
                            else
                                input_data = input_data ^ input_data1;
                        }
                        else if (cur_func == 9 && l == 2)
                        {
                            if(input_data1 != 1)
                                input_data = (input_data1 & _input_data) | ((input_data1  ^ 3) & input_data);
                            else if (_input_data == input_data)
                                input_data = input_data;
                            else
                                input_data = 1;
                        }
                    }
                    if (cur_func == 3 || cur_func == 4 || cur_func == 5 || cur_func == 7)
                    {
                        output_data = input_data ^ 3;   //VCD_1
                    }
                    else
                        output_data = input_data;
                    if (output_data != 0 && output_data != 3 && output_data != 1)
                    {
                        output_data = 1;   //VCD_X
                    }
                    if (func_flag == in_num+i)
                    {
                        cur_out = output_data;
                        break;
                    }
                    _cur_inValues[func_flag] = output_data;
                }
                
                if (j == time_start + tid*splited_time_width-1 && tid != 0 && flag)
                {
                    pre_out = cur_out;
                    //_dev_out_datas[end_flag] = cur_out;
                    last_time = 0;
                    //atomicExch(&dev_out_datas[out_start+i*(global_out_width/out_num)+end_flag], cur_out);
                    continue;
                }
                //printf("%d -- t=%ld start CheckDelayCuda\n", tid, cur_time);
                out_sense = CheckDelayCuda(pre_out, cur_out);
                //printf("%d -- t=%ld success CheckDelayCuda\n", tid, cur_time);
                if (out_sense == 0)
                {
                    continue;
                }
                // add delay
                num_in = 0;
                for (k = 0; k < in_num; ++k)
                {
                    //if (dev_in_datas[in_start+(j - time_start)*in_num+k] != dev_in_datas[in_start+(j -1 - time_start)*in_num+k])
                    //if(cur_inValues[inval_start+k] != last_inValues[lastval_start+k])
                    if(_cur_inValues[k] != _last_inValues[k])
                    {
                        num_in++;
                        changed_id = k;
                    }
                }
                //printf("%d --- t=%ld, delay driving num: %d\n", tid, cur_time, num_in);
                delay_val = 1.0;
                factor = 1.0;
               // printf("%d -- t=%ld start add delay\n", tid, cur_time);
                // only one input driving out
                if (num_in == 1) 
                {
                    delay_valid_id = -1;
                    delay_valid_id1 = -1;
                    delay_valid_id2 = -1;
                    for (k = delay_start; k < delay_start+delay_width; ++k)
                    {
                        if (dev_in_bit[k] == changed_id && dev_delay_edges[k] == 2 && dev_out_bit[k] == i)
                        {
                            delay_valid_id = k;
                            break;
                        }
                        else if (dev_in_bit[k] == changed_id && delay_valid_id1 == -1 && delay_valid_id2 == -1 && dev_out_bit[k] == i)
                        {
                            delay_valid_id1 = k;
                        }
                        else if (dev_in_bit[k] == changed_id && delay_valid_id1 != -1 && delay_valid_id2 == -1 && dev_out_bit[k] == i)
                        {
                            delay_valid_id2 = k;
                        }
                    }
                    if (delay_valid_id == -1)
                    {
                        in_sense = CheckDelayCuda(_last_inValues[changed_id], _cur_inValues[changed_id]);
                        
                        edge = 0;
                        if(in_sense == 1)
                            edge = 0;
                        else
                            edge = 1;
                        if (dev_delay_edges[delay_valid_id1] == edge)
                        {
                            delay_valid_id = delay_valid_id1;
                        }
                        else
                        {
                            delay_valid_id = delay_valid_id2;
                        }
                        
                    }
                    switch(out_sense)
                    {
                        case(1):
                            delay_val = dev_rise_val[delay_valid_id];
                            break;
                        case(2):
                            delay_val = dev_fall_val[delay_valid_id];
                            break;
                        //case(3):
                        //    delay_val = max(dev_rise_val[delay_valid_id], dev_fall_val[delay_valid_id]);
                        //    break;
                        //case(4):
                        //    delay_val = min(dev_rise_val[delay_valid_id], dev_fall_val[delay_valid_id]);
                        //    break;
                        default:
                            delay_val = 1.0;
                            break;
                    }
                }
                // multiple inputs driving out
                else
                {
                    min_val = 100000.0;
                    for (k = 0; k < in_num; ++k)
                    {
                        if(_cur_inValues[k] == _last_inValues[k]) 
                            continue;
                        cur = _cur_inValues[k];
                        last = _last_inValues[k];
                        for (l = delay_start; l < delay_start+delay_width; ++l)
                        {
                            if (dev_in_bit[l] == k && dev_out_bit[l] == i)
                            {
                                if (dev_delay_edges[l] != 2)
                                {
                                    //in_sense = CheckDelayCuda(dev_in_datas[in_start+(j - time_start-1)*in_num+k], dev_in_datas[in_start+(j - time_start)*in_num+k]);
                                    in_sense = CheckDelayCuda(last, cur);
                                    edge = 0;
                                    if(in_sense == 1)
                                        edge = 0;
                                    else
                                        edge = 1;
                                    if (edge != dev_delay_edges[l])
                                    {
                                        continue;
                                    }
                                }
                                switch(out_sense)
                                {
                                    case(1):
                                        min_val = min(dev_rise_val[l], min_val);
                                        break;
                                    case(2):
                                        min_val = min(dev_fall_val[l], min_val);
                                        break;
                                    //case(3):
                                    //    min_val = min(max(dev_rise_val[l], dev_fall_val[l]), min_val);
                                    //    break;
                                    //case(4):
                                    //    min_val = min(min(dev_rise_val[l], dev_fall_val[l]), min_val);
                                    //    break;
                                    default:
                                        min_val = 1.0;
                                        break;
                                }
                            }
                        }
                    }
                    delay_val = (min_val > 99999.0)?1.0:min_val;
                }
                //printf("%d -- t=%ld success add delay\n", tid, cur_time);
                // check time_unit
                //printf("%d -- t=%ld start check time unit\n", tid, cur_time);
                //if(dev_time_unit == 0)
                //    factor = 1e-12;
                //else if(dev_time_unit == 1)
                //    factor = 1e-9;
                //else if(dev_time_unit == 2)
                //    factor = 1e-6;
                //else if(dev_time_unit == 3)
                //    factor = 1e-3;
                //else if(dev_time_unit == 4)
                //    factor = 1.0;
                out_time = cur_time + static_cast<int>(delay_val);//*factor);
                last_value = pre_out;//dev_out_datas[out_start+i*out_width_every+end_flag];
                bool _flag = false;
                //if (cur_level == 0 && bid == 2215)
                //    printf("\ttime: %u val: %hd (last=%u, cur=%u,end_flag=%d) -- %d\n", out_time, cur_out, last_time, cur_time, end_flag, tid);                
                while(last_time >= out_time)
                {
                    //if ((end_flag == tid*(splited_time_width+1)+1 && tid != 0) || end_flag == 0)
                    if ((end_flag == tid*(splited_time_width)+1) || end_flag == 0)
                    {
                        if(end_flag != 0){
                            --end_flag;
                        }
                        _dev_out_datas[end_flag] = -1;
                        _dev_out_times[end_flag] = 0;
                        //end_flag--;
                        _flag = true;
                        break;
                    }
                    --end_flag;
                    _dev_out_datas[end_flag] = -1;
                    _dev_out_times[end_flag] = 0;
                    //atomicExch(&dev_out_datas[out_start+i*out_width_every+end_flag], -1);
                    //atomicExch(&dev_out_times[out_start+i*out_width_every+end_flag], 0);
                    //end_flag--;
                    last_time = _dev_out_times[end_flag - 1];
                    last_value = _dev_out_datas[end_flag - 1];
                    pre_out = last_value;
                }
                //printf("%d -- t=%lld, last=%hd, cur=%hd\n", tid, out_time, last_value, cur_out);
                if (_flag || (last_value != cur_out))
                {
                    //end_flag++;
                    _dev_out_times[end_flag] = out_time;
                    _dev_out_datas[end_flag] = cur_out;
                    last_time = out_time;
                    pre_out = cur_out;
                    ++end_flag;
                }
            }
            _dev_valid_width[tid] = end_flag - tid * splited_time_width;
        }
    }
}

__device__
short CheckDelayCuda(short pre, short cur)
{
    /** delay value --- 1:rise 2:fall 3:max(rise, fall) 4:min(rise, fall) 0:unchanged **/
    if ((pre == 0 && cur != 0) || (cur == 3 && pre != 3))
    {
        return 1;
    }
    else if ((pre == 3 && cur != 3) || (cur == 0 && pre != 0))
    {
        return 2;
    }
    //else if (pre == 2 && cur == 3)
    //{
    //    return 3;
    //}
    //else if (pre == 3 && cur == 2)
    //{
    //    return 4;
    //}
    else
    {
        return 0;
    }   
}

__global__
void DecompressTimesGPU(unsigned int *dev_times, unsigned short *dev_times_compressed, 
    unsigned int *dev_base_start, unsigned int *dev_base_start_start, 
    const unsigned int base_start_total, const unsigned int base_start_start_idx, const unsigned int total_times_sizes)
{
    //const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    unsigned int bid_x = blockIdx.y;
    unsigned int bid_y = blockIdx.x;
    const unsigned int tid = threadIdx.x;
    if(base_start_start_idx > MAX_BLOCKS){
        if(bid_x < base_start_total){
            bid_y = bid_y * 2;
        }
        else{
            bid_y = bid_y * 2 + 1;
            bid_x = bid_x - base_start_total;
        }
        //bid_y = (base_start_start_idx + MAX_BLOCKS - 1) / MAX_BLOCKS * bid_y;
    }
    if(bid_y >= base_start_start_idx){
        return;
    }
    unsigned int base_start_start_val = dev_base_start_start[bid_y];
    unsigned int *_dev_base_start = dev_base_start + base_start_start_val;
    unsigned int tile_num;
    if(bid_y < base_start_start_idx - 1){
        tile_num = dev_base_start_start[bid_y + 1] - base_start_start_val;
    }
    else{
        tile_num = base_start_total - base_start_start_val;
    }
    if(bid_x >= tile_num){
        return;
    }
    unsigned times_start = _dev_base_start[bid_x];
    if(times_start == 0 && base_start_start_val + bid_x > 0){
        return;
    }
    unsigned base = bid_x;
    unsigned int *_dev_times = dev_times + times_start;
    unsigned short *_dev_times_compressed = dev_times_compressed + times_start;
    //unsigned int *_dev_times_compressed = dev_times_compressed + times_start;
    unsigned times_size;
    if(bid_y == base_start_start_idx - 1 && bid_x == tile_num - 1){
        times_size = total_times_sizes - times_start;
    }
    else{
        bool flag = false;
        while(_dev_base_start[bid_x + 1] == 0){
            ++bid_x;
            if(bid_y == base_start_start_idx - 1 && bid_x == tile_num - 1){
                flag = true;
                break;
            }
        }
        if(flag){
            times_size = total_times_sizes - times_start;
        }
        else{
            times_size = _dev_base_start[bid_x + 1] - times_start;
        }
    }
    const unsigned run_width = (times_size + N_THREADS_PER_BLOCK - 1) / N_THREADS_PER_BLOCK;
    if(tid * run_width >= times_size){
        return;
    }
    for(unsigned i = tid * run_width; i < times_size && i < (tid + 1) * run_width; ++i){
        _dev_times[i] = static_cast<unsigned int>(_dev_times_compressed[i]) + COMPRESS_BASE * base;
    }
}

__global__
void DecompressGPU(short *dev_in_datas, short *dev_in_data_compressed, 
    unsigned int *dev_step_start_vec, unsigned int *dev_com_step_start_vec, 
    unsigned int *dev_data_val_num, unsigned int blocks_per_signal, unsigned int sigal_num)
{
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    const unsigned int bid = _bid / blocks_per_signal;
    if(bid >= sigal_num)
    {
        return;
    }
    const unsigned int tid = _tid + (_bid % blocks_per_signal) * N_THREADS_PER_BLOCK;
    const unsigned int data_start = dev_step_start_vec[bid];
    const unsigned int com_data_start = dev_com_step_start_vec[bid];
    const unsigned int data_size = dev_data_val_num[bid];
    const unsigned int com_data_size = (data_size + STATE_NUM - 1) / STATE_NUM;
    const short shift = (3 << 14);
    //const unsigned int tid = _tid + _bid * N_THREADS_PER_BLOCK;
    //if (tid < data_width)
    //{
    //    dev_in_datas[tid] = static_cast<short>(dev_in_data_compressed[tid]);
    //}
    short *_dev_in_data_compressed = dev_in_data_compressed + com_data_start;
    short *_dev_in_datas = dev_in_datas + data_start;
    if (tid < com_data_size)
    {
        short data = _dev_in_data_compressed[tid];
        for(int i = 0; i < STATE_NUM; ++i)
        {
            short cur_data = ((data & shift) >> 14) & 3;
            if(cur_data == UNKNOWN)
            {
                break;
            }
            _dev_in_datas[tid * STATE_NUM + i] = cur_data;
            data = (data << 2);
        }
    }
}

__global__
void CompressGPU(short *dev_in_datas, short *dev_in_data_compressed,
    unsigned int data_width, unsigned int data_width_compress)
{
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    const unsigned int tid = _tid + _bid * N_THREADS_PER_BLOCK;
    if (tid < data_width_compress)
    {
        const short splited_time_width = COMPRESS_WIDTH;
        int j;//, j_com;
        short temp = 0;
        //bool flag = false;
        //if(tid * splited_time_width >= data_width || tid * (splited_time_width / STATE_NUM) >= data_width_compress){
        //    return;
        //}
        short *_dev_in_datas = dev_in_datas + tid * splited_time_width;
        //short *_dev_in_data_compressed = dev_in_data_compressed + tid * (splited_time_width / STATE_NUM);
        for(j = 0; j < splited_time_width; ++j){
            short data = _dev_in_datas[j];
            if(data == -1 || j + tid * splited_time_width >= data_width){
                temp = (temp << 2);
                temp += UNKNOWN;   //unknown
            }
            else{
                temp = (temp << 2);
                temp += data;
            }
        }
        dev_in_data_compressed[tid] = temp;
        /*for (j = 0, j_com = 0; 
            j_com < (splited_time_width / STATE_NUM) && j < splited_time_width; 
            ++j_com)
        {
            if(j_com + tid * (splited_time_width / STATE_NUM) == data_width_compress){
                break;
            }
            temp = 0;
            for(int k = 0; k < STATE_NUM; ++k, ++j){
                data = _dev_in_datas[j];
                if(j + tid * splited_time_width >= data_width || data == -1){
                    temp = (temp << 2);
                    temp += UNKNOWN;   //unknown
                }
                else{
                    temp = (temp << 2);
                    temp += data;
                }
            }
            _dev_in_data_compressed[j_com] = temp;
        }*/
    }
}

__global__
void ProcessConflictCuda(int cur_level,int blocks_per_inst, short *dev_out_datas, unsigned int *dev_out_times,
                int dev_time_unit, short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size, 
                short *dev_valid_width_flag)
{
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    if (_tid < N_THREADS_PER_BLOCK)
    {
        const unsigned int bid = _bid / blocks_per_inst;
        const unsigned int tid = _tid + (_bid % blocks_per_inst) * N_THREADS_PER_BLOCK;
        //__shared__ int dev_in_times[12288];
        //printf("tid:%d, time_width:%d, time_start:%d\n", tid, time_width, time_start);
        const unsigned int _start = dev_data_in_num_start[cur_level];
        //out data flag
        const short out_num = dev_data_out_num[_start+bid];
        //begin point
        const unsigned int out_start = dev_output_start[bid];//bid*global_out_width;
        const unsigned int out_width_every = dev_output_size[bid];//global_out_width/out_num;
        const unsigned int valid_width_start = dev_valid_width_start[bid];//bid*global_out_width;
        const unsigned int valid_width_size = dev_valid_width_size[bid];//global_out_width/out_num;
        //printf("%d start calculate\n", tid);
        unsigned int end_flag, flag;
        unsigned int cur_time, last_time;
        unsigned int i,j,k,l;
        short cur_data, last_data;
        //__shared__ int _time_width[N_THREADS_PER_BLOCK];

        short splited_time_width = dev_out_splited_width[bid];
        if (tid == 0 || tid*splited_time_width >= out_width_every)
            return;
        for (i = 0; i < out_num; ++i)
        {
            //i :out id
            short *_dev_out_datas = dev_out_datas + out_start + i * out_width_every;
            unsigned int *_dev_out_times = dev_out_times + out_start + i * out_width_every;
            unsigned int *_dev_valid_width = dev_valid_width + valid_width_start + i * valid_width_size;
            short *_dev_valid_width_flag = dev_valid_width_flag + valid_width_start + i * valid_width_size;
            //set 0
            //_dev_out_datas[tid * splited_time_width] = -1;
            //_dev_out_times[tid * splited_time_width] = 0;
            last_time = 0;
            last_data = 1;
            for(j = (tid - 1) * splited_time_width; j < tid * splited_time_width && _dev_out_datas[j] != -1; ++j)
            {
                last_time = _dev_out_times[j];
                last_data = _dev_out_datas[j];
            }
            end_flag = j - 1;
            if (end_flag == (tid - 1) * splited_time_width - 1)
            {
                continue;
            }
            
            
            //
            flag = tid * splited_time_width;// + 1;
            cur_time = _dev_out_times[flag];
            cur_data = _dev_out_datas[flag];
            
            if (cur_data == -1 || cur_time == 0)
            {
                continue;
            }
            //last_time = _dev_out_times[end_flag];
            //if (bid < 10)
            //{
            //    printf("end_flag:%d, last_time:%u, cur_time:%u -- %d, %d\n", end_flag, last_time, cur_time, tid, bid);
            //}
            //if (cur_level ==30 && bid == 0 && cur_time > 2876754000 && cur_time < 2876754600 && i == 1)
            //    printf("end=%u, last=%u(%hd), cur=%u(%hd) -- %u\n",end_flag,last_time,_dev_out_datas[end_flag], cur_time,cur_data, tid);
            bool _flag = false;
            while(last_time >= cur_time)
            {
                if (end_flag == (tid-1)*splited_time_width)//+1) && tid > 1)
                {
                    _dev_out_datas[end_flag] = -1;
                    _dev_out_times[end_flag] = 0;
                    end_flag--;
                    --_dev_valid_width[tid - 1];
                    _flag = true;
                    break;
                }
                //if (cur_level ==28 && bid == 0 && tid == 69632 && i == 1)
                //    printf("end=%u, last=%u(%hd), cur=%u(%hd) -- %d\n",end_flag,last_time,_dev_out_datas[end_flag], cur_time,cur_data, tid);
                _dev_out_datas[end_flag] = -1;
                _dev_out_times[end_flag] = 0;
                end_flag--;
                --_dev_valid_width[tid - 1];
                last_time = _dev_out_times[end_flag];        
                last_data = _dev_out_datas[end_flag];        
            }
            if (cur_data == last_data && _flag == false)
            {
                _dev_out_datas[flag] = -1;
                _dev_out_times[flag] = 0;
                _dev_valid_width_flag[tid] = 1;
            }
        }
    }
}

__global__
void UpdateValidWidthGPU(int cur_level,int blocks_per_inst, short *dev_out_datas,
                short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size, 
                short *dev_valid_width_flag)
{
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    if (_tid < N_THREADS_PER_BLOCK)
    {
        const unsigned int bid = _bid / blocks_per_inst;
        const unsigned int tid = _tid + (_bid % blocks_per_inst) * N_THREADS_PER_BLOCK;
        //__shared__ int dev_in_times[12288];
        //printf("tid:%d, time_width:%d, time_start:%d\n", tid, time_width, time_start);
        const unsigned int _start = dev_data_in_num_start[cur_level];
        //out data flag
        const short out_num = dev_data_out_num[_start+bid];
        //begin point
        const unsigned int out_start = dev_output_start[bid];//bid*global_out_width;
        const unsigned int out_width_every = dev_output_size[bid];//global_out_width/out_num;
        const unsigned int valid_width_start = dev_valid_width_start[bid];//bid*global_out_width;
        const unsigned int valid_width_size = dev_valid_width_size[bid];//global_out_width/out_num;
        //printf("%d start calculate\n", tid);
        unsigned int i;
        //__shared__ int _time_width[N_THREADS_PER_BLOCK];

        short splited_time_width = dev_out_splited_width[bid];
        if (tid == 0 || tid*splited_time_width >= out_width_every)
            return;
        for (i = 0; i < out_num; ++i)
        {
            //i :out id
            short *_dev_out_datas = dev_out_datas + out_start + i * out_width_every;
            unsigned int *_dev_valid_width = dev_valid_width + valid_width_start + i * valid_width_size;
            short *_dev_valid_width_flag = dev_valid_width_flag + valid_width_start + i * valid_width_size;

            unsigned int flag = tid * splited_time_width;// + 1;
            if(_dev_valid_width_flag[tid] == 1){
                --_dev_valid_width[tid];
            }
            
        }
    }
}

__global__
void UpdateOutInfoGPU(unsigned int *dev_prefix_sum_of_valid_width, int cur_level,int inst_num,
                short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size,
                unsigned int *dev_out_width)
{
    const unsigned int tid = threadIdx.x + blockIdx.x * blockDim.x;
    if (tid < inst_num)
    {
        const unsigned int _start = dev_data_in_num_start[cur_level];
        //out data flag
        const short out_num = dev_data_out_num[_start+tid];
        //begin point
        const unsigned int valid_width_start = dev_valid_width_start[tid];//bid*global_out_width;
        
        const unsigned int valid_width_size = dev_valid_width_size[tid];//global_out_width/out_num;
        //printf("%d start calculate\n", tid);
        //__shared__ int _time_width[N_THREADS_PER_BLOCK];

        unsigned int *_dev_prefix_sum_of_valid_width = dev_prefix_sum_of_valid_width + valid_width_start;
        unsigned int start = _dev_prefix_sum_of_valid_width[0];
        dev_output_start[tid] = start;
        for (int i = 0; i < out_num; ++i)
        {
            if(tid == inst_num - 1 && i == out_num - 1){
                unsigned int *_dev_valid_width = dev_valid_width + valid_width_start;
                *dev_out_width = _dev_prefix_sum_of_valid_width[valid_width_size*(i+1) - 1] + _dev_valid_width[valid_width_size*(i+1) - 1];
                dev_output_size[tid * MAX_OUT_NUM + i] = *dev_out_width - start;
            }
            else{
                dev_output_size[tid * MAX_OUT_NUM + i] = _dev_prefix_sum_of_valid_width[valid_width_size*(i+1)] - start;
                start = _dev_prefix_sum_of_valid_width[valid_width_size];
            }
        }
        
    }
}

__global__
void RemoveInvalidStatesGPU(unsigned int *dev_out_times_new, short *dev_out_datas_new,
                int cur_level, int blocks_per_inst, short *dev_out_datas, unsigned int *dev_out_times,
                short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size,
                unsigned int *dev_prefix_sum_of_valid_width)
{

    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    if (_tid < N_THREADS_PER_BLOCK)
    {
        const unsigned int bid = _bid / blocks_per_inst;
        const unsigned int tid = _tid + (_bid % blocks_per_inst) * N_THREADS_PER_BLOCK;
        //__shared__ int dev_in_times[12288];
        //printf("tid:%d, time_width:%d, time_start:%d\n", tid, time_width, time_start);
        const unsigned int _start = dev_data_in_num_start[cur_level];
        //out data flag
        const short out_num = dev_data_out_num[_start+bid];
        //begin point
        const unsigned int out_start = dev_output_start[bid];//bid*global_out_width;
        const unsigned int out_width_every = dev_output_size[bid];//global_out_width/out_num;

        const unsigned int valid_width_start = dev_valid_width_start[bid];//bid*global_out_width;
        const unsigned int valid_width_size = dev_valid_width_size[bid];//global_out_width/out_num;
        //printf("%d start calculate\n", tid);
        //__shared__ int _time_width[N_THREADS_PER_BLOCK];

        short splited_time_width = dev_out_splited_width[bid];
        if (tid*splited_time_width >= out_width_every)
            return;
        for (int i = 0; i < out_num; ++i)
        {
            //i :out id
            short *_dev_out_datas = dev_out_datas + out_start + i * out_width_every;
            unsigned int *_dev_out_times = dev_out_times + out_start + i * out_width_every;
            

            unsigned int *_dev_valid_width = dev_valid_width + valid_width_start + i * valid_width_size;
            unsigned int *_dev_prefix_sum_of_valid_width = dev_prefix_sum_of_valid_width + valid_width_start + i * valid_width_size;
            unsigned int _valid_width = _dev_valid_width[tid];
            unsigned int _valid_start = _dev_prefix_sum_of_valid_width[tid];
            short *_dev_out_datas_new = dev_out_datas_new + _valid_start;
            unsigned int *_dev_out_times_new = dev_out_times_new + _valid_start;

            for (unsigned j = tid*splited_time_width, k = 0; k < _valid_width && j < (tid+1)*splited_time_width; ++j){
                if(j == out_width_every){
                    break;
                }
                if(_dev_out_datas[j] == -1){
                    continue;
                }
                _dev_out_times_new[k] = _dev_out_times[j];
                _dev_out_datas_new[k] = _dev_out_datas[j];
                ++k;
            }            
        }
    }

}

void checkError(cudaError_t error, std::string msg) {
    if (error != cudaSuccess) {
        printf("%s: %d\n", msg.c_str(), error);
        cout << cudaGetErrorString(error) << endl;
        exit(-1);
    }
}

__device__ int cu_abs(int i, int j)
{
    if (i <= j){
        return (j - i) >> 1;
    }
    else{
        return (i - j) >> 1;
    }
}
__global__ void PreCudaMerge(unsigned int j, unsigned int *dev_times, short *dev_datas, 
    short *dev_in_datas, int _blocks_per_inst, int cur_level, int inst_id_start, int inst_num_start,int inst_num_end, unsigned int *dev_total_times_start,
    unsigned int *dev_input_start, unsigned int *dev_data_val_num, unsigned int *dev_val_num_start, unsigned int *dev_data_in_num_start, short *dev_data_in_num,
    unsigned int *dev_temp_times, short *dev_temp_datas) {
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    const unsigned int bid = _bid / _blocks_per_inst + inst_id_start;
    if (bid >= inst_num_end || bid < inst_num_start)
    {
        return;
    }
    
    const unsigned int tid = _tid + (_bid % _blocks_per_inst) * _N_THREADS_PER_BLOCK;
    
    const unsigned int _start = dev_data_in_num_start[cur_level];
    const short in_num = dev_data_in_num[_start+bid];
    const unsigned int val_num_start = dev_val_num_start[bid];
    const unsigned int A_start = dev_total_times_start[bid];
    const unsigned int A_data_start = dev_input_start[bid];
    unsigned length_A = dev_data_val_num[val_num_start];
    //if(cur_level == 0 && bid == 2215 && tid == 0){
    //    printf("t=%u\n", *(dev_times + A_start + length_A / 2));
    //}
    for (int i = 1; i < in_num; ++i)
    {
        if (i < j)
        {
            length_A += dev_data_val_num[val_num_start+i];
        }
        //total_length += dev_data_val_num[val_num_start+i];
    }
    const unsigned int B_start = A_start + length_A;
    short *A_datas = dev_in_datas + A_start;// - dev_total_times_start[inst_num_start];
    short *D_datas = dev_datas + A_data_start;
    if (in_num == 1 && j == 1 && tid < length_A)
    {
        D_datas[tid] = A_datas[tid];
    }
    unsigned length_B = dev_data_val_num[val_num_start+j];
    if (j < in_num && tid < length_A + length_B)
    {
        unsigned int *A = dev_times + A_start;
        unsigned int *B = dev_times + B_start;
        
        short *B_datas = dev_in_datas + B_start;// - dev_total_times_start[inst_num_start];
        short *temp_datas = dev_temp_datas + A_data_start - dev_input_start[inst_num_start];
        unsigned int *temp_times = dev_temp_times + A_start - dev_total_times_start[inst_num_start];
        //if(cur_level == 1 && _bid == 0 && _tid < 3)
        //    printf("initial done!\n");
        duo K, P, Q;
        if (tid > length_A) {
            K.x = tid - length_A;
            K.y = length_A;
            P.x = length_A;
            P.y = tid - length_A;
        }else {
            K.x = 0;
            K.y = tid;
            P.x = tid;
            P.y = 0;
        }
        while (true) {
            int offset = cu_abs(K.y, P.y);
            Q.x = K.x + offset;
            Q.y = K.y - offset;
            if (Q.y >= 0 && Q.x <= length_B && (Q.y == length_A || Q.x == 0 || A[Q.y] > B[Q.x - 1])){
                if (Q.x == length_B || Q.y == 0 || A[Q.y - 1] <= B[Q.x]) {
                    if (Q.y < length_A && (Q.x == length_B || A[Q.y] <= B[Q.x])){
                        temp_times[tid] = A[Q.y];
                        //store data
                        if (j == 1)
                        {
                            D_datas[tid*2] = A_datas[Q.y];
                        }
                        else
                        {
                            for (int i = 0; i < j; ++i)
                            {
                                //dev_datas[A_data_start+tid*(j+1)+i] = dev_temp_datas[A_data_start+Q.y*j+i];
                                D_datas[tid*(j+1)+i] = temp_datas[Q.y*j+i];
                            }
                        }
                        if (A[Q.y] == B[Q.x]){
                            //dev_datas[A_data_start+tid*(j+1)+j] = dev_in_datas[B_start+Q.x];
                            D_datas[tid*(j+1)+j] = B_datas[Q.x];
                        }
                        else if(Q.x == 0){
                            D_datas[tid*(j+1)+j] = 1;   //VCD_X
                        }
                        else{
                            D_datas[tid*(j+1)+j] = B_datas[Q.x - 1];
                        }
                    }
                    else{
                        temp_times[tid] = B[Q.x];
                        //store data
                        if (j == 1)
                        {
                            if(Q.y == 0){
                                D_datas[tid*2] = 1;
                            }
                            else{
                                D_datas[tid*2] = A_datas[Q.y - 1];
                            }
                        }
                        else
                        {
                            if(Q.y == 0){
                                for (int i = 0; i < j; ++i)
                                {
                                    D_datas[tid*(j+1)+i] = 1;
                                }
                            }
                            else{
                                for (int i = 0; i < j; ++i)
                                {
                                    D_datas[tid*(j+1)+i] = temp_datas[(Q.y-1)*j+i];
                                }
                            }
                        }
                        D_datas[tid*(j+1)+j] = B_datas[Q.x];
                    }
                    break;
                }
                else {
                    K.x = Q.x + 1;
                    K.y = Q.y - 1;
                }
            }
            else {
                P.x = Q.x - 1;
                P.y = Q.y + 1;
            }
        }
    }
}

__global__ void CudaMerge(unsigned int j, unsigned int *dev_times, short *dev_datas, 
    int _blocks_per_inst, int cur_level, int inst_id_start, int inst_num_start,int inst_num_end, unsigned int *dev_total_times_start,
    unsigned int *dev_input_start, unsigned int *dev_data_val_num, unsigned int *dev_val_num_start, unsigned int *dev_data_in_num_start, short *dev_data_in_num,
    unsigned int *dev_temp_times, short *dev_temp_datas) {
    const unsigned int _bid = blockIdx.x + blockIdx.y * gridDim.x;
    const unsigned int _tid = threadIdx.x;
    const unsigned int bid = _bid / _blocks_per_inst + inst_id_start;
    if (bid >= inst_num_end || bid < inst_num_start)
    {
        return;
    }
    const unsigned int tid = _tid + (_bid % _blocks_per_inst) * _N_THREADS_PER_BLOCK;
    const unsigned int _start = dev_data_in_num_start[cur_level];
    const short in_num = dev_data_in_num[_start+bid];
    const unsigned int val_num_start = dev_val_num_start[bid];
    const unsigned int A_start = dev_total_times_start[bid];
    const unsigned int A_data_start = dev_input_start[bid];
    unsigned length_A = dev_data_val_num[val_num_start];
    //unsigned total_length = dev_data_val_num[val_num_start];
    for (int i = 1; i < in_num; ++i)
    {
        if (i < j)
        {
            length_A += dev_data_val_num[val_num_start+i];
        }
        //total_length += dev_data_val_num[val_num_start+i];
    }
    unsigned length_B = dev_data_val_num[val_num_start+j];
    if (j < in_num && tid < length_A + length_B)
    {
        unsigned int *times = dev_times + A_start;
        unsigned int *temp_times = dev_temp_times + A_start - dev_total_times_start[inst_num_start];
        short *temp_datas = dev_temp_datas + A_data_start - dev_input_start[inst_num_start];
        short *datas = dev_datas + A_data_start;
        times[tid] = temp_times[tid];
        if (j < in_num - 1){
            for (int i = 0; i <= j; ++i)
            {
                temp_datas[tid*(j+1)+i] = datas[tid*(j+1)+i];
            }
        }
    }
}

/*void processCore(int left, int right, int inst_num, inter &_inter, VCDParser &parser, VCDTime sim_start, VCDTime sim_end,
    unsigned int *total_input_times, char *input_datas,
    std::vector<int> &cur_level, std::vector<unsigned int*> &out_starts, std::vector<unsigned int*> &out_sizes,
    std::vector<unsigned int*> &out_times, std::vector<short*> &out_values, std::map<std::string, TimedValues*> &pinbitValues,
    std::vector<unsigned int> &total_times_start, std::unordered_map<std::string, bool> &cur_flag,
    std::unordered_map<std::string, std::vector<unsigned long long>> &temp_Result)*/
void processCore(int left, int right, int size, VCDTime sim_start, VCDTime sim_end,
    unsigned int *total_input_times, short *input_datas,
    std::vector<std::string> &name_vec, std::vector<unsigned int*> &time_vec, std::vector<short*> &value_vec, 
    unsigned int *step_vec, std::vector<bool> &is_init_vec, std::vector<short> &offset_vec, 
    std::vector<unsigned int> &step_start_vec, std::vector<unsigned int> &com_step_start_vec, 
    std::vector<bool> &cur_flag,
    std::unordered_map<std::string, std::vector<unsigned long long>> &temp_Result)
{
    //debug
    //bool flag = false;
    //
    for (int j = left; j < right && j < size; ++j)
    {
        string _name = name_vec[j];
        //if(_name == "tile_ICCADs_core_ICCADs_div_ICCADs_intadd_10_ICCADs_SUM_6_"){
        //    flag = true;
        //}
        unsigned int *pos = time_vec[j];
        short *pos_val = value_vec[j];
        unsigned int step = step_vec[j];
        bool is_init = is_init_vec[j];
        short offset = offset_vec[j];
        unsigned int sum = 0;
        unsigned int sum_compress = 0;
        unsigned int data_width = step_start_vec[j];//total_times_start[j];
        unsigned int data_width_compress = com_step_start_vec[j];
        bool _cur_flag = cur_flag[j];
        if (is_init)
        {
            unsigned long long time_1=0;
            unsigned long long time_0=0;
            unsigned long long time_x=0;
            unsigned long long time_z=0;
            unsigned long long last_time = sim_start;
            VCDBit last_state = VCD_X;
            short data = 0;
            unsigned k;
            for (k = 0; k < step; ++k)
            {
                total_input_times[sum+data_width] = pos[k];
                if(k > 0 && k % 8 == 0){
                    input_datas[sum_compress+data_width_compress] = data;
                    ++sum_compress;
                    data = 0;
                }
                data = (data << 2);
                data += pos_val[k];
                
                //input_datas[sum+data_width] = static_cast<char>(pos_val[k]);
                ++sum;
                if(_cur_flag){
                    continue;
                }
                VCDTime cur_t = pos[k];
                VCDBit cur_val = VCDBit(pos_val[k]);
                if (cur_t <= sim_start)
                {
                    last_state = cur_val;
                    continue;
                }
                if (cur_t > sim_end)
                {
                    continue;
                }
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
            while(k % 8){
                data = (data << 2);
                data += UNKNOWN;
                ++k;
            }
            input_datas[sum_compress+data_width_compress] = data;
            //++sum_compress;
            
            if (!_cur_flag && last_time < sim_end)
            {
                if (last_state == VCD_X)
                {
                    time_x += sim_end - last_time;
                }
                else if (last_state == VCD_0)
                {
                    time_0 += sim_end - last_time;
                }
                else if (last_state == VCD_1)
                {
                    time_1 += sim_end - last_time;
                }
                else if (last_state == VCD_Z)
                {
                    time_z += sim_end - last_time;
                }
            }
            if(!_cur_flag){
                std::vector<unsigned long long> res = {time_0, time_1, time_x+time_z};
                Result[_name] = res;
                temp_Result[_name] = res;
            }
        }
        else
        {
            unsigned long long time_1=0;
            unsigned long long time_0=0;
            unsigned long long time_x=0;
            unsigned long long time_z=0;
            unsigned long long last_time = sim_start;
            VCDBit last_state = VCD_X;
            // find real start
            unsigned pos_val_start = 0;
            short start_data = pos_val[pos_val_start];
            short data;
            short shift = (3 << 14);
            int o;
            for(o = 0; o < offset; ++o){
                start_data = (start_data << 2);
            }
            short temp_data = 0;
            unsigned k;
            for (k = 0; k < step; ++k)
            {
                short data = ((start_data & shift) >> 14) & 3;
                ++o;
                start_data = (start_data << 2);
                if(o == STATE_NUM){
                    ++pos_val_start;
                    o = 0;
                    start_data = pos_val[pos_val_start];
                }
                total_input_times[sum+data_width] = pos[k];
                if(k > 0 && k % 8 == 0){
                    input_datas[sum_compress+data_width_compress] = temp_data;
                    ++sum_compress;
                    temp_data = 0;
                }
                temp_data = (temp_data << 2);
                temp_data += data;

                //input_datas[sum+data_width] = static_cast<char>(data);
                ++sum;
                if(_cur_flag){
                    continue;
                }
                VCDTime cur_t = pos[k];
                VCDBit cur_val = VCDBit(data);//pos_val[k]);
                if (cur_t <= sim_start)
                {
                    last_state = cur_val;
                    continue;
                }
                if (cur_t > sim_end)
                {
                    continue;
                }
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
            while(k % 8){
                temp_data = (temp_data << 2);
                temp_data += UNKNOWN;
                ++k;
            }
            input_datas[sum_compress+data_width_compress] = temp_data;
            if (!_cur_flag && last_time < sim_end)
            {
                if (last_state == VCD_X)
                {
                    time_x += sim_end - last_time;
                }
                else if (last_state == VCD_0)
                {
                    time_0 += sim_end - last_time;
                }
                else if (last_state == VCD_1)
                {
                    time_1 += sim_end - last_time;
                }
                else if (last_state == VCD_Z)
                {
                    time_z += sim_end - last_time;
                }
            }
            if(!_cur_flag){
                std::vector<unsigned long long> res = {time_0, time_1, time_x+time_z};
                Result[_name] = res;
                temp_Result[_name] = res;
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    std::string vcdFilePath, database_path, saif_out, dumpon_time_str, dumpoff_time_str;
    int dumpon_time, dumpoff_time;
    if (argc == 6)
    {
        vcdFilePath = argv[2];
        database_path = argv[1];
        dumpon_time_str = argv[3];
        dumpoff_time_str = argv[4];
        saif_out = argv[5];
    }
    else
    {
        std::cout << "[USAGE] ./simulate [intermediate_file] vcd_path dumpon_time(ps) dumpoff_time(ps) saif_out" << std::endl;
        exit(-1);
    }
    //PreProcess processor;
    ofstream outf(saif_out);
    if (!outf)
    {
        cout << "File " << saif_out << " Open Error!" << endl;
        cout << "Exit!" << endl;
        exit(-1);
    }
    VCDTime sim_start = stoull(dumpon_time_str);
    VCDTime sim_end = stoull(dumpoff_time_str);

    std::vector<unsigned int> data_in_num_start;
    std::vector<unsigned int> data_out_num_start;
    std::vector<unsigned int> delay_start;
    std::vector<unsigned int> delay_width;
    std::vector<unsigned int> functions_start;
    std::vector<unsigned int> functions_width;

    std::vector<short> data_in_num;
    std::vector<short> data_out_num;
    //std::vector<unsigned int> val_num_start;
    std::vector<short> delay_val_num;
    std::vector<short> functions_func_num;
    std::vector<short> functions_val_num;
    std::vector<short> host_delay_edges;
    std::vector<short> host_in_bit;
    std::vector<short> host_out_bit;
    std::vector<float> host_rise_val;
    std::vector<float> host_fall_val;

    std::vector<short> host_functions;
    auto start_total = std::chrono::steady_clock::now();
    inter _inter;
    ifstream ifs(database_path.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia & _inter;
    cout << "new:" << endl;
    //debug
    cout << _inter.levels.size() << endl;
    cout << _inter.p.instances.size() << endl;
    //
    ifs.close();
    cudaDeviceProp cudade;
    /*cudaGetDeviceProperties(&cudade,0);
    std::cout << "GPU型号： " << cudade.name << std::endl;
    std::cout << "每块全局内存存储容量（B）: " << cudade.totalGlobalMem << std::endl;
    std::cout << "每块共享内存存储容量（B）: "<< cudade.sharedMemPerBlock << std::endl;
    std::cout << "每块寄存器数量: " << cudade.regsPerBlock << std::endl;
    std::cout << "WarpSize：  " << cudade.warpSize << std::endl;
    std::cout << "最大内存复制步长：  " << cudade.memPitch << std::endl;
    std::cout << "每块最大线程数量：  " << cudade.maxThreadsPerBlock << std::endl;
    std::cout << "线程块三维： " << cudade.maxThreadsDim << std::endl;
    std::cout << "线程格三维: " << cudade.maxGridSize << std::endl;
    //std::cout << "计算核心时钟频率（kHz）：  " << cudade.clockRate << std::endl;
    //std::cout << "常量存储容量：  " << cudade.totalConstMem << std::endl;
    //std::cout << "次计算能力（小数点后的值）： " << cudade.minor << std::endl;
    //std::cout << "纹理对齐要求: " << cudade.textureAlignment << std::endl;
    //std::cout << "绑定到等步长内存的纹理满足的要求: " << cudade.texturePitchAlignment << std::endl;
    //std::cout << "GPU是否支持并发内存复制和kernel执行: " << cudade.deviceOverlap << std::endl;
    std::cout << "SMX数量：  " << cudade.multiProcessorCount << std::endl;
    //std::cout << "是否有运行时限制：  " << cudade.kernelExecTimeoutEnabled << std::endl;
    //std::cout << "设备是否集成（否则独立）：  " << cudade.integrated << std::endl;
    //std::cout << "可否对主机内存进行映射： " << cudade.canMapHostMemory << std::endl;
    std::cout << "计算模式: " << cudade.computeMode << std::endl;
    //std::cout << "最大1D纹理尺寸：  " << cudade.maxTexture1D << std::endl;
    //std::cout << "线性内存相关的最大1D纹理尺寸：  " << cudade.maxTexture1DLinear << std::endl;
    //std::cout << "最大2D纹理维度：  " << cudade.maxTexture2D << std::endl;
    //std::cout << "最大2D纹理维度（width,height,pitch）： " << cudade.maxTexture2DLinear << std::endl;
    //std::cout << "纹理聚集时的最大纹理维度: " << cudade.maxTexture2DGather << std::endl;
    /*std::cout << "最大3D纹理维度: " << cudade.maxTexture3D << std::endl;
    std::cout << "最大立方图纹理维度: " << cudade.maxTextureCubemap << std::endl;
    std::cout << "最大1D分层纹理维度：  " << cudade.maxTexture1DLayered << std::endl;
    std::cout << "最大2D分层纹理维度：  " << cudade.maxTexture2DLayered << std::endl;
    std::cout << "最大立方图分层纹理维度：  " << cudade.maxTextureCubemapLayered << std::endl;
    std::cout << "最大1D表面尺寸： " << cudade.maxSurface1D << std::endl;
    std::cout << "主计算能力（小数点前的值）：  " << cudade.major << std::endl;
    std::cout << "最大2D表面维度: " << cudade.maxSurface2D << std::endl;
    std::cout << "最大3D表面维度：  " << cudade.maxSurface3D << std::endl;
    std::cout << "最大1D分层表面维度：  " << cudade.maxSurface1DLayered << std::endl;
    std::cout << "最大2D分层表面维度：  " << cudade.maxSurface2DLayered << std::endl;
    std::cout << "最大立方图表面维度： " << cudade.maxSurfaceCubemap << std::endl;
    std::cout << "表面对齐要求: " << cudade.surfaceAlignment << std::endl;
    std::cout << "设备能并发的kernel数量: " << cudade.concurrentKernels << std::endl;
    std::cout << "是否打开ECC校验: " << cudade.ECCEnabled << std::endl;
    std::cout << "PCI总线ID：  " << cudade.pciBusID << std::endl;
    std::cout << "PCI设备ID：  " << cudade.pciDeviceID << std::endl;
    std::cout << "PCI域ID：  " << cudade.pciDomainID << std::endl;
    std::cout << "是否支持TCC（Tesla集群）： " << cudade.tccDriver << std::endl;
    std::cout << "异步引擎数量: " << cudade.asyncEngineCount << std::endl;
    std::cout << "主机和设备共享同一地址空间：  " << cudade.unifiedAddressing << std::endl;
    std::cout << "存储时钟频率：  " << cudade.memoryClockRate << std::endl;
    std::cout << "Global memory 总线带宽：  " << cudade.memoryBusWidth << std::endl;
    std::cout << "L2 Cache尺寸（B)：  " << cudade.l2CacheSize << std::endl;*/
    //std::cout << "每个SMX驻留的最大线程数量：  " << cudade.maxThreadsDim << std::endl;
    cudaSetDevice(0);
    cudaError_t err;
    // malloc global parameters
    auto start_malloc = std::chrono::steady_clock::now();
    unsigned int *dev_data_in_num_start;
    unsigned int *dev_data_out_num_start;
    unsigned int *dev_delay_start;
    unsigned int *dev_delay_width;
    unsigned int *dev_functions_start;
    unsigned int *dev_functions_width;
    //unsigned int *dev_val_num_start;
    short *dev_data_in_num;
    short *dev_data_out_num;
    short *dev_delay_val_num;
    short *dev_functions_func_num;
    short *dev_functions_val_num;
    short *dev_delay_edges;
    short *dev_in_bit;
    short *dev_out_bit;
    float *dev_rise_val;
    float *dev_fall_val;
    short *dev_functions;
    err=cudaMalloc((void **)&dev_data_in_num_start, sizeof(unsigned int)*(_inter.data_in_num_start).size());
    err=cudaMalloc((void **)&dev_data_out_num_start, sizeof(unsigned int)*(_inter.data_out_num_start).size());
    err=cudaMalloc((void **)&dev_delay_start, sizeof(unsigned int)*(_inter.delay_start).size());
    err=cudaMalloc((void **)&dev_delay_width, sizeof(unsigned int)*(_inter.delay_width).size());
    err=cudaMalloc((void **)&dev_functions_start, sizeof(unsigned int)*(_inter.functions_start).size());
    err=cudaMalloc((void **)&dev_functions_width, sizeof(unsigned int)*(_inter.functions_width).size());
    //err=cudaMalloc((void **)&dev_val_num_start, sizeof(unsigned int)*val_num_start.size());
    err=cudaMalloc((void **)&dev_data_in_num, sizeof(short)*(_inter.data_in_num).size());
    err=cudaMalloc((void **)&dev_data_out_num, sizeof(short)*(_inter.data_out_num).size());
    err=cudaMalloc((void **)&dev_delay_val_num, sizeof(short)*(_inter.delay_val_num).size());
    err=cudaMalloc((void **)&dev_functions_func_num, sizeof(short)*(_inter.functions_func_num).size());
    err=cudaMalloc((void **)&dev_functions_val_num, sizeof(short)*(_inter.functions_val_num).size());
    err=cudaMalloc((void **)&dev_delay_edges, sizeof(short)*(_inter.host_delay_edges).size());
    err=cudaMalloc((void **)&dev_in_bit, sizeof(short)*(_inter.host_in_bit).size());
    err=cudaMalloc((void **)&dev_out_bit, sizeof(short)*(_inter.host_out_bit).size());
    err=cudaMalloc((void **)&dev_rise_val, sizeof(float)*(_inter.host_rise_val).size());
    err=cudaMalloc((void **)&dev_fall_val, sizeof(float)*(_inter.host_fall_val).size());
    err=cudaMalloc((void **)&dev_functions, sizeof(short)*(_inter.host_functions).size());
    checkError(err,"cudamalloc error");
    
    err = cudaMemcpy(dev_data_in_num_start, &(_inter.data_in_num_start)[0], sizeof(unsigned int)*(_inter.data_in_num_start).size(), cudaMemcpyHostToDevice);
    err = cudaMemcpy(dev_data_out_num_start, &(_inter.data_out_num_start)[0], sizeof(unsigned int)*(_inter.data_out_num_start).size(), cudaMemcpyHostToDevice);
    err = cudaMemcpy(dev_delay_start, &(_inter.delay_start)[0], sizeof(unsigned int)*(_inter.delay_start).size(), cudaMemcpyHostToDevice);
    err = cudaMemcpy(dev_delay_width, &(_inter.delay_width)[0], sizeof(unsigned int)*(_inter.delay_width).size(), cudaMemcpyHostToDevice);
    err = cudaMemcpy(dev_functions_start, &(_inter.functions_start)[0], sizeof(unsigned int)*(_inter.functions_start).size(), cudaMemcpyHostToDevice);
    err = cudaMemcpy(dev_functions_width, &(_inter.functions_width)[0], sizeof(unsigned int)*(_inter.functions_width).size(), cudaMemcpyHostToDevice);
    //err = cudaMemcpy(dev_val_num_start, &val_num_start[0], sizeof(unsigned int)*val_num_start.size(), cudaMemcpyHostToDevice);
    err = cudaMemcpy(dev_data_in_num, &(_inter.data_in_num)[0], sizeof(short)*(_inter.data_in_num).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_data_out_num, &(_inter.data_out_num)[0], sizeof(short)*(_inter.data_out_num).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_delay_val_num, &(_inter.delay_val_num)[0], sizeof(short)*(_inter.delay_val_num).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_functions_func_num, &(_inter.functions_func_num)[0], sizeof(short)*(_inter.functions_func_num).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_functions_val_num, &(_inter.functions_val_num)[0], sizeof(short)*(_inter.functions_val_num).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_functions, &(_inter.host_functions)[0], sizeof(short)*(_inter.host_functions).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_delay_edges, &(_inter.host_delay_edges)[0], sizeof(short)*(_inter.host_delay_edges).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_in_bit, &(_inter.host_in_bit)[0], sizeof(short)*(_inter.host_in_bit).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_out_bit, &(_inter.host_out_bit)[0], sizeof(short)*(_inter.host_out_bit).size(), cudaMemcpyHostToDevice);            
    err=cudaMemcpy(dev_rise_val, &(_inter.host_rise_val)[0], sizeof(float)*(_inter.host_rise_val).size(), cudaMemcpyHostToDevice);
    err=cudaMemcpy(dev_fall_val, &(_inter.host_fall_val)[0], sizeof(float)*(_inter.host_fall_val).size(), cudaMemcpyHostToDevice);
    checkError(err,"cudamemcpy error");
            

    /*std::map<std::string, Instance> instances = _inter.p.get_instances();
    std::vector<string> _inst_name_vec = _inter.p.get_instance_names();*/
    std::map<std::string, std::string> assign_pairs = _inter.p.get_assign_pairs();
    std::map<std::string, TimedValues*> pinbitValues = _inter.p.get_pinbitValues();
    std::vector<std::map<std::string, std::vector<unsigned int> > > OutMaps = _inter.OutMaps;
    //std::vector<std::string> pin_bits = _inter.p.pin_bits;


    cout << "start parsing vcd file..." << endl;
    
    /** parse vcd & simulate **/
    VCDParser parser;// = new VCDParser();

    // replace pre-malloc at PreProcess.cpp
    parser.init(vcdFilePath, sim_end);
    // need use map at PreProcess.cpp
    std::vector<std::string> _pin_bits = _inter.p.pin_bits;
    for(auto it:_pin_bits){
        if(it.find("[") != std::string::npos){
            pin_bits_set.insert(it);
        }
    }
    //
    auto start_vcd = std::chrono::steady_clock::now();
    if(parser.parse(vcdFilePath, sim_start, sim_end))//, initial_net_map))
    {
        auto end_vcd1 = std::chrono::steady_clock::now();
        long duration_vcd1 = std::chrono::duration_cast<std::chrono::milliseconds>(end_vcd1 - start_vcd).count();
        cout << "total time of vcd parse(only parse): " << duration_vcd1 << "ms" << endl;
        parser.process();

        int tmpp = -1;
        long duration_gpu=0;
        long duration_pre=0;
        long duration_pro0=0;
        long duration_pro1=0;
        long duration_cuda=0;
        long duration_cuda2=0;
        long duration_out=0;
        long duration_out2 = 0;
        long duration_data = 0;
        long duration_malloc2 = 0;
        long duration_pro00 = 0;
        auto end_vcd = std::chrono::steady_clock::now();
        long duration_vcd = std::chrono::duration_cast<std::chrono::milliseconds>(end_vcd - start_vcd).count();
        cout << "total time of vcd parse: " << duration_vcd << "ms" << endl;
        //vcd signals
        VCDTimeUnit time_unit = TIME_PS;//parser.time_units;
        unsigned time_res = 1;//parser.time_resolution;
        //VCDScope *root = parser.root_scope;
        string root_name = "root";//root->name;


        
        /** Simulate **/
        cout << "start simulator..." << endl;
        auto start = std::chrono::steady_clock::now();
        
        //GPU cuda
        unsigned _size1 = _inter.levels.size();
        std::vector<unsigned int*> out_times(_size1); //unsigned int **out_times = (unsigned int **)malloc(sizeof(unsigned int*)*_size1);
        std::vector<short*> out_values(_size1);
        std::vector<unsigned int*> out_sizes(_size1);
        std::vector<unsigned int*> out_starts(_size1);
        std::unordered_map<std::string, std::vector<unsigned int> > vec_bit_times;
        std::unordered_map<std::string, std::vector<short> > vec_bit_values;

        for (unsigned i = 0; i < _size1; ++i)
        {
            //cout << "level " << i << endl;
            const int _cur_level = i;
            std::vector<int> cur_level = (_inter.levels)[i];
            const int inst_num = cur_level.size();
            
            int blocks_per_inst = 1;
            int n_blocks = inst_num;

            int height = inst_num;
            //data to GPU
            int _time_unit = time_unit;

            
            unsigned out_width=0;
            unsigned length_of_valid_width=0;
            unsigned out_width2=0;
            unsigned data_width=0;
            unsigned data_width_compress=0;
            unsigned input_width=0;
            int process_data_width = 0;

            
            unsigned int val_num_width=0;

            
            int functions_width=0;

            unsigned int times_width=0;
            
            int delay_width=0;

            //some flag
            //short *data_in_num = (short *)malloc(sizeof(short)*height);
            //short *data_out_num = (short *)malloc(sizeof(short)*height);


            unsigned int *input_datas_val_num = (unsigned int *)malloc(sizeof(unsigned int)*inst_num*MAX_IN_NUM);
            unsigned int _end = stoull(dumpoff_time_str);
            unsigned long long Length;
            if(_end > 1000000000)
                Length = _end * 2;
            else
                Length = _end * 25;
            //char *input_datas = new char[Length];
            short *input_datas = new short[Length / STATE_NUM + inst_num * MAX_IN_NUM * 2 / 3];
            
            //inst_num*N_THREADS_PER_BLOCK*N_TIMES_PER_THREAD*12);
            //unsigned int *input_times = (unsigned int *)malloc(sizeof(unsigned int)*200000000);//inst_num*N_THREADS_PER_BLOCK*N_TIMES_PER_THREAD*12);
            unsigned int input_datas_size = 0;
            //unsigned int *total_input_times = new unsigned int[Length];
            unsigned int *total_input_times = new unsigned int[Length];
            // for times compress
            //unsigned int *base_start = new unsigned int[Length / 10];
            //unsigned int *base_start_start = new unsigned int[inst_num * MAX_IN_NUM];
            //base_start[0] = 0;
            unsigned base_start_total = 0;
            unsigned max_base_start_idx = 0;
            unsigned base_start_start_idx = 0;

            //cout << inst_num << endl;
            unsigned int total_input_times_size = 0;
            std::vector<unsigned int> total_times_sizes(inst_num);
            std::vector<unsigned int> total_times_start(inst_num);
            std::vector<unsigned int> total_times_sizes_com(inst_num);
            std::vector<unsigned int> total_times_start_com(inst_num);

            //std::vector<int> input_start(inst_num);
            unsigned int *input_start = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            unsigned int *output_start = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            unsigned int *output_size = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            unsigned int *valid_width_start = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            unsigned int *valid_width_size = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            //unsigned int *output_start2 = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            //unsigned int *output_size2 = (unsigned int *)malloc(sizeof(unsigned int)*inst_num);
            if (output_size == NULL)
            {
                cout << "error: memory full!" << endl;
                exit(-1);
            }
            //int *val_num_start = (int *)malloc(sizeof(int)*inst_num);
            std::vector<unsigned int> val_num_start(inst_num);

            auto start_pro = std::chrono::steady_clock::now();
            unsigned int max_in_num = 0;
            //unsigned int max_single_num = 0;
            unsigned int times_start = 0;
            std::vector<unsigned int> merge_max_thread(MAX_IN_NUM);

            std::unordered_map<std::string, std::vector<unsigned long long> > temp_Result;
            //short temp_data;
            //int t_flag = 0;
            std::vector<string> name_vec;
            //std::vector<unsigned int> step_vec;
            std::vector<unsigned int*> time_vec;
            std::vector<short*> value_vec;
            std::vector<bool> is_init_vec;
            std::vector<short> offset_vec;
            std::vector<unsigned> step_start_vec;
            std::vector<unsigned> com_step_start_vec;
            std::vector<bool> cur_flag;
            name_vec.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            time_vec.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            value_vec.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            is_init_vec.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            offset_vec.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            step_start_vec.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            cur_flag.reserve(inst_num * MAX_IN_NUM * 2 / 3);
            unsigned long long step_sum = 0;
            unsigned int max_step = 0;
            for (int j = 0; j < inst_num; ++j)
            {
                Instance *cur_inst = _inter.find_inst(cur_level[j]);//&((_inter.p.get_instances())[(_inter.p.get_instance_names())[cur_level[j]]]);
                std::vector<string> cur_in = cur_inst->in_net;
                short data_in_num = cur_in.size();
                std::vector<string> cur_out = cur_inst->out_net;
                short data_out_num = cur_out.size();
                std::vector<int> in_net_from_id = cur_inst->in_net_from_id;
                std::vector<int> in_net_from_level = cur_inst->in_net_from_level;    
                std::vector<int> in_net_from_pos_at_level = cur_inst->in_net_from_pos_at_level; 
                unsigned int sum = 0;
                unsigned int sum_compress = 0;
                short offset = 0;
                for(unsigned it = 0; it < cur_in.size(); it++)
                {
                    unsigned int *pos = nullptr;
                    short *pos_val = nullptr;
                    string _name = cur_in[it];
                    name_vec.push_back(_name);
                    if(in_net_from_id[it] != -2 && Result.find(_name) == Result.end()){
                        cur_flag.push_back(false);
                        Result[_name].resize(3);
                        temp_Result[_name].resize(3);
                    }
                    else{
                        cur_flag.push_back(true);
                    }
                    unsigned int step;
                    if (in_net_from_id[it] == -1)
                    {
                        std::vector<std::string> in_net_from_info = cur_inst->in_net_from_info;
                        string in_name = in_net_from_info[it];

                        if((parser.sizes).find(in_name) != (parser.sizes).end()){
                            step = (parser.sizes)[in_name];
                            pos = &(*((parser.times)[in_name].begin()));
                            pos_val = &(*((parser.values)[in_name].begin()));
                            time_vec.push_back(pos);
                            value_vec.push_back(pos_val);
                            auto _pos = (parser.InMaps_bus).find(_name);
                            if(_pos != (parser.InMaps_bus).end()){
                                (parser.InMaps_bus).erase(_pos);
                            }
                        }
                        else{
                            string hash = (parser.hash_name_pair)[in_name];
                            unsigned _idx = (parser.hash_index_pair)[hash];
                            step = (parser.vec_times)[_idx].size();//(parser.vec_sizes)[_idx];//
                            pos = &(*((parser.vec_times)[_idx].begin()));
                            pos_val = &(*((parser.vec_values)[_idx].begin()));
                            time_vec.push_back(pos);
                            value_vec.push_back(pos_val);
                            auto _pos = (parser.InMaps_bit).find(_name);
                            if(_pos != (parser.InMaps_bit).end()){
                                (parser.InMaps_bit).erase(_pos);
                            }
                        }
                        
                        step_start_vec.push_back(sum+data_width);
                        com_step_start_vec.push_back(sum_compress + data_width_compress);
                        max_step = max(step, max_step);
                        sum += step;
                        sum_compress += (step + STATE_NUM - 1) / STATE_NUM;
                        //step_vec.push_back(step);
                        is_init_vec.push_back(true);
                        offset_vec.push_back(0);
                        /*
                        pos = &(*((parser.times)[in_name].begin()));
                        pos_val = &(*((parser.values)[in_name].begin()));
                        time_vec.push_back(pos);
                        value_vec.push_back(pos_val);
                        if((parser.sizes).find(in_name) != (parser.sizes).end()){
                            step = (parser.sizes)[in_name];
                        }else{
                            step = (parser.times)[in_name].size();
                        }
                        step_start_vec.push_back(sum+data_width);
                        sum += step;
                        //step_vec.push_back(step);
                        is_init_vec.push_back(true);
                        offset_vec.push_back(0);*/
                    }
                    else if (in_net_from_id[it] >= 0)
                    {
                        std::vector<int> in_net_from_out_order = cur_inst->in_net_from_out_order;
                        int _level = in_net_from_level[it];
                        int _level_pos = in_net_from_pos_at_level[it];  
                        auto _pos = OutMaps[_level].find(_name);
                        if(_pos != OutMaps[_level].end()){
                            OutMaps[_level].erase(_pos);
                        } 
                        
                        unsigned tmp_step = *(out_sizes[_level] + _level_pos * MAX_OUT_NUM);
                        step = *(out_sizes[_level] + _level_pos * MAX_OUT_NUM + in_net_from_out_order[it]);
                        unsigned int _out_start = *(out_starts[_level] + _level_pos) + in_net_from_out_order[it] * tmp_step;
                        pos = out_times[_level] + _out_start;
                        pos_val = out_values[_level] + _out_start / STATE_NUM;
                        time_vec.push_back(pos);
                        value_vec.push_back(pos_val);
                        offset = _out_start % STATE_NUM;
                        step_start_vec.push_back(sum+data_width);
                        com_step_start_vec.push_back(sum_compress + data_width_compress);
                        max_step = max(step, max_step);
                        sum += step;
                        sum_compress += (step + STATE_NUM - 1) / STATE_NUM;
                        //step_vec.push_back(step);
                        is_init_vec.push_back(false);
                        offset_vec.push_back(offset);
                    }
                    else if (in_net_from_id[it] == -2)
                    {
                        std::vector<std::string> in_net_from_info = cur_inst->in_net_from_info;
                        string in_name = in_net_from_info[it];

                        TimedValues* te = pinbitValues[in_name];
                        auto tev = te->begin();
                        step = te->size();
                        unsigned int* _tvs = (unsigned int *)malloc(sizeof(unsigned int)*step);
                        short* _tvs_val = (short *)malloc(sizeof(short)*step);
                        for (unsigned k = 0; k < step; ++k)
                        {
                            TimedValue _tv = *(tev+k);
                            _tvs[k] = _tv.t;
                            _tvs_val[k] = _tv.value;
                        }
                        pos = _tvs;
                        pos_val = _tvs_val;
                        time_vec.push_back(pos);
                        value_vec.push_back(pos_val);
                        step_start_vec.push_back(sum+data_width);
                        com_step_start_vec.push_back(sum_compress + data_width_compress);
                        max_step = max(step, max_step);
                        sum += step;
                        sum_compress += (step + STATE_NUM - 1) / STATE_NUM;
                        //step_vec.push_back(step);
                        is_init_vec.push_back(true);
                        offset_vec.push_back(0);
                    }
                    else
                    {
                        cout << "error." << endl;
                        exit(-1);
                    }
                    step_sum += step;
                    input_datas_val_num[it+val_num_width] = step;
                    merge_max_thread[it] = max(merge_max_thread[it], sum);
                } 
                //
                //int tmp_size = sum;//tmp_times.size();
                total_times_sizes[j] = sum;//tmp_size;
                total_times_start[j] = data_width;
                //total_times_sizes_com[j] = sum_compress;//tmp_size;
                //total_times_start_com[j] = data_width_compress;
                input_start[j] = input_width;

                
                int _blocks_per_inst = (sum + N_THREADS_PER_BLOCK * N_TIMES_PER_THREAD - 1) / (N_THREADS_PER_BLOCK * N_TIMES_PER_THREAD);
                if (_blocks_per_inst*inst_num/MAX_BLOCKS > MAX_BLOCKS)
                {
                    _blocks_per_inst = (MAX_BLOCKS + inst_num - 1) / inst_num * MAX_BLOCKS;
                }

                unsigned int _out_width = sum;
                unsigned int _length_of_valid_width = (sum + N_TIMES_PER_THREAD - 1) / N_TIMES_PER_THREAD;
                output_start[j] = out_width;
                valid_width_start[j] = length_of_valid_width;
                output_size[j] = _out_width;
                valid_width_size[j] = _length_of_valid_width;
                out_width += _out_width*data_out_num;
                length_of_valid_width += _length_of_valid_width * data_out_num;

                blocks_per_inst = max(blocks_per_inst, _blocks_per_inst);

                val_num_start[j] = val_num_width;
                unsigned int _val_num_width = data_in_num;
                max_in_num = max(max_in_num, _val_num_width);
                val_num_width += _val_num_width;
                data_width += sum;//tmp_size*data_in_num[j];
                data_width_compress += sum_compress;
                input_width += sum*data_in_num;                
            }
            //data_width_compress = (data_width + STATE_NUM - 1 ) / STATE_NUM;
            auto end_pro000 = std::chrono::steady_clock::now();
            duration_pro00 += std::chrono::duration_cast<std::chrono::microseconds>(end_pro000 - start_pro).count();

            // CPU multiple threads
            unsigned size = name_vec.size();
            unsigned max_cores = (size > thread::hardware_concurrency()*6) ? (thread::hardware_concurrency()*6) : size;
            vector<thread> threads(max_cores);
            unsigned left = 0;
            //unsigned num_every_thread = (size + max_cores - 1) / max_cores;
            unsigned long long num_every_thread = step_sum / max_cores;
            vector<unsigned> right_vec(max_cores);
            for(unsigned t = 0; t < max_cores; ++t){
                unsigned ll;
                unsigned long long _size = 0;
                for(ll = left; ll < size && _size < num_every_thread; ++ll){
                    _size += input_datas_val_num[ll];
                }
                right_vec[t] = ll;
                left = ll;
            }
            left = 0;
            for(unsigned t = 0; t < max_cores; ++t){
                unsigned right = right_vec[t];//left + num_every_thread;
                //cout << "start thread" << endl;
                /*threads[t] = thread(processCore, left, right, inst_num, std::ref(_inter), std::ref(parser), sim_start, 
                    sim_end, total_input_times, input_datas, std::ref(cur_level), std::ref(out_starts), std::ref(out_sizes),
                    std::ref(out_times), std::ref(out_values), std::ref(pinbitValues),
                    std::ref(total_times_start), std::ref(cur_flag), std::ref(temp_Result));*/
                threads[t] = thread(processCore, left, right, size, sim_start, sim_end, 
                    total_input_times, input_datas, std::ref(name_vec), std::ref(time_vec), std::ref(value_vec),
                    input_datas_val_num ,std::ref(is_init_vec), std::ref(offset_vec), 
                    std::ref(step_start_vec), std::ref(com_step_start_vec), std::ref(cur_flag), std::ref(temp_Result));
                left = right;
            }
            for(auto &t:threads){
                t.join();
            }

            // Done multiple-thread
            
            

            int n_block_num2 = blocks_per_inst;//(inst_num * blocks_per_inst + MAX_BLOCKS - 1) / MAX_BLOCKS;
            int n_block_num1 = inst_num;//(inst_num * blocks_per_inst + n_block_num2 - 1) / n_block_num2;
            dim3 n_grid(n_block_num1, n_block_num2, 1);
            //n_blocks = inst_num * blocks_per_inst;
            
            //cout << "blocks_per_inst: " << blocks_per_inst << endl;
            //out_width = out_width * max_out;
            
            //sort(total_input_times->begin(),total_input_times->end());  
            //total_input_times->erase(unique(total_input_times->begin(),total_input_times->end()),total_input_times->end());
            //cout << "total_input_times size: " << total_input_times->size() << endl;
            //cout << "data_width size: " << data_width << endl;
            //cout << "data_width_compress size: " << data_width_compress << endl;
            //cout << "max_in size: " << max_in_num << endl;
            //out_width = total_input_times->size() * max_out;
            auto end_pro = std::chrono::steady_clock::now();
            long duration_pro = std::chrono::duration_cast<std::chrono::milliseconds>(end_pro - start_pro).count();
            //cout << "total time of proccess: " << duration_pro << "ms" << endl;
            //cout << "total time of proccess(us): " << duration_pro00 << "us" << endl;
            duration_pre += duration_pro;
            
            auto start_data = std::chrono::steady_clock::now();


            unsigned int *dev_total_times_sizes;
            err=cudaMalloc((void **)&dev_total_times_sizes, sizeof(unsigned int)*height);
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_total_times_sizes) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            err = cudaMemcpy(dev_total_times_sizes, &total_times_sizes[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_total_times_sizes) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            unsigned int *dev_total_times_start;
            err=cudaMalloc((void **)&dev_total_times_start, sizeof(unsigned int)*height);
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_total_times_start) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            err = cudaMemcpy(dev_total_times_start, &total_times_start[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            //unsigned int *dev_total_times_sizes_com;
            //err=cudaMalloc((void **)&dev_total_times_sizes_com, sizeof(unsigned int)*height);
            //if(err!=cudaSuccess)
            //{
            //    printf("the cudaMalloc(dev_total_times_sizes) on GPU is failed, return value is %d\n", err);
            //    cout << cudaGetErrorString(err) << endl;
            //   exit(-1);
            //}
            //err = cudaMemcpy(dev_total_times_sizes_com, &total_times_sizes_com[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            //if(err!=cudaSuccess)
            //{
            //    printf("the cudaMemcpy(dev_total_times_sizes) on GPU is failed\n");
            //    cout << cudaGetErrorString(err) << endl;
            //    exit(-1);
            //}
            /*unsigned int *dev_total_times_start_com;
            err=cudaMalloc((void **)&dev_total_times_start_com, sizeof(unsigned int)*height);
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_total_times_start) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            err = cudaMemcpy(dev_total_times_start_com, &total_times_start_com[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);*/
            //Decompress GPU
            short *dev_in_datas;
            //char *dev_in_data_compressed;
            short *dev_in_data_compressed;
            unsigned int *dev_step_start_vec;
            unsigned int *dev_com_step_start_vec;
            unsigned int *dev_data_val_num;
            unsigned com_size = step_start_vec.size();
            err=cudaMalloc((void **)&dev_in_datas, sizeof(short)*data_width);
            err=cudaMalloc((void **)&dev_in_data_compressed, sizeof(short)*data_width_compress);
            err=cudaMalloc((void **)&dev_step_start_vec, sizeof(unsigned int)*com_size);
            err=cudaMalloc((void **)&dev_com_step_start_vec, sizeof(unsigned int)*com_size);
            err=cudaMalloc((void **)&dev_data_val_num, sizeof(unsigned int)*val_num_width);
            err = cudaMemcpy(dev_in_data_compressed, &input_datas[0], sizeof(short)*data_width_compress, cudaMemcpyHostToDevice);
            err=cudaMemcpy(dev_step_start_vec, &step_start_vec[0], sizeof(unsigned int)*com_size, cudaMemcpyHostToDevice);
            err=cudaMemcpy(dev_com_step_start_vec, &com_step_start_vec[0], sizeof(unsigned int)*com_size, cudaMemcpyHostToDevice);
            err=cudaMemcpy(dev_data_val_num, &input_datas_val_num[0], sizeof(unsigned int)*val_num_width, cudaMemcpyHostToDevice);

            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_in_data_compressed) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            //int _n_block_num2 = (data_width / N_THREADS_PER_BLOCK > MAX_BLOCKS) ? MAX_BLOCKS : 1;
            //int _n_block_num1 = (data_width / N_THREADS_PER_BLOCK > MAX_BLOCKS) ? ((data_width + N_THREADS_PER_BLOCK*MAX_BLOCKS - 1)/(N_THREADS_PER_BLOCK*MAX_BLOCKS)) : ((data_width + N_THREADS_PER_BLOCK - 1)/N_THREADS_PER_BLOCK);
            unsigned int blocks_per_signal = (((max_step + STATE_NUM - 1) / STATE_NUM) + N_THREADS_PER_BLOCK - 1) / N_THREADS_PER_BLOCK;
            int _n_block_num2 = (com_size * blocks_per_signal > MAX_BLOCKS) ? MAX_BLOCKS : 1;
            int _n_block_num1 = (com_size * blocks_per_signal > MAX_BLOCKS) ? ((com_size * blocks_per_signal + MAX_BLOCKS - 1)/(MAX_BLOCKS)) : (com_size * blocks_per_signal);
            //cout << "_n_grid:" << _n_block_num1 << ", " << _n_block_num2 << endl;
            dim3 _n_grid(_n_block_num1, _n_block_num2, 1);
            DecompressGPU<<<_n_grid, N_THREADS_PER_BLOCK>>>(dev_in_datas, dev_in_data_compressed, dev_step_start_vec, 
                dev_com_step_start_vec, dev_data_val_num, blocks_per_signal, com_size);
            cudaDeviceSynchronize();
            err = cudaGetLastError();
            if (err != cudaSuccess)
            {
                cout << "DecompressGPU kenerl error: " << cudaGetErrorString(err) << endl;
            } 
            cudaFree(dev_in_data_compressed);
            cudaFree(dev_step_start_vec);
            cudaFree(dev_com_step_start_vec);

            // Done Decompress
            
            unsigned int *dev_times;
            total_input_times_size = data_width;
            err=cudaMalloc((void **)&dev_times, sizeof(unsigned int)*total_input_times_size);
            err=cudaMemcpy(dev_times, &(total_input_times[0]), sizeof(unsigned int)*total_input_times_size, cudaMemcpyHostToDevice);
            
            unsigned int *dev_input_start;
            err=cudaMalloc((void **)&dev_input_start, sizeof(unsigned int)*height);
            err = cudaMemcpy(dev_input_start, &input_start[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);


            short *dev_datas;
            err = cudaMalloc((void **)&dev_datas, sizeof(short)*input_width);
            
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_total_times_start) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }

            /*if(_cur_level == 4){
                ofstream outFile("test.dat", ios::out | ios::binary);
                outFile.write((char*)&total_input_times[0], sizeof(unsigned int)*total_input_times_size);
                outFile.close();
            }*/

            
            
            unsigned int *dev_val_num_start;
            err=cudaMalloc((void **)&dev_val_num_start, sizeof(unsigned int)*height);
            
            err = cudaMemcpy(dev_val_num_start, &val_num_start[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            
            if(data_width > 390000000){
                
                unsigned data_width1 = total_times_start[(inst_num + 1) / 2];
                unsigned data_width2 = data_width - total_times_start[(inst_num + 1) / 2];
                unsigned input_width1 = input_start[(inst_num + 1) / 2];
                unsigned input_width2 = input_width - input_start[(inst_num + 1) / 2];
                //err = cudaMalloc((void **)&dev_in_datas, sizeof(short)*data_width1);
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(in data) on GPU is failed");
                    exit(-1);
                }

                // GPU Merge sort
                unsigned int *dev_temp_times;
                err=cudaMalloc((void **)&dev_temp_times, sizeof(unsigned int)*data_width1);
                //err=cudaMemset(dev_temp_times, 0, sizeof(unsigned int)*total_input_times_size);
            
                short *dev_temp_datas;
            
                err = cudaMalloc((void **)&dev_temp_datas, sizeof(short)*input_width1);
    
                //err=cudaMemcpy(dev_in_datas, &input_datas[0], sizeof(short)*data_width1, cudaMemcpyHostToDevice);
                
                auto start_data2 = std::chrono::steady_clock::now();
                for (unsigned j = 1; j < max_in_num; j++)
                {
                    //cout << "CUDA merge." << j << endl;
                    unsigned loop_num = 1;
                    int block_num1, block_num2, block_num;
                    int _blocks_per_inst;
                    unsigned int _merge_max_thread = max(merge_max_thread[0], merge_max_thread[j]);
                    if (inst_num / 2 *((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK) / _MAX_BLOCKS > _MAX_BLOCKS)
                    {
                        //cout << "CUDA merge multiple." << endl;
                        unsigned num = inst_num;
                        unsigned temp = inst_num*((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK);
                        while(temp > _MAX_BLOCKS)
                        {
                            num = (inst_num + loop_num) / (loop_num + 1);
                            temp = num*((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK);
                            loop_num++;
                        }
                        int every_loop_num = (inst_num + loop_num - 1) / loop_num;
                        int cur_inst_num = inst_num;
                        int inst_id_start = 0;
                        while(loop_num > 0)
                        {
                            if (loop_num == 1)
                            {
                                every_loop_num = cur_inst_num;
                            }
                            _blocks_per_inst = (_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK;
                            block_num = every_loop_num*_blocks_per_inst;
                            PreCudaMerge<<<block_num, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, dev_in_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, (inst_num + 1) / 2,
                                dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                                dev_temp_times, dev_temp_datas);
                            cudaDeviceSynchronize();
                            err = cudaGetLastError();
                            if (err != cudaSuccess)
                            {
                                cout << "cuda merge kenerl error: " << cudaGetErrorString(err) << endl;
                            }  
                            CudaMerge<<<block_num, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, (inst_num + 1) / 2,
                                dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                                dev_temp_times, dev_temp_datas);
                            cudaDeviceSynchronize();
                            err = cudaGetLastError();
                            if (err != cudaSuccess)
                            {
                                cout << "cuda merge2 kenerl error: " << cudaGetErrorString(err) << endl;
                            }  
                            loop_num--;
                            cur_inst_num -= every_loop_num;
                            inst_id_start += every_loop_num;
                        }
                    }
                    else{
                        _blocks_per_inst = (_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK;
                        block_num2 = (inst_num * _blocks_per_inst + _MAX_BLOCKS - 1) / _MAX_BLOCKS;
                        block_num1 = (inst_num * _blocks_per_inst + block_num2 - 1) / block_num2;
                        dim3 grid(block_num1, block_num2, 1);
                        int inst_id_start = 0;
                        PreCudaMerge<<<grid, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, dev_in_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, (inst_num + 1) / 2,
                            dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                            dev_temp_times, dev_temp_datas);
                        cudaDeviceSynchronize();
                        err = cudaGetLastError();
                        if (err != cudaSuccess)
                        {
                            cout << "cuda merge kenerl error: " << cudaGetErrorString(err) << endl;
                        }     
                        CudaMerge<<<grid, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, (inst_num + 1) / 2,
                            dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                            dev_temp_times, dev_temp_datas);
                        cudaDeviceSynchronize();
                        err = cudaGetLastError();
                        if (err != cudaSuccess)
                        {
                            cout << "cuda merge2 kenerl error: " << cudaGetErrorString(err) << endl;
                        }  
                    }
                }
                auto end_data2 = std::chrono::steady_clock::now();
                long duration_data2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_data2 - start_data2).count();
                duration_malloc2 += duration_data2;
                cudaFree(dev_temp_times);
                cudaFree(dev_temp_datas);
                //cudaFree(dev_in_datas);
                // Next
                //err = cudaMalloc((void **)&dev_in_datas, sizeof(short)*data_width2);
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(in data) on GPU is failed");
                    exit(-1);
                }
                // GPU Merge sort
                err=cudaMalloc((void **)&dev_temp_times, sizeof(unsigned int)*data_width2);
                err = cudaMalloc((void **)&dev_temp_datas, sizeof(short)*input_width2);
                //err=cudaMemcpy(dev_in_datas, input_datas+data_width1, sizeof(short)*data_width2, cudaMemcpyHostToDevice);
                if(err!=cudaSuccess)
                {
                    printf("the cudaMemcpy(dev_in_datas) on GPU is failed\n");
                    cout << cudaGetErrorString(err) << endl;
                    exit(-1);
                }

                start_data2 = std::chrono::steady_clock::now();
                for (unsigned j = 1; j < max_in_num; j++)
                {
                    unsigned loop_num = 1;
                    int block_num1, block_num2, block_num;
                    int _blocks_per_inst;
                    unsigned int _merge_max_thread = max(merge_max_thread[0], merge_max_thread[j]);
                    if (inst_num / 2 *((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK) / _MAX_BLOCKS > _MAX_BLOCKS)
                    {
                        unsigned num = inst_num;
                        unsigned temp = inst_num*((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK);
                        while(temp > _MAX_BLOCKS)
                        {
                            num = (inst_num + loop_num) / (loop_num + 1);
                            temp = num*((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK);
                            loop_num++;
                        }
                        int every_loop_num = (inst_num + loop_num - 1) / loop_num;
                        int cur_inst_num = inst_num;
                        int inst_id_start = 0;
                        while(loop_num > 0)
                        {
                            if (loop_num == 1)
                            {
                                every_loop_num = cur_inst_num;
                            }
                            _blocks_per_inst = (_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK;
                            block_num = every_loop_num*_blocks_per_inst;
                            PreCudaMerge<<<block_num, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, dev_in_datas, _blocks_per_inst, _cur_level, inst_id_start, (inst_num + 1) / 2, inst_num, 
                                dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                                dev_temp_times, dev_temp_datas);
                            cudaDeviceSynchronize();
                            err = cudaGetLastError();
                            if (err != cudaSuccess)
                            {
                                cout << "cuda merge kenerl error: " << cudaGetErrorString(err) << endl;
                            }  
                            CudaMerge<<<block_num, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, _blocks_per_inst, _cur_level, inst_id_start, (inst_num + 1) / 2, inst_num, 
                                dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                                dev_temp_times, dev_temp_datas);
                            cudaDeviceSynchronize();
                            err = cudaGetLastError();
                            if (err != cudaSuccess)
                            {
                                cout << "cuda merge2 kenerl error: " << cudaGetErrorString(err) << endl;
                            }  
                            loop_num--;
                            cur_inst_num -= every_loop_num;
                            inst_id_start += every_loop_num;
                        }
                    }
                    else{
                        _blocks_per_inst = (_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK;
                        block_num2 = (inst_num * _blocks_per_inst + _MAX_BLOCKS - 1) / _MAX_BLOCKS;
                        block_num1 = (inst_num * _blocks_per_inst + block_num2 - 1) / block_num2;
                        dim3 grid(block_num1, block_num2, 1);
                        //cout << "block_num: " << block_num1 * block_num2 << ", blocks_per_inst: " << _blocks_per_inst << endl;
                        int inst_id_start = 0;
                        PreCudaMerge<<<grid, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, dev_in_datas, _blocks_per_inst, _cur_level, inst_id_start, (inst_num + 1) / 2, inst_num, 
                            dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                            dev_temp_times, dev_temp_datas);
                        cudaDeviceSynchronize();
                        err = cudaGetLastError();
                        if (err != cudaSuccess)
                        {
                            cout << "cuda merge kenerl error: " << cudaGetErrorString(err) << endl;
                        }     
                        CudaMerge<<<grid, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, _blocks_per_inst, _cur_level, inst_id_start, (inst_num + 1) / 2, inst_num, 
                            dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                            dev_temp_times, dev_temp_datas);
                        cudaDeviceSynchronize();
                        err = cudaGetLastError();
                        if (err != cudaSuccess)
                        {
                            cout << "cuda merge2 kenerl error: " << cudaGetErrorString(err) << endl;
                        }  
                    }
                }
                end_data2 = std::chrono::steady_clock::now();
                duration_data2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_data2 - start_data2).count();
                duration_malloc2 += duration_data2;
            //
                cudaFree(dev_temp_times);
                cudaFree(dev_temp_datas);
                cudaFree(dev_in_datas);
            }
            else{
                //short *dev_in_datas;
                //err = cudaMalloc((void **)&dev_in_datas, sizeof(short)*data_width);
                if(err!=cudaSuccess)
                {
                    printf("the cudaMalloc(in data) on GPU is failed");
                    exit(-1);
                }
                // GPU Merge sort
                unsigned int *dev_temp_times;
                short *dev_temp_datas;
                err=cudaMalloc((void **)&dev_temp_times, sizeof(unsigned int)*data_width);
                err = cudaMalloc((void **)&dev_temp_datas, sizeof(short)*input_width);
                //err=cudaMemcpy(dev_in_datas, input_datas, sizeof(short)*data_width, cudaMemcpyHostToDevice);
                if(err!=cudaSuccess)
                {
                    printf("the cudaMemcpy(dev_in_datas) on GPU is failed\n");
                    cout << cudaGetErrorString(err) << endl;
                    exit(-1);
                }

                auto start_data2 = std::chrono::steady_clock::now();
                for (unsigned j = 1; j < max_in_num; j++)
                {
                    unsigned loop_num = 1;
                    int block_num1, block_num2, block_num;
                    int _blocks_per_inst;
                    unsigned int _merge_max_thread = max(merge_max_thread[0], merge_max_thread[j]);
                    if (inst_num / 2 *((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK) / _MAX_BLOCKS > _MAX_BLOCKS)
                    {
                        unsigned num = inst_num;
                        unsigned temp = inst_num*((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK);
                        while(temp > _MAX_BLOCKS)
                        {
                            num = (inst_num + loop_num) / (loop_num + 1);
                            temp = num*((_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK);
                            loop_num++;
                        }
                        int every_loop_num = (inst_num + loop_num - 1) / loop_num;
                        int cur_inst_num = inst_num;
                        int inst_id_start = 0;
                        while(loop_num > 0)
                        {
                            if (loop_num == 1)
                            {
                                every_loop_num = cur_inst_num;
                            }
                            _blocks_per_inst = (_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK;
                            block_num = every_loop_num*_blocks_per_inst;
                            PreCudaMerge<<<block_num, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, dev_in_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, inst_num, 
                                dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                                dev_temp_times, dev_temp_datas);
                            cudaDeviceSynchronize();
                            err = cudaGetLastError();
                            if (err != cudaSuccess)
                            {
                                cout << "cuda merge kenerl error: " << cudaGetErrorString(err) << endl;
                            }  
                            CudaMerge<<<block_num, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, inst_num, 
                                dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                                dev_temp_times, dev_temp_datas);
                            cudaDeviceSynchronize();
                            err = cudaGetLastError();
                            if (err != cudaSuccess)
                            {
                                cout << "cuda merge2 kenerl error: " << cudaGetErrorString(err) << endl;
                            }  
                            loop_num--;
                            cur_inst_num -= every_loop_num;
                            inst_id_start += every_loop_num;
                        }
                    }
                    else{
                        _blocks_per_inst = (_merge_max_thread + _N_THREADS_PER_BLOCK - 1) / _N_THREADS_PER_BLOCK;
                        block_num2 = (inst_num * _blocks_per_inst + _MAX_BLOCKS - 1) / _MAX_BLOCKS;
                        block_num1 = (inst_num * _blocks_per_inst + block_num2 - 1) / block_num2;
                        dim3 grid(block_num1, block_num2, 1);
                        int inst_id_start = 0;
                        PreCudaMerge<<<grid, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, dev_in_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, inst_num, 
                            dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                            dev_temp_times, dev_temp_datas);
                        cudaDeviceSynchronize();
                        err = cudaGetLastError();
                        if (err != cudaSuccess)
                        {
                            cout << "cuda merge kenerl error: " << cudaGetErrorString(err) << endl;
                        }     
                        CudaMerge<<<grid, _N_THREADS_PER_BLOCK>>>(j, dev_times, dev_datas, _blocks_per_inst, _cur_level, inst_id_start, 0, inst_num, 
                            dev_total_times_start, dev_input_start, dev_data_val_num, dev_val_num_start, dev_data_in_num_start, dev_data_in_num,
                            dev_temp_times, dev_temp_datas);
                        cudaDeviceSynchronize();
                        err = cudaGetLastError();
                        if (err != cudaSuccess)
                        {
                            cout << "cuda merge2 kenerl error: " << cudaGetErrorString(err) << endl;
                        }  
                    }
                }
                auto end_data2 = std::chrono::steady_clock::now();
                long duration_data2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_data2 - start_data2).count();
                duration_malloc2 += duration_data2;
cout << "merge runtime (ms):" << duration_data2 << "--max_in_num:" << max_in_num << endl;
                cudaFree(dev_temp_times);
                cudaFree(dev_temp_datas);
                cudaFree(dev_in_datas);
            }
            cudaFree(dev_data_val_num);
            cudaFree(dev_val_num_start);
            delete[] total_input_times;
            delete[] input_datas;
            free(input_datas_val_num);


            auto start_malloc = std::chrono::steady_clock::now();

            unsigned int *dev_output_start;
            err=cudaMalloc((void **)&dev_output_start, sizeof(unsigned int)*height);
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_output_start) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }

            unsigned int *dev_output_size;
            err=cudaMalloc((void **)&dev_output_size, sizeof(unsigned int)*height);
            //err=cudaMalloc((void **)&dev_output_size2, sizeof(unsigned int)*height);
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_output_size) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            
            
            unsigned int *dev_out_times;
            short *dev_out_datas;  // 2D
            short *dev_out_splited_width;
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_input_start) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
        

        
            err=cudaMalloc((void **)&dev_out_splited_width, sizeof(short)*height);
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_out_splited_width) on GPU is failed, return value is %d\n", err);
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            
            
            err=cudaMalloc((void **)&dev_out_times, sizeof(unsigned int)*out_width);
            
            //cout << "out_width: " << out_width << ", height: " << height << endl;
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(dev_out_times) on GPU is failed");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            
            
            
            err = cudaMalloc((void **)&dev_out_datas, sizeof(short)*out_width);
            
            if(err!=cudaSuccess)
            {
                printf("the cudaMalloc(out data) on GPU is failed");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            //printf("SUCCESS\n");
            auto end_malloc = std::chrono::steady_clock::now();
            long duration_malloc = std::chrono::duration_cast<std::chrono::milliseconds>(end_malloc - start_malloc).count();
            //cout << "total time of simulating: " << duration_malloc << "ms" << endl;
            //printf("start copy memory\n");
            auto start_cpy = std::chrono::steady_clock::now();
            
            err = cudaMemset(dev_out_splited_width, tmpp, sizeof(short)*height);
            //err = cudaMemcpy(dev_out_splited_width, &host_out_splited_width[0], sizeof(int)*height, cudaMemcpyHostToDevice);
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_out_splited_width) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            err = cudaMemcpy(dev_output_start, &output_start[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            //err = cudaMemcpy(dev_output_start2, &output_start2[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_output_start) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            err = cudaMemcpy(dev_output_size, &output_size[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            //err = cudaMemcpy(dev_output_size2, &output_size2[0], sizeof(unsigned int)*height, cudaMemcpyHostToDevice);
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_output_size) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            
            
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_input_start) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            
            

            
            err = cudaMemset(dev_out_datas, tmpp, sizeof(short)*out_width);
            //err=cudaMemcpy(dev_out_datas, host_out_data, sizeof(int)*out_width*height, cudaMemcpyHostToDevice);  
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_out_datas) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }          
            
            unsigned int ttmmpp = 0;
            err = cudaMemset(dev_out_times, ttmmpp, sizeof(unsigned int)*out_width);
            //err=cudaMemcpy(dev_out_times, out_times, sizeof(int)*out_width*height, cudaMemcpyHostToDevice);
            if(err!=cudaSuccess)
            {
                printf("the cudaMemcpy(dev_out_times) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            
            unsigned int *dev_valid_width;
            unsigned int *dev_valid_width_start;
            unsigned int *dev_valid_width_size;
            unsigned int ch_init = 0;
            err = cudaMalloc((void**)&dev_valid_width, sizeof(unsigned int) * length_of_valid_width);
            err = cudaMalloc((void**)&dev_valid_width_start, sizeof(unsigned int) * inst_num);
            err = cudaMalloc((void**)&dev_valid_width_size, sizeof(unsigned int) * inst_num);
            err = cudaMemset(dev_valid_width, ch_init, sizeof(unsigned int)*length_of_valid_width);
            err = cudaMemcpy(dev_valid_width_start, &valid_width_start[0], sizeof(unsigned int)*inst_num, cudaMemcpyHostToDevice);
            err = cudaMemcpy(dev_valid_width_size, &valid_width_size[0], sizeof(unsigned int)*inst_num, cudaMemcpyHostToDevice);

            auto end_cpy = std::chrono::steady_clock::now();
            long duration_cpy = std::chrono::duration_cast<std::chrono::milliseconds>(end_cpy - start_cpy).count();
            auto end_data = std::chrono::steady_clock::now();
            //cout << "total time of memcpy: " << duration_cpy << "ms" << endl;
            duration_cuda += (duration_malloc + duration_cpy);
            duration_data += std::chrono::duration_cast<std::chrono::milliseconds>(end_data - start_data).count();
            // GPU parallel
            //printf("Start simulate with GPU\n");
            //int num_thread = N_THREADS_PER_BLOCK;
            auto start_gpu = std::chrono::steady_clock::now();
            SimulateCuda<<<n_grid,N_THREADS_PER_BLOCK>>>(_cur_level,blocks_per_inst, dev_datas,dev_out_datas,dev_times,dev_out_times,//dev_data_val_num,dev_in_times,
                dev_functions,dev_delay_edges,dev_in_bit,dev_out_bit,dev_rise_val,dev_fall_val,
                _time_unit,dev_total_times_start,dev_total_times_sizes,dev_data_in_num,dev_data_out_num,dev_delay_val_num,dev_functions_func_num,dev_functions_val_num,dev_input_start,//dev_val_num_start,
                //functions_width,delay_width,
                dev_out_splited_width,dev_output_start,dev_output_size, dev_data_in_num_start,
                dev_data_out_num_start,dev_delay_start,dev_delay_width,dev_functions_start,dev_functions_width,
                dev_valid_width, dev_valid_width_start, dev_valid_width_size);
            if(_cur_level == 0){
                DumpSaif(outf, time_unit, time_res, root_name, sim_start, sim_end);
                //DumpSaif(outf, parser.in_Result);
            }
            DumpSaif(outf, temp_Result);
            //else{
                //DumpSaif(outf, temp_Result);
            if(_cur_level == _size1 - 1){
                DumpSaif(outf, _inter.p.Result, sim_start, sim_end);
            }
            //}
            cudaDeviceSynchronize();
            auto end_gpu = std::chrono::steady_clock::now();
            duration_gpu += std::chrono::duration_cast<std::chrono::milliseconds>(end_gpu - start_gpu).count();
            err = cudaGetLastError();
            if (err != cudaSuccess)
            {
                cout << "kenerl error: " << cudaGetErrorString(err) << endl;
            }
            //unsigned int *host_valid_width1 = (unsigned int*)malloc(sizeof(unsigned int) * length_of_valid_width);
            //err = cudaMemcpy(host_valid_width1, dev_valid_width, sizeof(unsigned int)*length_of_valid_width, cudaMemcpyDeviceToHost);
            

            free(input_start);
            cudaFree(dev_total_times_start);//
            cudaFree(dev_total_times_sizes);//
            cudaFree(dev_input_start);//
            err=cudaFree(dev_times);//
            if (err != cudaSuccess)
            {
                printf("the cudafree(dev_times) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            err=cudaFree(dev_datas);//
            //err=cudaFree(dev_in_datas);//
            if (err != cudaSuccess)
            {
                printf("the cudafree(dev_datas) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }


            // GPU merge output data
            //printf("Start copy memory to host\n");
            short *dev_valid_width_flag;
            err = cudaMalloc((void**)&dev_valid_width_flag, sizeof(short) * length_of_valid_width);
            err = cudaMemset(dev_valid_width_flag, 0, sizeof(short)*length_of_valid_width);
            auto start_out = std::chrono::steady_clock::now();
            short *out_data = (short *)malloc(sizeof(short)*out_width);
            //short *out_data = (short *)malloc(sizeof(short)*ceil(out_width / STATE_NUM));
            unsigned int *out_time = (unsigned int *)malloc(sizeof(unsigned int)*out_width);
            ProcessConflictCuda<<<n_grid,N_THREADS_PER_BLOCK>>>(_cur_level,blocks_per_inst, dev_out_datas, dev_out_times,
                _time_unit, dev_data_out_num,
                dev_out_splited_width, dev_output_start, dev_output_size, 
                dev_data_in_num_start, 
                dev_valid_width, dev_valid_width_start, dev_valid_width_size,
                dev_valid_width_flag);
            cudaDeviceSynchronize();
            err = cudaGetLastError();
            if (err != cudaSuccess)
            {
                cout << "memcpy GPU to CPU kenerl error: " << cudaGetErrorString(err) << endl;
            }
            //unsigned int *host_valid_width2 = (unsigned int*)malloc(sizeof(unsigned int) * length_of_valid_width);
            //err = cudaMemcpy(host_valid_width2, dev_valid_width, sizeof(unsigned int)*length_of_valid_width, cudaMemcpyDeviceToHost);
            

            UpdateValidWidthGPU<<<n_grid,N_THREADS_PER_BLOCK>>>(_cur_level,blocks_per_inst, dev_out_datas,
                dev_data_out_num,
                dev_out_splited_width, dev_output_start, dev_output_size, 
                dev_data_in_num_start, 
                dev_valid_width, dev_valid_width_start, dev_valid_width_size,
                dev_valid_width_flag);
            cudaDeviceSynchronize();
            err = cudaGetLastError();
            if (err != cudaSuccess)
            {
                cout << "UpdateValidWidthGPU kenerl error: " << cudaGetErrorString(err) << endl;
            }
            //unsigned int *host_valid_width3 = (unsigned int*)malloc(sizeof(unsigned int) * length_of_valid_width);
            //err = cudaMemcpy(host_valid_width3, dev_valid_width, sizeof(unsigned int)*length_of_valid_width, cudaMemcpyDeviceToHost);
            
            cudaFree(dev_valid_width_flag);
            // debug
            /*if(_cur_level == 0){
            char *valid_width = new char[length_of_valid_width];
            cudaMemcpy(valid_width, dev_valid_width, sizeof(char)*length_of_valid_width, cudaMemcpyDeviceToHost);
            for(int c = 0; c < length_of_valid_width; ++c){
                int _c = static_cast<int>(valid_width[c]);
                cout << _c << endl;
            }
            }*/
            // Get prefix_sum of dev_valid_width, using thrust::exclusive_scan
            unsigned int *dev_prefix_sum_of_valid_width;
            err = cudaMalloc((void**)&dev_prefix_sum_of_valid_width, sizeof(unsigned int) * length_of_valid_width);
            thrust::exclusive_scan(thrust::device, dev_valid_width, dev_valid_width+length_of_valid_width,  dev_prefix_sum_of_valid_width);
            // Done prefix_sum
            //unsigned int *host_prefix_sum_of_valid_width = (unsigned int*)malloc(sizeof(unsigned int) * length_of_valid_width);
            //err = cudaMemcpy(host_prefix_sum_of_valid_width, dev_prefix_sum_of_valid_width, sizeof(unsigned int)*length_of_valid_width, cudaMemcpyDeviceToHost);
            //if(_cur_level == 0){
            //cout << "valid_width:" << endl;
            //for(unsigned p = 0; p < length_of_valid_width; ++p){
            //    cout << host_valid_width1[p] << "," << host_valid_width2[p] << "," << host_valid_width3[p] << "--" << host_prefix_sum_of_valid_width[p] << endl;
            //}
            //}
            //free(host_valid_width1);
            //free(host_valid_width2);
            //free(host_valid_width3);
            //free(host_prefix_sum_of_valid_width);

            // Get new out_starts and out_sizes
            unsigned int *new_output_start = (unsigned int*)malloc(sizeof(unsigned int) * inst_num);
            unsigned int *new_output_size = (unsigned int*)malloc(sizeof(unsigned int) * inst_num * MAX_OUT_NUM);
            if(new_output_size == NULL){
                cout << "error malloc: new_output_size" << endl;
                exit(-1);
            }
            unsigned int *dev_out_width;
            unsigned int *dev_output_start_new;
            unsigned int *dev_output_size_new;
            
            err = cudaMalloc((void **)&dev_out_width, sizeof(unsigned int));
            err = cudaMalloc((void **)&dev_output_start_new, sizeof(unsigned int) * inst_num);
            err = cudaMalloc((void **)&dev_output_size_new, sizeof(unsigned int) * inst_num * MAX_OUT_NUM);
            if (err != cudaSuccess)
            {
                cout << "dev_output_size_new cudaMalloc error: " << cudaGetErrorString(err) << endl;
            }
            err = cudaMemset(dev_output_size_new, 0, sizeof(unsigned int) * inst_num * MAX_OUT_NUM);
            int _block = (inst_num + UPDATE_N_THREADS_PER_BLOCK - 1) / UPDATE_N_THREADS_PER_BLOCK;
            UpdateOutInfoGPU<<<_block, UPDATE_N_THREADS_PER_BLOCK>>>(dev_prefix_sum_of_valid_width, _cur_level,inst_num,
                dev_data_out_num,
                dev_out_splited_width, dev_output_start_new, dev_output_size_new, 
                dev_data_in_num_start, 
                dev_valid_width, dev_valid_width_start, dev_valid_width_size,
                dev_out_width);
            cudaDeviceSynchronize();
            err = cudaGetLastError();
            if (err != cudaSuccess)
            {
                cout << "UpdateOutInfoGPU kenerl error: " << cudaGetErrorString(err) << endl;
            }
            err = cudaMemcpy(&out_width, dev_out_width, sizeof(unsigned int), cudaMemcpyDeviceToHost);
            err = cudaMemcpy(new_output_start, dev_output_start_new, sizeof(unsigned int) * inst_num, cudaMemcpyDeviceToHost);
            err = cudaMemcpy(new_output_size, dev_output_size_new, sizeof(unsigned int)  * inst_num * MAX_OUT_NUM, cudaMemcpyDeviceToHost);
            //cout << "new out_width:" << out_width << endl;
            //if(_cur_level == 0){
            //cout << "new_output_start/size:" << endl;
            //for(int p = 0; p < inst_num; ++p){
            //    cout << new_output_start[p] << "--" << new_output_size[p*MAX_OUT_NUM] << "," << new_output_size[p*MAX_OUT_NUM+1] << endl;
            //}
            //}
            // Remove invalid state
            unsigned int *dev_out_times_new;
            short *dev_out_datas_new;
            err = cudaMalloc((void **)&dev_out_times_new, sizeof(unsigned int) * out_width);
            err = cudaMalloc((void **)&dev_out_datas_new, sizeof(short) * out_width);
            RemoveInvalidStatesGPU<<<n_grid,N_THREADS_PER_BLOCK>>>(dev_out_times_new, dev_out_datas_new,
                _cur_level,blocks_per_inst, dev_out_datas, dev_out_times,
                dev_data_out_num,
                dev_out_splited_width, dev_output_start, dev_output_size, 
                dev_data_in_num_start, 
                dev_valid_width, dev_valid_width_start, dev_valid_width_size,
                dev_prefix_sum_of_valid_width);
            cudaDeviceSynchronize();
            err = cudaGetLastError();
            if (err != cudaSuccess)
            {
                cout << "RemoveInvalidStatesGPU kenerl error: " << cudaGetErrorString(err) << endl;
            }
            //unsigned int* host_out_times = new unsigned int[out_width];
            //short* host_out_datas = new short[out_width];
            //err = cudaMemcpy(host_out_times, dev_out_times_new, sizeof(unsigned int) * out_width, cudaMemcpyDeviceToHost);
            //err = cudaMemcpy(host_out_datas, dev_out_datas_new, sizeof(short) * out_width, cudaMemcpyDeviceToHost);
            //if(_cur_level == 0){
            //cout << "new_output:" << endl;
            //for(unsigned p = 0; p < out_width; ++p){
            //    cout << host_out_times[p] << "," << host_out_datas[p] << endl;
            //}
            //}
            //delete[] host_out_times;
            //delete[] host_out_datas;

            cudaFree(dev_output_start);
            cudaFree(dev_output_size);
            cudaFree(dev_output_start_new);
            cudaFree(dev_output_size_new);
            err=cudaFree(dev_out_datas);//
            
            if (err != cudaSuccess)
            {
                printf("the cudafree(dev_out_datas) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            cudaFree(dev_out_times);//
            if (err != cudaSuccess)
            {
                printf("the cudafree(dev_out_times) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            // Done Remove

            short *dev_out_datas_compressed;
            unsigned int dev_out_width_compressed = (out_width + STATE_NUM - 1) / STATE_NUM;
            err = cudaMalloc((void **)&dev_out_datas_compressed, sizeof(short)*dev_out_width_compressed);
            CompressGPU<<<_n_grid, N_THREADS_PER_BLOCK>>>(dev_out_datas_new, dev_out_datas_compressed, out_width, dev_out_width_compressed);
            cudaDeviceSynchronize();
            if (err != cudaSuccess)
            {
                cout << "CompressGPU kenerl error: " << cudaGetErrorString(err) << endl;
            }
            auto end_out2 = std::chrono::steady_clock::now();
            cudaMemcpy(out_time, dev_out_times_new, sizeof(unsigned int)*out_width, cudaMemcpyDeviceToHost);
            //cudaMemcpy(out_data, dev_out_datas, sizeof(short)*out_width, cudaMemcpyDeviceToHost);
            cudaMemcpy(out_data, dev_out_datas_compressed, sizeof(short)*dev_out_width_compressed, cudaMemcpyDeviceToHost);
            if (err != cudaSuccess)
            {
                cout << "cudaMemcpy dev_out_datas_compressed error: " << cudaGetErrorString(err) << endl;
            }
            auto end_out = std::chrono::steady_clock::now();
            duration_out += std::chrono::duration_cast<std::chrono::milliseconds>(end_out2 - start_out).count();
            duration_out2 += std::chrono::duration_cast<std::chrono::milliseconds>(end_out - start_out).count();

            
            out_times[_cur_level] = out_time;
            out_values[_cur_level] = out_data;
            out_sizes[_cur_level] = new_output_size;
            out_starts[_cur_level] = new_output_start;
            cudaFree(dev_out_datas_compressed);
            //cudaFree(dev_output_size);
            //cudaFree(dev_output_start);
            cudaFree(dev_valid_width_start);
            cudaFree(dev_valid_width_size);
            cudaFree(dev_out_splited_width);//
            cudaFree(dev_valid_width);//
            cudaFree(dev_prefix_sum_of_valid_width);//
            cudaFree(dev_out_width);//
            free(valid_width_start);
            free(valid_width_size);
            free(output_size);
            free(output_start);
            
            
            err=cudaFree(dev_out_datas_new);//
            
            if (err != cudaSuccess)
            {
                printf("the cudafree(dev_out_datas) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
            cudaFree(dev_out_times_new);//
            
            if (err != cudaSuccess)
            {
                printf("the cudafree(dev_out_datas) on GPU is failed\n");
                cout << cudaGetErrorString(err) << endl;
                exit(-1);
            }
        }
        err = cudaFree(dev_data_in_num);//
        checkError(err, "cudafree error");
        //cudaFree(dev_val_num_start);//
        cudaFree(dev_data_out_num);//
        cudaFree(dev_delay_val_num);//
        cudaFree(dev_functions_func_num);//
        cudaFree(dev_functions_val_num);//
        cudaFree(dev_delay_edges);//
        cudaFree(dev_in_bit);//
        cudaFree(dev_out_bit);//
        cudaFree(dev_rise_val);//
        cudaFree(dev_fall_val);//
        cudaFree(dev_functions);//
        cudaFree(dev_data_in_num_start);//
        cudaFree(dev_data_out_num_start);//
        cudaFree(dev_delay_start);//
        cudaFree(dev_delay_width);//
        cudaFree(dev_functions_start);//
        cudaFree(dev_functions_width);//
        auto end = std::chrono::steady_clock::now();
        long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        cout << "total time of simulating: " << duration << "ms" << endl;
        cout << "\ttotal time of gpu: " << duration_gpu << "ms(" << (float)((float)duration_gpu / duration) * 100.0 << "%)" << endl;
        cout << "\ttotal time of pre process: " << duration_pre << "ms(" << (float)((float)duration_pre / duration) * 100.0 << "%)" << endl;
        cout << "\t\ttime of first step: " << duration_pro0 << "ms(" << (float)((float)duration_pro0 / duration) * 100.0 << "%)" << endl;
        cout << "\t\ttime of second step: " << duration_pro00 << "us(" << (float)((float)duration_pro1 / duration) * 100.0 << "%)" << endl;
        cout << "\ttotal time of function/delay copy: " << duration_cuda2 << "ms(" << (float)((float)duration_cuda2 / duration) * 100.0 << "%)" << endl;        
        cout << "\ttotal time of data copy: " << duration_data << "ms(" << (float)((float)duration_data / duration) * 100.0 << "%)" << endl;
        cout << "\t\tmerge gpu: " << duration_malloc2 << "ms(" << (float)((float)duration_malloc2 / duration) * 100.0 << "%)" << endl;
        cout << "\t\ttotal time of data copy(not include pre process's data): " << duration_cuda << "ms(" << (float)((float)duration_cuda / duration) * 100.0 << "%)" << endl;
        cout << "\ttotal time of out process: " << duration_out2 << "ms(" << (float)((float)duration_out2 / duration) * 100.0 << "%)" << endl;
        cout << "\t\ttotal time of out process(not include out data cpy): " << duration_out << "ms(" << (float)((float)duration_out / duration) * 100.0 << "%)" << endl;
        auto start_write = std::chrono::steady_clock::now();
        //DumpSaif(outf);
        DumpSaif(outf, OutMaps, out_times, out_values, out_sizes, out_starts,
            sim_start, sim_end);
        DumpSaif(outf, parser.InMaps_bus, parser.InMaps_bit, parser.times, parser.values, parser.sizes,
            parser.vec_times, parser.vec_values, parser.vec_sizes,
            parser.hash_name_pair, parser.hash_index_pair,
            sim_start, sim_end);
        DumpSaif(outf, assign_pairs, _inter.p.Result,
            sim_start, sim_end);
        DumpSaif(outf, sim_start, sim_end);

        outf.close();
        auto end_write = std::chrono::steady_clock::now();
        long duration_write = std::chrono::duration_cast<std::chrono::milliseconds>(end_write - start).count();
        long duration_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_write - start_vcd).count();
        long duration_total2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_write - start_total).count();
        cout << "total time(not including read-vcd): " << duration_write << "ms" << endl;
        cout << "total time: " << duration_total << "ms" << endl;
        cout << "total time(all): " << duration_total2 << "ms" << endl;
        //DumpSaif(parser.times,parser.values, assign_pairs, out_times,out_values,out_sizes,out_starts, pinbitValues, _inter.p.pin_bits, saif_out, time_unit, time_res, root_name, sim_start, sim_end, _size1, _inter.levels, _inter);
    }
    else
    {
        std::cout << "Parse Failed." << std::endl;
        return 1;
    }
    return 0;
}