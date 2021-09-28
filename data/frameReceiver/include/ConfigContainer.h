#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>

using namespace boost::placeholders;
namespace pt = boost::property_tree;

class ConfigContainer
{

    typedef boost::function<void(pt::ptree&)> SetterFunc;
    typedef std::map<std::string, SetterFunc> SetterFuncMap;

    public:
        void update(pt::ptree& tree)
        {
            for (SetterFuncMap::iterator it = setter_map_.begin(); it != setter_map_.end(); ++it)
            {
                (*it).second(tree);
            }
        }

    protected:

        template<typename T>
        void bind_param(const std::string& name, T& param, const std::string& path)
        {
            setter_map_[name] = boost::bind(
                &ConfigContainer::param_set<T>, this, boost::ref(param), path, _1
            );
        }

        template<typename T>
        void param_set(T& param, std::string path, pt::ptree& tree)
        {
            replace_path_sep(path);

            boost::optional<T> val = tree.get_optional<T>(path);
            if (val)
            {
                param = *val;
            }
        }

        template<typename T>
        void bind_vector_param(const std::string name, std::vector<T>& param, const std::string& path)
        {
            setter_map_[name] = boost::bind(
                &ConfigContainer::vector_param_set<T>, this, boost::ref(param), path, _1
            );
        }

        template<typename T>
        void vector_param_set(std::vector<T>& param, std::string path, pt::ptree& tree)
        {
            replace_path_sep(path);

            boost::optional< pt::ptree& > array = tree.get_child_optional(path);
            if (array)
            {
                param.clear();
                BOOST_FOREACH(pt::ptree::value_type &v, *array) {
                    param.push_back(v.second.data());
                }
            }
        }

        void replace_path_sep(std::string& path)
        {
            std::size_t pos = path.find("/");
            while (pos != std::string::npos)
            {
                path.replace(pos, 1, ".");
                pos = path.find("/", pos + 1);
            }
        }

    private:
        SetterFuncMap setter_map_;

};