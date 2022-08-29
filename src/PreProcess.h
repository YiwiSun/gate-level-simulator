

#include <fstream>
#include <sstream>


#include "VCDTypes.h"
#include "VCDValue.h"
#include "Function.h"
#include "Instance.h"

#ifndef PreProcess_H
#define PreProcess_H

/*!
@brief Top level object to represent a single VCD file.
*/
class PreProcess {

    public:
        
        //! Instance a new PreProcess Class
        PreProcess();
        
        //! Destructor
        ~PreProcess();

        std::vector<std::string> Split(const std::string &str, const std::string &delim);
        
        void parse_vlib(std::string& vlib_path);
        void parse_gv(std::string& gv_path);
        void parse_sdf(std::string& sdf_path);
        void add_in_net_from(int m, std::string str, int n, int k, std::string name, int out_order){
            instances[name].add_in_net_from(m, str, n, k, out_order);
        }
        Instance* find_inst(int id){
            std::string name = instance_name_vec[id];
            return &(instances[name]);
        }
        //void parse_vcd(const char* vcd_path);

        //for generate Graph --- see Class Graph
        //void generate_graph();

        //for vlib
        void vlib_add_module(std::string s, ModuleType m){module_types[s] = m;}

        //for gv
        std::map<std::string, TimedValues*> get_pinbitValues(){return pinbitValues;}
        std::map<std::string, Instance> get_instances(){return instances;}
        std::vector<std::string> get_instance_names(){return instance_name_vec;}
        std::map<std::string, Pin> get_pins(){return pins;}
        std::map<std::string, std::string> get_assign_pairs(){return assign_pairs;}

        //vlib param
        std::map<std::string, ModuleType> module_types;   //name:ModuleType
        //std::map<std::string, Primitive*> primitives;   //name:Primitive
        std::vector<std::string> ignored_modules;   //DFF && LATCH && RAM ...

        //gv param
        std::string top_module_name;
        std::map<std::string, Instance> instances;   //name:Instance
        std::vector<std::string> instance_name_vec;
        std::map<std::string, std::vector<int> > net_instance_map;
        std::map<std::string, int> out_net_from_id;
        std::map<std::string, Pin> pins;   //name:Pin
        std::vector<std::string> pin_bits;   //name
        //PinBit : single bit pin
        std::map<std::string, TimedValues*> pinbitValues;   //name:TimedValues(PinBitValue), mainly for simulator, and here is for process 'assign'
        std::map<std::string, std::string> assign_pairs;   //assign_left:assign_right
        std::map<std::string, std::vector<unsigned long long> > Result;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) //
        {
            //ar.register_type(static_cast<ModuleType *>(NULL));
            ar & module_types;
            ar & ignored_modules;
            ar & top_module_name;
            ar & instances;
            ar & instance_name_vec;
            ar & net_instance_map;
            ar & out_net_from_id;
            ar & pins;
            ar & pin_bits;
            ar & pinbitValues;
            ar & assign_pairs;
            ar & Result;
        }


        
};


#endif
