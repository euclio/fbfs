#include "FBQuery.h"

#include <sstream>

FBQuery::FBQuery(std::string node) :
    FBQuery(node, "", "", parameters_t()) {};

FBQuery::FBQuery(std::string node, parameters_t parameters) :
    FBQuery(node, "", "", parameters) {};

FBQuery::FBQuery(std::string node, std::string endpoint) :
    FBQuery(node, endpoint, "", parameters_t()) {};

FBQuery::FBQuery(std::string node, std::string endpoint, parameters_t parameters) :
    FBQuery(node, endpoint, "", parameters) {};

FBQuery::FBQuery(std::string node, std::string endpoint, std::string edge) :
    FBQuery(node, endpoint, edge, parameters_t()) {};

FBQuery::FBQuery(std::string node, std::string endpoint, std::string edge,
                 parameters_t parameters) :
    node(node), endpoint(endpoint), edge(edge), parameters(parameters) {};

void FBQuery::add_parameter(std::string key, std::string value) {
    parameters.push_back(make_pair(key, value));
}

std::string FBQuery::get_node() const {
    return node;
}

std::string FBQuery::get_endpoint() const {
    return endpoint;
}

std::string FBQuery::get_edge() const {
    return edge;
}

std::vector<std::pair<std::string, std::string>> FBQuery::get_parameters() const {
    return parameters;
}
