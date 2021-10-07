#include "double_buffer.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <atomic>

int main() {
    const std::string config_file = "test.yaml";
    //create file
    int fd = open(config_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 777);
    std::string content= "redis_domain: 10.0.0.10";
    write(fd, content.c_str(), content.size());
    close(fd);

    //use loader, init loader
    std::unique_ptr<::inf::utils::YamlLoader> loader = std::make_unique<::inf::utils::YamlLoader>();
    loader->init(config_file.c_str());

    //init double data
    auto config_data = new ::inf::utils::DoubleData<YAML::Node, ::inf::utils::YamlLoader>(std::move(loader));
    config_data->init();
    

    //use double data
    std::atomic<int> cnt = 0;
    while(true) {
        ++cnt;
        auto config_ptr = config_data->get_current();
        std::cout << "ptr1 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr2 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr3 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr4 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr5 "<<(*config_ptr)["redis_domain"] << std::endl;
        //do something
        sleep(1);
        std::cout << "ptr6 "<<(*config_ptr)["redis_domain"] << std::endl;
        
        if(cnt % 2 == 0) {
            sleep(1);
        }

    }


    
    std::cout << "Hello world" << std::endl;
    return 0;
}

