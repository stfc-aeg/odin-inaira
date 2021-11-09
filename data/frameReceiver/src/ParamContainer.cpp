#include <sstream>
#include "ParamContainer.h"

namespace OdinData
{

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

void ParamContainer::encode(ParamContainer::Document& doc_obj, std::string prefix_path)
{
    std::string pointer_prefix = pointer_path("");
    if (!prefix_path.empty())
    {
        pointer_prefix += prefix_path; // + "/";
        if (*pointer_prefix.rbegin() != '/')
        {
            pointer_prefix += "/";
        }
    }

    for (GetterFuncMap::iterator it = getter_map_.begin(); it != getter_map_.end(); ++it)
    {
        rapidjson::Value value_obj;
        (*it).second(value_obj);
        std::string path = pointer_prefix + (*it).first;
        rapidjson::Pointer(path.c_str()).Set(doc_obj, value_obj);
    }
}


void ParamContainer::update(std::string json)
{
    update(json.c_str());
}

void ParamContainer::update(const char* json)
{
    doc_.Parse(json);
    if (doc_.HasParseError())
    {
        std::stringstream ss;
        ss << "JSON parse error updating configuration from string at offset " 
            << doc_.GetErrorOffset() ;
        ss << " : " << rapidjson::GetParseError_En(doc_.GetParseError());
        throw ParamContainerException(ss.str());
    }

    update(doc_);
}

void ParamContainer::update(ParamContainer::Document& doc_obj)
{
    for (SetterFuncMap::iterator it = setter_map_.begin(); it != setter_map_.end(); ++it)
    {
        if (rapidjson::Value* value_ptr = rapidjson::Pointer(
            pointer_path((*it).first).c_str()).Get(doc_obj)
        )
        {
            (*it).second(*value_ptr);
        }
    }
}

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