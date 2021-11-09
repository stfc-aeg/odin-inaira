#ifndef PARAMCONTAINER_H_
#define PARAMCONTAINER_H_

#include <map>
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>

namespace OdinData
{

class ParamContainerException : public std::exception
{
public:

  //! Create ParamContainerException with no message
  ParamContainerException(void) throw() :
      what_("")
  { };

  //! Creates ParamContainerException with informational message
  ParamContainerException(const std::string what) throw() :
      what_(what)
  {};

  //! Returns the content of the informational message
  virtual const char* what(void) const throw()
  {
    return what_.c_str();
  };

  //! Destructor
  ~ParamContainerException(void) throw() {};

private:

  // Member variables
  const std::string what_;  //!< Informational message about the exception

};

} // namespace OdinData

// Override rapidsjon assertion mechanism before including appropriate headers
#ifdef RAPIDJSON_ASSERT
#undef RAPIDJSON_ASSERT
#endif
#define RAPIDJSON_ASSERT(x) if (!(x)) throw OdinData::ParamContainerException("rapidjson assertion thrown");
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

namespace OdinData
{

class ParamContainer
{

    typedef boost::function<void(rapidjson::Value&)> SetterFunc;
    typedef std::map<std::string, SetterFunc> SetterFuncMap;
    typedef boost::function<void(rapidjson::Value&)> GetterFunc;
    typedef std::map<std::string, GetterFunc> GetterFuncMap;

    public:

        typedef rapidjson::Document Document;

        std::string encode(void);
        void encode(ParamContainer::Document& doc_obj, std::string prefix_path = std::string());
        void update(std::string json);
        void update(const char* json);
        void update(ParamContainer::Document& doc_obj);

    protected:

        template<typename T>
        void bind_param(T& param, const std::string& path)
        {
            setter_map_[path] = boost::bind(
                &ParamContainer::param_set<T>, this, boost::ref(param), path,
                boost::placeholders::_1
            );

            getter_map_[path] = boost::bind(
                &ParamContainer::param_get<T>, this, boost::ref(param), path,
                boost::placeholders::_1
            );

        }

        template<typename T>
        void bind_vector_param(std::vector<T>& param, const std::string& path)
        {
            setter_map_[path] = boost::bind(
                &ParamContainer::vector_param_set<T>, this, boost::ref(param), path,
                boost::placeholders::_1
            );

            getter_map_[path] = boost::bind(
                &ParamContainer::vector_param_get<T>, this, boost::ref(param), path,
                boost::placeholders::_1
            );
        }

        template<typename T>
        void param_set(T& param, std::string path, rapidjson::Value& value_obj)
        {
            param = set_value<T>(value_obj);
        }

        template<typename T> const
        void param_get(T& param, std::string path, rapidjson::Value& value_obj)
        {
            get_value<T>(param, value_obj);
        }

        template<typename T>
        void vector_param_set(std::vector<T>& param, std::string& path, rapidjson::Value& value_obj)
        {
            param.clear();

            for (rapidjson::Value::ValueIterator itr = value_obj.Begin();
                itr != value_obj.End(); ++itr)
            {
                param.push_back(set_value<T>(*itr));
            }
        }

        template<typename T>
        void vector_param_get(std::vector<T>& param, std::string& path, rapidjson::Value& value_obj)
        {
            rapidjson::Document::AllocatorType& allocator = doc_.GetAllocator();
            rapidjson::Value vec_obj(rapidjson::kArrayType);
            value_obj.Swap(vec_obj);

            for (typename std::vector<T>::iterator itr = param.begin(); itr != param.end(); ++itr)
            {
                rapidjson::Value val;
                get_value<T>(*itr, val);
                value_obj.PushBack(val, allocator);
            }
        }

    private:

        template<typename T> T set_value(rapidjson::Value& value_obj) const;
        template<typename T> void get_value(T& param, rapidjson::Value& value_obj) const;

        inline const std::string pointer_path(const std::string& path)
        {
            std::string pointer_path = "/" + path;
            return pointer_path;
        }

        SetterFuncMap setter_map_;
        GetterFuncMap getter_map_;
        ParamContainer::Document doc_;
};

} // namespace OdinData

#endif // PARAMCONTAINER_H_