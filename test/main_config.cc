#include <iostream>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <string>
#include <map>
#include "log.h"
#include "config.h"

static co_lib::Logger::ptr g_logger = LOG_NAME("system");

co_lib::ConfigVar<int>::ptr g_int_value_config =
    co_lib::Config::Lookup("system.port", (int)8080, "system port");

co_lib::ConfigVar<std::vector<int>>::ptr vec =
    co_lib::Config::Lookup("vec", std::vector<int>(), "vector");

co_lib::ConfigVar<std::string>::ptr hh =
    co_lib::Config::Lookup("hh.name", (std::string)"name", "hh name");

co_lib::ConfigVar<std::map<std::string, std::string>>::ptr mp =
    co_lib::Config::Lookup("mp", std::map<std::string, std::string>(), "map");

int main() {
    YAML::Node root = YAML::LoadFile("../log.yml");
    co_lib::Config::LoadFromYaml(root);


    LOG_DEBUG(g_logger) << g_int_value_config->getValue();
    LOG_DEBUG(g_logger) << hh->getValue();
    for (auto& i : mp->getValue())
        LOG_DEBUG(g_logger) << i.second;

    for (auto& i : vec->getValue())
        LOG_DEBUG(g_logger) << i;
    
    return 0;
}