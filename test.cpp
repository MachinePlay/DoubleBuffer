#include "double_data.h"
#include <string>
#include <string>
#include <vector>
#include <thread>
#include <time.h>
#include <memory>
#include <iostream>
#define DEBUG_LOG(x) do { std::cout << x << std::endl;} while(0);
void display_vec(const std::vector<std::string> &raw_vecs) {
    for(auto &vec : raw_vecs) {
        std::cout << vec << std::endl;
    }
    std::cout << " " << std::endl;
}


int main(int argc, char* argv[]) {
    ::inf::utils::DoubleData<std::vector<std::string>> user_data;
    user_data.init(10, "front");

    auto ptr = user_data.get_data();
    int cnt = 10;
    for (int i = 0; i < 2; ++i) {
        // if (i % 2) {
            auto update_ptr = user_data.get_backup_data();
            update_ptr->push_back("backup");
            user_data.swap_dual_buffer();
        // }

        DEBUG_LOG("first")
        display_vec(*ptr);
        auto temp_ptr = user_data.get_data();
        DEBUG_LOG("second")
        display_vec(*temp_ptr);
    }
    
    
    return 0;
}