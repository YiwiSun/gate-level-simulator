
#include <bits/stdc++.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <string>
#include <iostream>
#include <unordered_set>
#include "VCDParser.h"
#include "PreProcess.h"
#include "Instance.h"
#include "Function.h"
#include "VCDTypes.h"

#ifndef simulate_CUH
#define simulate_CUH
class inter;
void DumpSaif(std::ofstream &outf);
void DumpSaif(std::ofstream &outf, std::unordered_map<std::string, std::vector<unsigned long long> >& Result);
void DumpSaif(std::ofstream &outf, std::map<std::string, std::vector<unsigned long long> >& Result, VCDTime start, VCDTime end);
void DumpSaif(std::ofstream &outf, const VCDTimeUnit time_unit, const unsigned time_res, const std::string root_name, VCDTime start, VCDTime end);
void DumpSaif(std::ofstream &outf, std::vector<std::map<std::string, std::vector<unsigned int> > > &OutMaps,
    std::vector<unsigned int*> &out_times,std::vector<short*> &out_values,std::vector<unsigned int*> &out_sizes, std::vector<unsigned int*> &out_starts,
    VCDTime start, VCDTime end);
void DumpSaif(std::ofstream &outf, std::unordered_set<std::string> &InMaps_bus, std::unordered_set<std::string> &InMaps_bit, 
    std::unordered_map<std::string, std::vector<unsigned int> > &times, std::unordered_map<std::string, std::vector<short> > &values, 
    std::unordered_map<std::string, unsigned int> &sizes,
    std::vector<std::vector<unsigned int> > &vec_times, std::vector<std::vector<short> > &vec_values, std::vector<unsigned int> &vec_sizes,
    std::unordered_map<std::string, std::string> &hash_name_pair, std::unordered_map<std::string, unsigned int> &hash_index_pair,
    VCDTime start, VCDTime end);
void DumpSaif(std::ofstream &outf, std::map<std::string, std::string> &assign_pairs, 
    //std::unordered_map<std::string, std::vector<unsigned long long> >& inResult, 
    std::map<std::string, std::vector<unsigned long long> >& pinResult,
    VCDTime start, VCDTime end);
void DumpSaif(std::ofstream &outf, VCDTime start, VCDTime end);
__global__
void SimulateCuda(int cur_level,int blocks_per_inst, short *dev_datas, short *dev_out_datas,unsigned int *dev_times, unsigned int *dev_out_times,
                short *dev_functions, short *dev_delay_edges, short *dev_in_bit, short *dev_out_bit, float *dev_rise_val, float *dev_fall_val,
                int dev_time_unit, unsigned int *dev_total_times_start, unsigned int *dev_total_times_sizes, short *dev_data_in_num, short *dev_data_out_num, short *dev_delay_val_num,short *dev_functions_func_num, short *dev_functions_val_num, 
                unsigned int *dev_input_start, 
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, unsigned int *dev_data_in_num_start,
                unsigned int *dev_data_out_num_start, unsigned int *dev_delay_start, unsigned int *dev_delay_width, unsigned int *dev_functions_start, unsigned int *dev_functions_width,
                char *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size);
__device__
short CheckDelayCuda(short pre, short cur);
__device__ unsigned int cu_abs(unsigned int i, unsigned int j);
__global__
void DecompressTimesGPU(unsigned int *dev_times, unsigned short *dev_times_compressed, 
    unsigned int *dev_base_start, unsigned int *dev_base_start_start, 
    const unsigned int base_start_total, const unsigned int base_start_start_idx, const unsigned int total_times_sizes);
//__global__
//void DecompressGPU(short *dev_in_datas, short *dev_in_data_compressed,
//    unsigned int data_width);//, unsigned int data_width_compress, int cur_level);
__global__
void DecompressGPU(short *dev_in_datas, short *dev_in_data_compressed, 
    unsigned int *dev_step_start_vec, unsigned int *dev_com_step_start_vec, 
    unsigned int *dev_data_val_num, unsigned int blocks_per_signal, unsigned int sigal_num);
