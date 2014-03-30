class FBGraph {
    public:
        bool is_logged_in() const;
        void authenticate();
        void login();
    private:
        bool logged_in;
};
