#include "config.h"
#include <iostream>

namespace sylar{
// Config::ConfigVarMap Config::s_datas;

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string& prefix, 
                        const YAML::Node& node, 
                        std::list<std::pair<std::string, const YAML::Node>>& output) {
    // find_first_not_of()是找到第一个不在 "abcdefghijklmnopqrstuvwxyz._012345678" 中出现的字符
    // 判断prefix每个字符是否都合法
    //std::cout << prefix << std::endl;
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")
        != std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << ":" << node;
        return ;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        // 迭代器似乎存着这样的一种值(如果是map的话): <"log", node>
        for (auto it = node.begin(); 
            it != node.end(); it++) {
            
            ListAllMember(prefix.empty() ? it->first.Scalar()
                : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

// 将配置文件中的数据载入到config中的MAP当中
// 将字符串转化为具体数据
void Config::LoadFromYaml(const YAML::Node& root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);
    // 取pair
    for (auto& i : all_nodes) {
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }
        // 将key转化为小写
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);   // 去MAP中查找该key
        if (var) {
            // 节点是否为标量
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for (auto it = m.begin();
            it != m.end(); it++) {
        cb(it->second);
    }
}


}