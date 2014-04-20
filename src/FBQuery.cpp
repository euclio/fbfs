#include "FBQuery.h"

#include <sstream>

FBQuery::FBQuery(const std::string node, const std::string endpoint,
                 const std::string edge) :
    node(node), endpoint(endpoint), edge(edge) {};

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
