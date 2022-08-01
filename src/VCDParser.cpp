
#include <iostream>
#include <algorithm>
#include <numeric>
#include <thread>

#include "VCDParser.h"
        
using namespace std;      
static unordered_map<char, short> lut = {{'0', 0}, {'1', 3}, {'x', 1}, {'z', 2}, {'X', 1}, {'Z', 2}};
//! Instance a new VCD file container.
VCDParser::VCDParser(){

}
        
//! Destructor
VCDParser::~VCDParser(){

    // Delete signals and scopes.
    delete root_scope;

    

    for (std::vector<VCDScope*>::iterator j = scopes.begin(); j != scopes.end(); ++j) {
    
        for (std::vector<VCDSignal*>::iterator i = ((*j)->signals).begin(); i != ((*j)->signals).end(); ++i)
        {
            delete *i;
        }
        
        delete *j;
    }
    for (std::vector<VCDScope*>::iterator j = _scopes.begin(); j != _scopes.end(); ++j) {
    
        for (std::vector<VCDSignal*>::iterator i = ((*j)->signals).begin(); i != ((*j)->signals).end(); ++i)
        {
            delete *i;
        }
        
        delete *j;
    }
    for(auto it:signals_map){
        delete it.second;
    }
    for (std::vector<VCDSignal*>::iterator i = signals.begin(); i != signals.end(); ++i){
        delete *i;
    }

    

}


/*!
@brief Add a new scope object to the VCD file
*/
void VCDParser::add_scope(
    VCDScope * s
){
    scopes.push_back(s);
}


/*!
@brief Add a new signal object to the VCD file
*/
void VCDParser::add_signal(
    VCDSignal * s
){
    signals.push_back(s);
    if (signals_map.find(s->hash) == signals_map.end())
    {
        signals_map[s->hash] = s;
    }
    // Add a timestream entry
    /*if(val_map.find(s -> reference) == val_map.end()) {
        // Values will be populated later.
        val_map[s -> hash] = new VCDSignalValues();
    }*/
}


/*!
 */
VCDScope *VCDParser::get_scope(VCDScopeName name) {
  for (std::vector<VCDScope*>::iterator i = scopes.begin(); i != scopes.end(); ++i)
    {
    if ((*i)->name == name) {
      return *i;
    }
  }
  return NULL;
}


/*!
@brief Add a new signal value to the VCD file, tagged by time.
*/
void VCDParser::add_signal_value(
    unsigned int time,
    VCDBit bit,
    std::string   name
){
    times[name].push_back(time);
    values[name].push_back(bit);
}


/*!
*/
/*std::vector<VCDTime> VCDParser::get_timestamps(){
    return times;
}*/


/*!
*/
std::vector<VCDScope*> VCDParser::get_scopes(){
    return _scopes;
}


/*!
*/
std::vector<VCDSignal*> VCDParser::get_signals(){
    return signals;
}


/*!
*/
/*void VCDParser::add_timestamp(
    VCDTime time
){
    times.push_back(time);
}*/

/*!
*/
VCDValue * VCDParser::get_signal_value_at (
    const VCDSignalHash& hash,
    VCDTime       time,
    bool erase_prior
){/*
    auto find = val_map.find(hash); 
    if(find == val_map.end()) {
        return NULL;
    }
    
    VCDSignalValues * vals = find->second;

    if(vals -> size() == 0) {
        return NULL;
    }

    VCDSignalValues::iterator erase_until = vals->begin();

    VCDValue * tr = NULL;

    for(VCDSignalValues::iterator it = vals -> begin();
             it != vals -> end();
             ++ it) {

        if((*it) -> time <= time) {
            erase_until = it;
            tr = (*it) -> value;
        } else {
            break;
        }
    }

    if (erase_prior) {
        // avoid O(n^2) performance for large sequential scans
        for (VCDSignalValues::iterator i = vals->begin() ; i != erase_until; i++) {
            delete (*i) -> value;
        }
        vals->erase(vals->begin(), erase_until);
    }

    return tr;*/
}