__global__
void CompressGPU(short *dev_in_datas, short *dev_in_data_compressed,
    unsigned int data_width, unsigned int data_width_compress);
__global__ void PreCudaMerge(unsigned int j, unsigned int *dev_times, short *dev_datas, 
    short *dev_in_datas, int _blocks_per_inst, int cur_level, int inst_id_start, int inst_num_start,int inst_num_end,  unsigned int *dev_total_times_start,
    unsigned int *dev_input_start, unsigned int *dev_data_val_num, unsigned int *dev_val_num_start, unsigned int *dev_data_in_num_start, short *dev_data_in_num,
    unsigned int *dev_temp_times, short *dev_temp_datas);
__global__ void CudaMerge(unsigned int j, unsigned int *dev_times, short *dev_datas, 
    int _blocks_per_inst, int cur_level, int inst_id_start, int inst_num_start,int inst_num_end,  unsigned int *dev_total_times_start,
    unsigned int *dev_input_start, unsigned int *dev_data_val_num, unsigned int *dev_val_num_start, unsigned int *dev_data_in_num_start, short *dev_data_in_num,
    unsigned int *dev_temp_times, short *dev_temp_datas);

__global__
void ProcessConflictCuda(int cur_level,int blocks_per_inst, short *dev_out_datas, unsigned int *dev_out_times,
    int dev_time_unit, short *dev_data_out_num,
    short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
    unsigned int *dev_data_in_num_start,
    char *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size,
    short *dev_valid_width_flag);
__global__
void UpdateValidWidthGPU(int cur_level,int blocks_per_inst, short *dev_out_datas,
                short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                char *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size,
                short *dev_valid_width_flag);
__global__
void UpdateOutInfoGPU(unsigned int *dev_prefix_sum_of_valid_width, int cur_level,int inst_num,
                short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size,
                unsigned int *dev_out_width);
__global__
void RemoveInvalidStatesGPU(unsigned int *dev_out_times_new, short *dev_out_datas_new,
                int cur_level, int blocks_per_inst, short *dev_out_datas, unsigned int *dev_out_times,
                short *dev_data_out_num,
                short *dev_out_splited_width, unsigned int *dev_output_start, unsigned int *dev_output_size, 
                unsigned int *dev_data_in_num_start,
                unsigned int *dev_valid_width, unsigned int *dev_valid_width_start, unsigned int *dev_valid_width_size,
                unsigned int *dev_prefix_sum_of_valid_width);

/*void processCore(int left, int right, int inst_num, inter &_inter, VCDParser &parser, VCDTime sim_start, VCDTime sim_end,
    unsigned int *total_input_times, char *input_datas,
    std::vector<int> &cur_level, std::vector<unsigned int*> &out_starts, std::vector<unsigned int*> &out_sizes,
    std::vector<unsigned int*> &out_times, std::vector<short*> &out_values, std::map<std::string, TimedValues*> &pinbitValues,
    std::vector<unsigned int> &total_times_start, std::unordered_map<std::string, bool> &cur_flag,
    std::unordered_map<std::string, std::vector<unsigned long long>> &temp_Result);*/
void processCore(int left, int right, int size, VCDTime sim_start, VCDTime sim_end,
    unsigned int *total_input_times, short *input_datas,
    std::vector<std::string> &name_vec, std::vector<unsigned int*> &time_vec, std::vector<short*> &value_vec, 
    unsigned int *step_vec, std::vector<bool> &is_init_vec, std::vector<short> &offset_vec, 
    std::vector<unsigned int> &step_start_vec, std::vector<unsigned int> &com_step_start_vec, 
    std::vector<bool> &cur_flag,
    std::unordered_map<std::string, std::vector<unsigned long long>> &temp_Result);
void checkError(cudaError_t error, std::string msg);

#endif