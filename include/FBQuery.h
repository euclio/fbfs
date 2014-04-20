#ifndef FBQUERY_H
#define FBQUERY_H

#include <string>
#include <vector>

typedef std::vector<std::pair<std::string, std::string>> parameters_t;

class FBQuery {
    public:
        FBQuery(std::string);
        FBQuery(std::string, parameters_t);
        FBQuery(std::string, std::string);
        FBQuery(std::string, std::string, parameters_t);
        FBQuery(std::string, std::string, std::string);
        FBQuery(std::string, std::string, std::string, parameters_t);
        std::string get_node() const;
        std::string get_endpoint() const;
        std::string get_edge() const;
        std::vector<std::pair<std::string, std::string>> get_parameters() const;
        void add_parameter(std::string, std::string);

    private:
        std::string node;
        std::string endpoint;
        std::string edge;
        parameters_t parameters;
};

#endif // FBQUERY_H