/*TimedValues * VCDParser::get_signal_values (
    VCDSignalHash hash
){
    if(val_map.find(hash) == val_map.end()) {
        return NULL;
    }

    return val_map[hash];
}*/

//multi thread to process vec_values
void core(VCDParser &p ,unsigned left, unsigned right, unsigned vec_values_size){
    //unsigned _size = p.vec_values_vec.size();
    for(unsigned i = left; i < right && i < vec_values_size; ++i){
        string real_name = (p.vec_values_vec)[i];   //real name , e.g. bus_net[2]
        //unsigned int *_time = &(*((p.times)[real_name].begin()));
        //short *_value = &(*((p.values)[real_name].begin()));
        size_t pos = real_name.find("[");
        string reference = real_name.substr(0, pos);  //bus name, e.g. bus_net
        //vector<string> tmp = (p.vec_values)[reference];
        unsigned _idx = (p.vec_values_id_vec)[i];
        //vector<string> tmp = (p.vec_vec_values)[_idx];
        vector<char*> tmp = (p.vec_vec_values)[_idx];
        unsigned vec_size = tmp.size();//(p.vec_sizes)[_idx];
        /*if (reference == "cbus_ICCADs_out_xbar_ICCADs__T_485")
                    {
                        cout << (p.name_lindex_pair)[reference] << endl;
                        cout << vec_size << endl;
                    }*/
        int lindex = (p.name_lindex_pair)[i];
        //int rindex = (p.name_rindex_pair)[reference];
        int idx = stoi(real_name.substr(pos + 1, real_name.find("]") - pos - 1));
        int j = abs(lindex - idx);   // pos at string 
        short last = 2;
        //vector<unsigned> last_idx(size, 0);
        unsigned last_idx = 0;
        //unsigned int *_times = &(*((p.times)[reference].begin()));
        unsigned int *_times = &(*((p.vec_times)[_idx].begin()));
        for(unsigned k = 0; k < vec_size; ++k){
            //string str = tmp[k];
            char *str = tmp[k];
            unsigned int cur_time = _times[k];
            if(lut.find(str[j]) == lut.end()){
                cout << "error" << endl;
            }
            short cur = lut[str[j]];
            //cout << j << "," << endl;
            //cout << ":" << str[j] << endl;
            if(cur != last){
                //_time[last_idx] = cur_time;
                //_value[last_idx] = cur;
                (p.times)[real_name].push_back(cur_time);
                (p.values)[real_name].push_back(cur);
                ++last_idx;
                last = cur;
            }
        }
        (p.sizes)[real_name] = last_idx;
    }
}

void VCDParser::process(){
    //sort(vec_values_vec.begin(), vec_values_vec.end());
    //random_shuffle(vec_values_vec.begin(), vec_values_vec.end());
    unsigned long long total_len = _total_len;//4753040 * 16;//0;
    unsigned vec_values_size = vec_values_vec.size();
    unsigned max_cores = min(vec_values_size, thread::hardware_concurrency()*2);// thread::hardware_concurrency() * 2;
    vector<thread> threads(max_cores);
    cout << vec_values_size << endl;
    unsigned long long num_every_thread = total_len / max_cores;
    cout << num_every_thread << endl;
    unsigned left = 0;
    vector<unsigned> right(max_cores);
    for(unsigned t = 0; t < max_cores; ++t){
        unsigned long long size_ = 0;
        unsigned i;
        for(i = left; i < vec_values_size && size_ < num_every_thread; ++i){
            //string real_name = (vec_values_vec)[i];   //real name , e.g. bus_net[2]
            //size_t pos = real_name.find("[");
            //string reference = real_name.substr(0, pos);
            //unsigned long long _size = (times)[reference].size();
            unsigned _idx = vec_values_id_vec[i];
            unsigned long long _size = vec_times[_idx].size();
            size_ += _size;
        }
        //cout << size_ << endl;
        right[t] = i;
        left = i;
    }
    left = 0;
    for(unsigned t = 0; t < max_cores; ++t){
        //cout << "\t" << size_ << "," << right << endl;
        threads[t] = thread(core, std::ref(*this), left, right[t], vec_values_size);
        left = right[t];
    }
    for(auto &t:threads){
        t.join();
    }
}

