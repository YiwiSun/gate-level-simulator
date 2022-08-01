

#include <fstream>
#include <sstream>
#include <unordered_set>


#include "VCDTypes.h"
#include "VCDValue.h"

#ifndef VCDParser_H
#define VCDParser_H


/*!
@brief Top level object to represent a single VCD file.
*/
class VCDParser {

    public:
        
        //! Instance a new VCD file container.
        VCDParser();
        
        //! Destructor
        ~VCDParser();
        
        //! Timescale of the VCD file.
        VCDTimeUnit time_units;

        //! Multiplier of the VCD file time units.
        VCDTimeRes  time_resolution;
        
        //! Date string of the VCD file.
        std::string date;

        //! Version string of the simulator which generated the VCD.
        std::string version;

        //! Version string of the simulator which generated the VCD.
        //std::string comment;

        //! Root scope node of the VCD signals
        VCDScope * root_scope;

        //! times and values with name related
        std::unordered_map<std::string, std::vector<unsigned int> > times;
        std::unordered_map<std::string, std::vector<short> > values;
        std::unordered_map<std::string, unsigned int> sizes;
        //std::unordered_map<std::string, std::vector<std::string> > vec_values;
        //std::unordered_map<std::string, int> name_lindex_pair;
        unsigned int end_time;
        unsigned int num;

        std::vector<std::vector<unsigned int> > vec_times;
        std::vector<std::vector<short> > vec_values;
        std::vector<std::vector<char*> > vec_vec_values;
        std::vector<unsigned int> vec_sizes;

        // multi-threading
        std::vector<std::vector<std::vector<unsigned int> > > _vec_times;
        std::vector<std::vector<std::vector<short> > > _vec_values;
        std::vector<std::vector<std::vector<char*> > > _vec_vec_values;

        std::unordered_map<std::string, std::string> hash_name_pair;  //name:hash
        std::unordered_map<std::string, unsigned int> hash_index_pair;

        std::unordered_set<std::string> InMaps_bit;
        std::unordered_set<std::string> InMaps_bus;

        void init(std::string& vcdFilePath, VCDTime end);
        void process();
        friend void core(VCDParser &p, unsigned left, unsigned right, unsigned vec_values_size);
        friend void parse_core(VCDParser &p ,std::string& file_str, unsigned t, std::vector<unsigned long long>& total_len);
        friend void combine_core(VCDParser &p ,unsigned left, unsigned right, unsigned max_cores);

        /*!
        @brief Add a new scope object to the VCD file
        @param s in - The VCDScope object to add to the VCD file.
        */
        void add_scope(
            VCDScope * s
        );

        /*!
        @brief Add a new signal to the VCD file
        @param s in - The VCDSignal object to add to the VCD file.
        */
        void add_signal(
            VCDSignal * s
        );


        /*!
        @brief Add a new timestamp value to the VCD file.
        @details Add a time stamp to the sorted array of existing
        timestamps in the file.
        @param time in - The timestamp value to add to the file.
        */
        /*void add_timestamp(
            VCDTime time
        );*/
    

        /*!
        @brief Return the scope object in the VCD file with this name
        @param name in - The name of the scope to get and return.
        */
        VCDScope * get_scope(
            VCDScopeName name
        );


        /*!
        @brief Add a new signal value to the VCD file, tagged by time.
        @param time_val in - A signal value, tagged by the time it occurs.
        @param hash in - The VCD hash value representing the signal.
        */
        void add_signal_value(
            unsigned int time,
            VCDBit bit,
            std::string   hash
        );
        

        /*!
        @brief Get the value of a particular signal at a specified time.
        @note The supplied time value does not need to exist in the
        vector returned by get_timestamps().
        @param hash in - The hashcode for the signal to identify it.
        @param time in - The time at which we want the value of the signal.
        @param erase_prior in - Erase signals prior to this time. Avoids O(n^2) searching times when scanning large .vcd files sequentially.
        @returns A pointer to the value at the supplie time, or nullptr if
        no such record can be found.
        */
        VCDValue * get_signal_value_at (
            const VCDSignalHash& hash,
            VCDTime       time,
            bool erase_prior = false
        );

        /*!
        @brief Get a vector of VCD time values
        @param hash in - The hashcode for the signal to identify it.
        @returns A pointer to the vector of time values, or nullptr if hash not found
        */
        /*TimedValues * get_signal_values (
            VCDSignalHash hash
        );*/
        
        /*!
        @brief Return a pointer to the set of timestamp samples present in
               the VCD file.
        */
        //std::vector<VCDTime> get_timestamps();
        
        /*!
        @brief Get a vector of all scopes present in the file.
        */
        std::vector<VCDScope*> get_scopes();
        
        /*!
        @brief Return a flattened vector of all signals in the file.
        */
        std::vector<VCDSignal*> get_signals();

        /*!
        @brief parse the vcd file
        */
        bool parse(std::string& vcdFilePath, VCDTime start, VCDTime end);//, std::map<std::string, int>& initial_net_map);
        std::vector<std::string> SplitBySpace(std::string& str);
        //void set_name_hash_pair(std::string name, std::string hash){name_hash_pair[name] = hash;}
        void set_hash_name_pair(std::string hash, std::string name){hash_name_pair[hash] = name;}
        //std::unordered_map<std::string, std::string> get_name_hash_pair(){return name_hash_pair;}
        std::unordered_map<std::string, std::string> get_hash_name_pair(){return hash_name_pair;}
        /*bool is_input(std::string pin_name){
            std::unordered_map<std::string, std::string>::iterator pos = name_hash_pair.find(pin_name);
            return (pos != name_hash_pair.end())?true:false;
        }*/
        //std::unordered_map<std::string, TimedValues*> get_val_map(){return val_map;}

        VCDSignal* get_signal_by_hash(std::string hash){
            if (signals_map.find(hash) != signals_map.end())
            {
                return signals_map[hash];
            }
            return NULL;
        }

    protected:
        
        //! Flat vector of all signals in the file.
        std::vector<VCDSignal*> signals;
        std::unordered_map<std::string, VCDSignal*> signals_map;   //hash->
        
        //! Flat mao of all scope objects in the file, keyed by name.
        std::vector<VCDScope*>  scopes;
        std::vector<VCDScope*>  _scopes;

        //! Vector of time values present in the VCD file - sorted, asc
        //std::vector<VCDTime>    times;

        //! Map of hashes onto vectors of times and signal values.
        //std::unordered_map<VCDSignalHash, VCDSignalValues*> val_map;
        //std::unordered_map<std::string, TimedValues*> val_map;

        std::unordered_map<std::string, int> hash_size_pair;
        
        std::unordered_map<std::string, int> name_rindex_pair;
        
        std::vector<std::string> vec_values_vec;
        std::vector<unsigned int> vec_values_id_vec;
        std::vector<int> name_lindex_pair;

        unsigned long long _total_len;

};


#endif
