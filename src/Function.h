
#ifndef Function_H
#define Function_H

#include <iostream>

#include "VCDTypes.h"

/*!
@brief Function Class, including UDP(User Defined Primitive)
@details 
*/
class Function {

    public:
        
        /*!
        @brief Create a new Instance
        */
        Function(){}
        Function(std::string _name, int _id){name = _name;id = _id;}

        ~Function(){}
        void set_in(std::string pin){
            input_pins.push_back(pin);
        }

        void set_out(std::string pin){
            output_pins.push_back(pin);
        }

        void set_in_data(VCDBit data){
            input_data.push_back(data);
        }

        VCDBit get_output_data(){return output_data;}

        std::string get_output(){return output_pins[0];}


        std::string   name;
        int id;

        std::vector<std::string> input_pins;    
        std::vector<VCDBit> input_data;    
        std::vector<std::string> output_pins;

        VCDBit output_data;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) //
        {
            //ar.register_type(static_cast<ModuleType *>(NULL));
            ar & name;
            ar & id;
            ar & input_pins;
            //ar & out_port_net;
            //ar & unknown_in;
            ar & input_data;
            ar & output_pins;
            ar & output_data;
        }
        
       
};
/*
class Primitive: public Function
{
public:
    Primitive(std::string _name):Function(_name){};
    ~Primitive();

    void set_table(std::vector<std::string> table_line){
        table.push_back(table_line);
    }

    std::vector<std::vector<std::string> > table;
};

class And: public Function
{
public:
    And(std::string _name):Function(_name){};
    ~And();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() > 1);
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
};

class Or: public Function
{
public:
    Or(std::string _name):Function(_name){};
    ~Or();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() > 1);
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
};

class Xor: public Function
{
public:
    Xor(std::string _name):Function(_name){};
    ~Xor();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() > 1);
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
};

class Xnor: public Function
{
public:
    Xnor(std::string _name):Function(_name){};
    ~Xnor();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() > 1);
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = ~(input_data[0] ^ input_data[1]);
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
                    if (input_data[i] == VCD_0 || input_data[i] == VCD_1)
                    {
                        tmp = ~(tmp ^ input_data[i]);
                    }
                    else{
                        tmp = VCD_X;
                    }
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
                    tmp = VCD_X;
                    break;
                //Z
                case(3):
                    tmp = (input_data[i] == VCD_Z)?VCD_0:VCD_X;
                    break;
                default:
                    tmp = VCD_X;
                    break;
            }
        }
        output_data = VCDBit(tmp);
    }
};

class Nor: public Function
{
public:
    Nor(std::string _name):Function(_name){};
    ~Nor();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() > 1);
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = ~(input_data[0] | input_data[1]);
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[0] == VCD_Z)
            {
                if (input_data[1] == VCD_1)
                {
                    tmp = VCD_0;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else if (input_data[1] == VCD_X || input_data[1] == VCD_Z)
            {
                if (input_data[0] == VCD_1)
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
                    if (input_data[i] == VCD_1 || input_data[i] == VCD_0)
                    {
                        tmp = ~(tmp | input_data[i]);
                    }
                    else{
                        tmp = VCD_X;
                    }
                    break;
                case(1):
                    tmp = VCD_0;
                    break;
                //X
                case(2):
                //Z
                case(3):
                    if (input_data[i] == VCD_1)
                    {
                        tmp = VCD_0;
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
};

class Nand: public Function
{
public:
    Nand(std::string _name):Function(_name){};
    ~Nand();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() > 1);
        int tmp;
        if (input_data[0] != VCD_X && input_data[1] != VCD_X && input_data[0] != VCD_Z && input_data[1] != VCD_Z)
        {
            tmp = ~(input_data[0] & input_data[1]);
        }
        else
        {
            if (input_data[0] == VCD_X || input_data[0] == VCD_Z)
            {
                if (input_data[1] == VCD_0)
                {
                    tmp = VCD_1;
                }
                else{
                    tmp = VCD_X;
                }
            }
            else if (input_data[1] == VCD_X || input_data[1] == VCD_Z)
            {
                if (input_data[0] == VCD_0)
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
                    tmp = VCD_1;
                    break;
                case(1):
                    if (input_data[i] == VCD_1 || input_data[i] == VCD_0)
                    {
                        tmp = ~(tmp & input_data[i]);
                    }
                    else{
                        tmp = VCD_X;
                    }
                    break;
                //X
                case(2):
                //Z
                case(3):
                    if (input_data[i] == VCD_0)
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
};

class Buf: public Function
{
public:
    Buf(std::string _name):Function(_name){};
    ~Buf();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() == 1);
        output_data = VCDBit(input_data[0] == VCD_Z ? VCD_X:input_data[0]);
    }
};

class Not: public Function
{
public:
    Not(std::string _name):Function(_name){};
    ~Not();

    void calc(){
        assert(input_pins.size() == input_data.size());
        assert(input_pins.size() == 1);
        int tmp;
        tmp = (input_data[0] == VCD_0 || input_data[0] == VCD_1)?(~input_data[0]):VCD_X;
        output_data = VCDBit(tmp);
    }
};

class Udp_xbuf: public Function
{
public:
    Udp_xbuf(std::string _name):Function(_name){};
    ~Udp_xbuf();

    void calc(){
        assert(input_data.size() == 2);
        assert(input_data[1] == VCD_1);
        assert(input_data[0] != VCD_Z);
        output_data = VCDBit(input_data[0] == VCD_X ? VCD_1:input_data[0]);
    }
};

class Udp_mux2: public Function
{
public:
    Udp_mux2(std::string _name):Function(_name){};
    ~Udp_mux2();

    void calc(){
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
            assert(input_data[0] == input_data[1]);
            output_data = input_data[0];
        }
        else
        {
            output_data = VCD_X;
        }
    }
};
*/
#endif