void VCDParser::init(std::string& vcdFilePath, VCDTime end){
    string str;
    unsigned _idx = 0;
    VCDTime _end;
    if(end > 1000000000 && end < 2900000000)
        end = 53994;
    else if(end > 2900000000)
        end = 743009;
    else if(end > 100000000)
        end = 10000;
    else
        end = 5000;
    if(end == 743009)
    {
        _end = end / 5.5;
    }
    else
    {
        _end = end;
    }
    ifstream inf1(vcdFilePath);
    if (!inf1)
    {
        cout << "File " << vcdFilePath << " Open Error!" << endl;
        cout << "Exit!" << endl;
        return;
    }
    std::stringstream buffer; 
    buffer << inf1.rdbuf(); 
    inf1.close();
    
    //vec_values_vec.reserve();
    while(buffer >> str){
        if(str == "$var"){
            vector<string> line_vec(5);
            int i = 0;
            while(buffer >> str && str != "$end"){
                line_vec[i++] = str;
            }            
            ++_idx;
        }
    }
    buffer.clear();
    buffer.str("");
    unsigned max_cores = thread::hardware_concurrency() * 2;
    _vec_times.resize(_idx, vector<vector<unsigned>>(max_cores));
    _vec_values.resize(_idx, vector<vector<short>>(max_cores));
    _vec_vec_values.resize(_idx, vector<vector<char*>>(max_cores));
    vec_times.resize(_idx);
    vec_values.resize(_idx);
    vec_vec_values.resize(_idx);
    vec_sizes.resize(_idx, 0);
    num = _idx;

    ifstream inf(vcdFilePath);
    if (!inf)
    {
        cout << "File " << vcdFilePath << " Open Error!" << endl;
        cout << "Exit!" << endl;
        return;
    }
    unsigned int current_time = 0;
    unsigned int last_time = 0;
    std::stringstream buffer1; 
    buffer1 << inf.rdbuf(); 
    inf.close();
    _idx = 0;
    while(buffer1 >> str){
        if(str == "$var"){
            vector<string> line_vec(5);
            int i = 0;
            while(buffer1 >> str && str != "$end"){
                line_vec[i++] = str;
            }
            string hash = line_vec[2];
            int size = stoi(line_vec[1]);
            string reference = line_vec[3];
            
            hash_size_pair[hash] = size;
            hash_index_pair[hash] = _idx;
            if(i == 5){
                string tmp = line_vec[4].substr(1, line_vec[4].size() - 2);
                size_t pos = tmp.find(":");
                if (pos != std::string::npos)
                {
                    int lindex = stoi(tmp.substr(0,pos));
                    int rindex = stoi(tmp.substr(pos+1));
                    //name_lindex_pair[reference] = lindex;
                    name_rindex_pair[reference] = rindex;
                    hash_name_pair[reference] = hash;
                    int start = lindex;
                    for (unsigned i = 0; i < size; ++i)
                    {
                        string name = reference + "[" + to_string(start) + "]";
                        vec_values_vec.push_back(name);
                        InMaps_bus.insert(name);
                        vec_values_id_vec.push_back(_idx);
                        name_lindex_pair.push_back(lindex);
                        times[name].reserve(_end);
                        values[name].reserve(_end);
                        sizes.insert(make_pair(name, 0));//[name] = 0;
                        
                        if(lindex < rindex)
                            start++;
                        else
                            start--;
                    }
                    /*for(int t = 0; t < max_cores; ++t){
                        _vec_times[_idx][t].resize((end + 2) / max_cores + 1);
                        _vec_vec_values[_idx][t].resize((end + 2) / max_cores + 1);
                    }*/
                    //vec_times[_idx].resize(end + 2);
                    //vec_vec_values[_idx].resize(end + 2);
                }
                else
                {
                    int lindex = stoi(tmp);
                    string name = reference + "[" + to_string(lindex) + "]";
                    hash_name_pair[name] = hash;
                    InMaps_bit.insert(name);
                    /*for(int t = 0; t < max_cores; ++t){
                        _vec_times[_idx][t].resize((end * 2 + 2) / max_cores + 1);
                        _vec_values[_idx][t].resize((end * 2 + 2) / max_cores + 1);
                    }*/
                    //vec_times[_idx].resize(end * 2 + 2);
                    //vec_values[_idx].resize(end * 2 + 2);
                }
            }
            else{
                hash_name_pair[reference] = hash;
                if(size == 1)
                {
                    /*for(int t = 0; t < max_cores; ++t){
                        _vec_times[_idx][t].resize((end * 2 + 2) / max_cores + 1);
                        _vec_values[_idx][t].resize((end * 2 + 2) / max_cores + 1);
                    }*/
                    //vec_times[_idx].resize(end * 2 + 2);
                    //vec_values[_idx].resize(end * 2 + 2);
                    InMaps_bit.insert(reference);
                }
                else
                {
                    int lindex = size - 1;
                    //name_lindex_pair[reference] = lindex;
                    name_rindex_pair[reference] = 0;
                    int start = lindex;
                    for (unsigned i = 0; i < size; ++i)
                    {
                        string name = reference + "[" + to_string(start) + "]";
                        vec_values_vec.push_back(name);
                        InMaps_bus.insert(name);
                        vec_values_id_vec.push_back(_idx);
                        name_lindex_pair.push_back(lindex);
                        times[name].reserve(_end);
                        values[name].reserve(_end);
                        sizes.insert(make_pair(name, 0));
                        start--;
                    }
                    /*for(int t = 0; t < max_cores; ++t){
                        _vec_times[_idx][t].resize((end + 2) / max_cores + 1);
                        _vec_vec_values[_idx][t].resize((end + 2) / max_cores + 1);
                    }*/
                    //vec_times[_idx].resize(end + 2);
                    //vec_vec_values[_idx].resize(end + 2);
                }
            }
            ++_idx;
        }
    }
    //vec_times.resize(_idx);
    //vec_values.resize(_idx);
    //vec_vec_values.resize(_idx);
    buffer1.clear();
    buffer1.str("");
}
void combine_core(VCDParser &p ,unsigned left, unsigned right, unsigned max_cores){
    for(unsigned i = left; i < right && i < (p.num); ++i)
    {
        //unsigned int *times = &(*((p.vec_times)[i].begin()));
        //unsigned length = 0;
        if((p._vec_vec_values)[i][0].size())
        {
            //char **values = &(*((p.vec_vec_values)[i].begin()));
            for(unsigned t = 0; t < max_cores; ++t)
            {
                unsigned int *_times = &(*((p._vec_times)[i][t].begin()));
                char **_values = &(*((p._vec_vec_values)[i][t].begin()));
                unsigned int _size = (p._vec_times)[i][t].size();
                for(unsigned j = 0; j < _size; ++j)
                {
                    if(_times[j] == 0 && !(t == 0 && j == 0))
                    {
                        break;
                    }
                    //times[length] = _times[j];
                    //values[length++] = _values[j];
                    (p.vec_times)[i].push_back(_times[j]);
                    (p.vec_vec_values)[i].push_back(_values[j]);
                }
            }
        }
        else
        {
            //short *values = &(*((p.vec_values)[i].begin()));
            for(unsigned t = 0; t < max_cores; ++t)
            {
                unsigned int *_times = &(*((p._vec_times)[i][t].begin()));
                short *_values = &(*((p._vec_values)[i][t].begin()));
                unsigned int _size = (p._vec_times)[i][t].size();
                for(unsigned j = 0; j < _size; ++j)
                {
                    if(_times[j] == 0 && !(t == 0 && j == 0))
                    {
                        break;
                    }
                    //times[length] = _times[j];
                    //values[length++] = _values[j];
                    (p.vec_times)[i].push_back(_times[j]);
                    (p.vec_values)[i].push_back(_values[j]);
                }
            }
        }
        //(p.vec_sizes)[i] = length;
    }
}
void parse_core(VCDParser &p ,std::string& file_str, unsigned t, vector<unsigned long long>& total_len){
    stringstream buffer;
    stringstream num_str;
    buffer << file_str;
    unsigned long long current_time = 0;
    unsigned long long last_time = 0;
    std::vector<unsigned> size_(p.num, 0);
    string str;
    unsigned total_ = 0;
    while(buffer >> str){
        if(str == "$timescale"){
            while(buffer >> str && str != "$end"){
                //unit process
            }
        }
        else if(str == "$dumpvars"){
            while(getline(buffer, str, '\n')){
                str.erase(0, str.find_first_not_of(" "));
                str.erase(str.find_last_not_of(" ") + 1);
                if(str == "$end"){
                    break;
                }
                if(str[0] == 'b'){
                    size_t pos1 = str.find_first_of(" ");
                    size_t pos2 = str.find_last_of(" ");
                    string val = str.substr(1, pos1 - 1);
                    int val_size = val.size();
                    char *val_ch = new char[val_size + 1];
                    strcpy(val_ch, val.c_str());
                    string hash = str.substr(pos2 + 1);
                    //string name = hash_name_pair[hash];
                    //int real_size = hash_size_pair[hash];
                    //int val_size = val.size();
                    //if (real_size > val_size)
                    //{
                    //    val = (val[0] == 'x'?std::string(real_size - val_size, 'x'):std::string(real_size - val_size, '0')) + val;
                    //}
                    unsigned idx = (p.hash_index_pair)[hash];
                    /*unsigned pos = size_[idx];
                    (p._vec_times)[idx][t][pos] = current_time;
                    (p._vec_vec_values)[idx][t][pos] = val_ch;
                    ++pos;
                    size_[idx] = pos;*/
                    (p._vec_times)[idx][t].push_back(current_time);
                    (p._vec_vec_values)[idx][t].push_back(val_ch);
                    total_ += val_size;
                }
                else if(lut.find(str[0]) != lut.end()){
                    short val = lut[str[0]];
                    //cout << str << endl;
                    string hash = str.substr(1);
                    //string name = hash_name_pair[hash];
                    unsigned idx = (p.hash_index_pair)[hash];
                    /*unsigned pos = size_[idx];
                    (p._vec_times)[idx][t][pos] = current_time;
                    (p._vec_values)[idx][t][pos] = val;
                    ++pos;
                    size_[idx] = pos;*/
                    (p._vec_times)[idx][t].push_back(current_time);
                    (p._vec_values)[idx][t].push_back(val);
                }
            }
        }
        else if(str[0] == '#'){
            string tmp = str.substr(1);
            num_str << tmp;
            num_str >> current_time;
            if (current_time > last_time)
            {
                //cout << "\t" << current_time << endl;
                last_time = current_time;
                //end_time = current_time;
            }
            while(getline(buffer, str, '\n')){
                str.erase(0, str.find_first_not_of(" "));
                str.erase(str.find_last_not_of(" ") + 1);
                if(str[0] == '#'){
                    string tmp = str.substr(1);
                    num_str.clear();
                    num_str.str("");
                    num_str << tmp;
                    num_str >> current_time;
                    //cout << "\t" << current_time << endl;
                    if (current_time > last_time)
                    {
                        last_time = current_time;
                        //end_time = current_time;
                    }
                }
                else if(str[0] == 'b'){
                    size_t pos1 = str.find_first_of(" ");
                    size_t pos2 = str.find_last_of(" ");
                    string val = str.substr(1, pos1 - 1);
                    int val_size = val.size();
                    char *val_ch = new char[val_size + 1];
                    strcpy(val_ch, val.c_str());
                    string hash = str.substr(pos2 + 1);
                    //string name = hash_name_pair[hash];
                    //int real_size = hash_size_pair[hash];
                    //int val_size = val.size();
                    //if (real_size > val_size)
                    //{
                    //    val = (val[0] == 'x'?std::string(real_size - val_size, 'x'):std::string(real_size - val_size, '0')) + val;
                    //}
                    unsigned idx = (p.hash_index_pair)[hash];
                    /*unsigned pos = size_[idx];
                    (p._vec_times)[idx][t][pos] = current_time;
                    (p._vec_vec_values)[idx][t][pos] = val_ch;
                    ++pos;
                    size_[idx] = pos;*/
                    (p._vec_times)[idx][t].push_back(current_time);
                    (p._vec_vec_values)[idx][t].push_back(val_ch);
                    total_ += val_size;
                }
                else if(lut.find(str[0]) != lut.end()){
                    short val = lut[str[0]];
                    string hash = str.substr(1);
                    //string name = hash_name_pair[hash];
                    unsigned idx = (p.hash_index_pair)[hash];
                    /*unsigned pos = size_[idx];
                    (p._vec_times)[idx][t][pos] = current_time;
                    (p._vec_values)[idx][t][pos] = val;
                    ++pos;
                    size_[idx] = pos;*/
                    (p._vec_times)[idx][t].push_back(current_time);
                    (p._vec_values)[idx][t].push_back(val);
                }
            }
            break;
        }
    }
    total_len[t] = total_;
    num_str.clear();
    num_str.str("");
    buffer.clear();
    buffer.str("");
}
bool VCDParser::parse(std::string& vcdFilePath, VCDTime start, VCDTime end){//, std::map<std::string, int>& initial_net_map){
    //unsigned size = initial_net_map.size();
    //times.resize(size);
    //values.resize(size);
    _total_len = 0;
    ifstream inf(vcdFilePath);
    if (!inf)
    {
        cout << "File " << vcdFilePath << " Open Error!" << endl;
        cout << "Exit!" << endl;
        return false;
    }
    unsigned int current_time = 0;
    unsigned int last_time = 0;
    std::stringstream buffer; 
    stringstream num_str;
    buffer << inf.rdbuf(); 
    string file_str = buffer.str();
    unsigned cnt = 0;
    unsigned long long position = 0;
    unsigned max_cores = thread::hardware_concurrency() * 2;
    vector<unsigned long long> file_str_pos;
    vector<string> file_str_vec(max_cores);
    while((position=file_str.find("\n#", position)) != std::string::npos){
        ++cnt;
        file_str_pos.push_back(position);
        ++position;
    }
    cout << cnt << endl;
    unsigned long long num_of_flag_every_thread = (cnt + max_cores - 1) / max_cores;
    unsigned long long left = 0;
    for(unsigned i = 0; i < max_cores - 1; ++i)
    {
        unsigned long long right = file_str_pos[num_of_flag_every_thread * (i + 1)];
        file_str_vec[i] = file_str.substr(left, right + 1 - left);
        left = right + 1;
    }
    file_str_vec[max_cores - 1] = file_str.substr(left);
    cout << left << endl;
    inf.close();

    vector<thread> threads(max_cores);
    vector<unsigned long long> total_len(max_cores);
    for(unsigned t = 0; t < max_cores; ++t)
    {
        threads[t] = thread(parse_core, std::ref(*this), std::ref(file_str_vec[t]), t, std::ref(total_len));
    }
    for(auto &th:threads){
        th.join();
    }
    cout << "parse_core done" << endl;
    _total_len = accumulate(total_len.begin(), total_len.end(), 0);
    unsigned _left = 0;
    unsigned num_every_thread = (num + max_cores - 1) / max_cores;
    vector<thread> _threads(max_cores);
    for(unsigned t = 0; t < max_cores; ++t)
    {
        unsigned right = _left + num_every_thread;
        _threads[t] = thread(combine_core, std::ref(*this), _left, right, max_cores);
        _left = right;
    }
    for(auto &th:_threads){
        th.join();
    }
    cout << "combine_core done" << endl;
    
    cout << "VCD parse done!" << endl;
    num_str.clear();
    num_str.str("");
    buffer.clear();
    buffer.str("");
    return true;
}

std::vector<std::string> VCDParser::SplitBySpace(std::string& str)
{
    std::string buf;
    std::stringstream ss(str);

    std::vector<std::string> tokens;

    while (ss >> buf)
        tokens.push_back(buf);

    return tokens;
}

