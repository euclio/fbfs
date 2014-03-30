#include "Util.h"

#include <algorithm>
#include <iostream>
#include <string>

bool confirm_yes(const std::string &prompt, bool assume_yes) {
    std::string response;
    do {
        std::cout << prompt;

        if (assume_yes) {
            std::cout << " [Y/n]: ";
        } else {
            std::cout << " [y/N]: ";
        }

        std::getline(std::cin, response);

        if (response.length() == 0) {
            return assume_yes;
        }

        std::cout << "RAN GETLINE" << std::endl;

        // Make the string lower case
        std::transform(response.begin(), response.end(), response.begin(), ::tolower);
    } while (!std::cin.fail() && response != "y" && response != "n");

    return response == "y";
}
