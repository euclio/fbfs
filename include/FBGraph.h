#ifndef FBGRAPH_H
#define FBGRAPH_H

class FBGraph {
    public:
        bool is_logged_in() const;
        void authenticate();
        void login();
    private:
        bool logged_in;
};

#endif // FBGRAPH_H
