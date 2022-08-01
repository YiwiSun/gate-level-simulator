
#include <map>
#include <unordered_map>
#include <utility>
#include <string>
#include <cstring>
#include <vector>
#include <deque>
#include <assert.h>
#include <cstdlib>
#include <stdlib.h>
#include <cstddef>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
//#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/access.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

/*!
@file VCDTypes.hpp
@brief A file for common types and data structures used by the VCD parser.
*/

#ifndef VCDTypes_H
#define VCDTypes_H

//! Friendly name for a signal
typedef std::string VCDSignalReference;

//! Friendly name for a scope
typedef std::string VCDScopeName;

//! Compressed hash representation of a signal.
typedef std::string VCDSignalHash;

//! Represents a single instant in time in a trace
typedef unsigned long long VCDTime;

//! Specifies the timing resoloution along with VCDTimeUnit
typedef unsigned VCDTimeRes;

//! Width in bits of a signal.
typedef unsigned VCDSignalSize;

//! Represents the four-state signal values of a VCD file.
typedef enum {
    VCD_0 = 0,  //!< Logic zero
    VCD_X = 1,  //!< Unknown / Undefined
    VCD_Z = 2,  //!< High Impedence
    VCD_1 = 3   //!< Logic one.
} VCDBit;


//! A vector of VCDBit values.
typedef std::vector<VCDBit> VCDBitVector;

//! Typedef to identify a real number as stored in a VCD.
typedef double VCDReal;


//! Describes how a signal value is represented in the VCD trace.
typedef enum {
    VCD_SCALAR, //!< Single VCDBit
    VCD_VECTOR, //!< Vector of VCDBit
    VCD_REAL    //!< IEEE Floating point (64bit).
} VCDValueType;


// Forward declaration of class.
class VCDValue;


//! A signal value tagged with times.
typedef struct {
    VCDTime     time;
    VCDValue  * value;
} VCDTimedValue;


//! A vector of tagged time/value pairs, sorted by time values.
typedef std::deque<VCDTimedValue*> VCDSignalValues;


//! Variable types of a signal in a VCD file.
typedef std::string VCDVarType;


//! Represents the possible time units a VCD file is specified in.
typedef enum {
    TIME_S,     //!< Seconds
    TIME_MS,    //!< Milliseconds
    TIME_US,    //!< Microseconds
    TIME_NS,    //!< Nanoseconds
    TIME_PS,    //!< Picoseconds
} VCDTimeUnit;


//! Represents the type of SV construct who's scope we are in.
typedef std::string VCDScopeType;


// Typedef over vcdscope to make it available to VCDSignal struct.
typedef struct vcdscope VCDScope;

//! Represents a single signal reference within a VCD file
typedef struct {
    VCDSignalHash       hash;
    VCDSignalReference  reference;   //port name
    VCDScope          * scope;
    VCDSignalSize       size;
    VCDVarType          type;
    int                 lindex; // -1 if no brackets, otherwise [lindex] or [lindex:rindex]
    int                 rindex; // -1 if not [lindex:rindex]
} VCDSignal;
/** Module Instance SDF **/
//Pin : ModuleType's port/wire OR instance's port/wire declare
typedef struct {
    std::string         name;
    std::string         type;   //input/output/wire/reg/inout
    unsigned            size;
    int                 lindex; // -1 if no brackets, otherwise [lindex] or [lindex:rindex]
    int                 rindex; // -1 if not [lindex:rindex]
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) //
    {
        //ar.register_type(static_cast<ModuleType *>(NULL));
        ar & type;
        ar & name;
        ar & size;
        ar & lindex;
        ar & rindex;
            
    }
} Pin;



typedef struct 
{
    VCDTime t;
    VCDBit  value;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) //
    {
        //ar.register_type(static_cast<ModuleType *>(NULL));
        ar & t;
        ar & value;
            
    }
} TimedValue;

typedef std::deque<TimedValue> TimedValues;

//For Simulate
typedef struct 
{
    VCDTime time;
    std::map<std::string, VCDBit> values;
} TimedValueSim;

typedef struct 
{
    int edge;   // 0:pos;1:neg;2:both
    std::string in_bit;
    std::string out_bit;
    // delay value --- in theory, there should be 12 kinds of value, but generally we store 2 : rise and fall
    VCDReal rise_val;  // default:1ps , following is the same
    VCDReal fall_val;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) //
    {
        //ar.register_type(static_cast<ModuleType *>(NULL));
        ar & edge;
        ar & in_bit;
        ar & out_bit;
        ar & rise_val;
        ar & fall_val;
            
    }
} Delay;


class Function;
typedef struct 
{
    std::string name;
    std::map<std::string, Pin> ports;
    std::vector<std::string> in_out_ports;
    std::vector<std::string> supply1_vec;
    std::vector<std::string> supply0_vec;
    std::vector<Function> function;           //include UDP
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) //
    {
        //ar.register_type(static_cast<ModuleType *>(NULL));
        ar & name;
        ar & ports;
        ar & in_out_ports;
        ar & supply1_vec;
        ar & supply0_vec;
        ar & function;
            
    }
} ModuleType;

//! Represents a scope type, scope name pair and all of it's child signals.
struct vcdscope {
    VCDScopeName              name;     //!< The short name of the scope
    VCDScopeType              type;     //!< Construct type
    VCDScope                * parent;   //!< Parent scope object
    std::vector<VCDScope*>    children; //!< Child scope objects.
    std::vector<VCDSignal*>   signals;  //!< Signals in this scope.
};

#endif
