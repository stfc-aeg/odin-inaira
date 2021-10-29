/*!
 * ParamContainer.cpp - parameter container class with JSON encoding/decoding
 *
 * This class implements a simple parameter container with JSON encoding/decoding, allowing
 * applications to maintain e.g. configuration and status parameters with easy integration
 * with external client control via JSON message payloads (e.g IpcMessage).
 *
 * Created on: Oct 7, 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include <sstream>
#include "ParamContainer.h"

namespace OdinData
{

//! Encodes the parameter container to a JSON-formatted string
//!
//! This method encodes the parameter container to a JSON-formatted string. The values of
//! all bound parameters in the container are encoded into return JSON string.
//!
//! \return JSON-encoded string of all bound parameters in the container

std::string ParamContainer::encode(void)
{
    doc_.SetObject();
    encode(doc_);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<> > writer(sb);

    doc_.Accept(writer);

    std::string encoded(sb.GetString());
    return encoded;
}

//! Encodes the parameter container into an existing document
//!
//! This method encodes the paramter container into a JSON document, using the specified path
//! as a prefix for all parameter paths. The values of all bound parameters are encoded into
//! the document.
//!
//! \param doc_obj - reference to the JSON document to encode parameters into
//! \param prefix_path - string to prefix to the path of all bound parameters

void ParamContainer::encode(ParamContainer::Document& doc_obj, std::string prefix_path)
{
    // Construct the JSON pointer prefix based on the prefix path
    std::string pointer_prefix = pointer_path("");
    if (!prefix_path.empty())
    {
        pointer_prefix += prefix_path;

        // Ensure that the pointer prefix is correctly terminated with a trailing slash.
        if (*pointer_prefix.rbegin() != '/')
        {
            pointer_prefix += "/";
        }
    }

    // Iterate through all the bound paramters in the getter map, retreiving their current
    // values and setting in the JSON document.
    for (GetterFuncMap::iterator it = getter_map_.begin(); it != getter_map_.end(); ++it)
    {
        rapidjson::Value value_obj;
        (*it).second(value_obj);
        std::string path = pointer_prefix + (*it).first;
        rapidjson::Pointer(path.c_str()).Set(doc_obj, value_obj);
    }
}

//! Updates the values of parameters from the specified JSON-formatted string
//!
//! This method updates the values of the parameters from the specfied JSON-formatted string.
//! Parameters in the string that do not correspond to bound parameters in the container are
//! ignored.
//!
//! \param json - JSON-formatted string to update parameters from

void ParamContainer::update(std::string json)
{
    update(json.c_str());
}

//! Updates the values of parameters from the specified JSON-formatted character array
//!
//! This method updates the values of the parameters from the specfied JSON-formatted character.
//! array. Parameters in the array that do not correspond to bound parameters in the container are
//! ignored.
//!
//! \param json - pointer to a JSON-formatted character array to update parameters from

void ParamContainer::update(const char* json)
{
    // Parse the JSON argument into the document
    doc_.Parse(json);

    // Throw an informative exception of parse errors were encountered
    if (doc_.HasParseError())
    {
        std::stringstream ss;
        ss << "JSON parse error updating configuration from string at offset "
            << doc_.GetErrorOffset() ;
        ss << " : " << rapidjson::GetParseError_En(doc_.GetParseError());
        throw ParamContainerException(ss.str());
    }

    // Update parameters in the container from the parsed JSON document
    update(doc_);
}

//! Updates the values of the parameters from the specified JSON document
//!
//! This method updates the values of the parameters from the specfied JSON document object.
//! Parameters in the document that do not correspond to bound parameters in the container are
//! ignored.
//!
//! \param json - reference to a JSON document obhect to update paramters from

void ParamContainer::update(ParamContainer::Document& doc_obj)
{
    // Iterate through all bound parameters in the setter function map
    for (SetterFuncMap::iterator it = setter_map_.begin(); it != setter_map_.end(); ++it)
    {
        // If the path of the bound parameter is found in the JSON document, call the setter
        // function to update the value of the parameter
        if (rapidjson::Value* value_ptr = rapidjson::Pointer(
            pointer_path((*it).first).c_str()).Get(doc_obj)
        )
        {
            (*it).second(*value_ptr);
        }
    }
}

// Explicit specialisations of the set_value and get_value methods, mapping native attribute types
// to the appropriate RapidJSON storage type.

template<> int ParamContainer::set_value(rapidjson::Value& value_obj) const
{
    return value_obj.GetInt();
}

template<> void ParamContainer::get_value(int& param, rapidjson::Value& value_obj) const
{
    value_obj.SetInt(param);
}

template<> unsigned int ParamContainer::set_value(rapidjson::Value& value_obj) const
{
    return value_obj.GetUint();
}

template<> void ParamContainer::get_value(unsigned int& param, rapidjson::Value& value_obj) const
{
    value_obj.SetUint(param);
}

#ifdef __APPLE__
// The MacOS clang compiler seems to base uint64 on a different base type than gcc on Linux, causing
// linker errors with templated specialisations of get_value and set_value for unsigned long.
template<> unsigned long ParamContainer::set_value(rapidjson::Value& value_obj) const
{
  return value_obj.GetUint64();
}

template<> void ParamContainer::get_value(unsigned long& param, rapidjson::Value& value_obj) const
{
    value_obj.SetUint64(param);
}
#endif

template<> int64_t ParamContainer::set_value(rapidjson::Value& value_obj) const
{
  return value_obj.GetInt64();
}

template<> void ParamContainer::get_value(int64_t& param, rapidjson::Value& value_obj) const
{
    value_obj.SetInt64(param);
}

template<> uint64_t ParamContainer::set_value(rapidjson::Value& value_obj) const
{
  return value_obj.GetUint64();
}

template<> void ParamContainer::get_value(uint64_t& param, rapidjson::Value& value_obj) const
{
    value_obj.SetUint64(param);
}

template<> double ParamContainer::set_value(rapidjson::Value& value_obj) const
{
  return value_obj.GetDouble();
}

template<> void ParamContainer::get_value(double& param, rapidjson::Value& value_obj) const
{
    value_obj.SetDouble(param);
}

template<> std::string ParamContainer::set_value(rapidjson::Value& value_obj) const
{
  return value_obj.GetString();
}

template<> void ParamContainer::get_value(std::string& param, rapidjson::Value& value_obj) const
{
    value_obj.SetString(rapidjson::StringRef(param.c_str()));
}

template<> bool ParamContainer::set_value(rapidjson::Value& value_obj) const
{
  return value_obj.GetBool();
}

template<> void ParamContainer::get_value(bool& param, rapidjson::Value& value_obj) const
{
    value_obj.SetBool(param);
}

} // namespace OdinData