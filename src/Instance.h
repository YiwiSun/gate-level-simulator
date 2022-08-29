
#ifndef Instance_H
#define Instance_H

#include "VCDTypes.h"
#include "Function.h"

/*!
@brief Represents a single value found in a VCD File.
@details Can contain a single bit (a scalar), a bti vector, or an
IEEE floating point number.
*/
class Instance {

    public:
        
        /*!
        @brief Create a new Instance
        */
        Instance(){}
        Instance(std::string ins_name, ModuleType ins_type):name(ins_name), type(ins_type){

        }

        ~Instance () {
        }
        void set_in(std::string port, std::string net){
            in_port_net.insert(std::pair<std::string, std::string>(port, net));
        }

        void set_out(std::string port, std::string net){
            out_port_net.insert(std::pair<std::string, std::string>(port, net));
        }

        std::map<std::string, std::string> get_out(){return out_port_net;}
        std::map<std::string, std::string> get_in(){return in_port_net;}

        void set_unknown_in(std::string in_input){
            unknown_in.push_back(in_input);
        }

        void set_unknown_out(std::string in_input){
            unknown_out.push_back(in_input);
        }

        void set_delay(Delay delay){
            _delay.push_back(delay);
        }
        std::vector<Delay> get_delay(){return _delay;}
        
        void set_function(std::vector<Function> _function_vec){
            function_vec = _function_vec;
        }
        std::vector<Function> get_function(){return function_vec;}

        //GET
        std::string get_name(){return name;}

        //judge
        bool has_input(std::string net)
        {
            for(std::map<std::string, std::string>::iterator i = in_port_net.begin();
             i != in_port_net.end();
             ++i)
            {
                if (i->second == net)
                {
                    return true;
                }
            }
            return false;
        }

        void add_in_net_from(int m, std::string str, int n, int k, int out_order)
        {
            in_net_from_id.push_back(m);
            in_net_from_info.push_back(str);
            in_net_from_level.push_back(n);
            in_net_from_pos_at_level.push_back(k);
            in_net_from_out_order.push_back(out_order);
        }


        std::string   name;
        //! The type of instance
        ModuleType   type;
        //! .port_name(pinbit)
        std::map<std::string, std::string> in_port_net;  
        std::vector<std::string> in_net;
        std::map<std::string, std::string> out_port_net;
        std::vector<std::string> out_net;

        std::vector<std::string> unknown_in;    
        std::vector<std::string> unknown_out;

        std::vector<Function> function_vec;
        std::vector<std::vector<int> > function_id_vec;

        std::vector<Delay>     _delay;    

        std::vector<int>     in_net_from_id;    
        std::vector<std::string>     in_net_from_info;    
        std::vector<int>     in_net_from_level;    
        std::vector<int>     in_net_from_pos_at_level;    
        std::vector<int>     in_net_from_out_order;    

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) //
        {
            //ar.register_type(static_cast<ModuleType *>(NULL));
            ar & type;
            ar & name;
            ar & in_port_net;
            ar & in_net;
            ar & out_port_net;
            ar & out_net;
            ar & unknown_in;
            ar & unknown_out;
            ar & function_vec;
            ar & function_id_vec;
            ar & _delay;
            ar & in_net_from_id;
            ar & in_net_from_info;
            ar & in_net_from_level;
            ar & in_net_from_pos_at_level;
            ar & in_net_from_out_order;  //0 ,1
        }
       
};


#endif
