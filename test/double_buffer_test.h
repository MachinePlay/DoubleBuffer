#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"
#include "double_buffer.h"
#include <string>
#include <iostream>
#include <memory>
#include <cstdint>

/**
 * @class DemoServerTest
 * @desc 测试DemoServer环境初始化类
 */
class DemoServerTest : public testing::Environment {
public:
    /* init test environment. */
    virtual void SetUp() {};

    /* clear test environment. */
    virtual void TearDown() {}; 
};

/*
 * @class TestDoubleBufferManager
 */
class TestDoubleBuffer : public testing::Test {
public:
    /* start test. */
    static void SetUpTestCace() {
        std::cout << "StartDoubleBufferTesting." << std::endl;
    }
    
    /* end test. */
    static void TearDownTestCase() {
        std::cout << "TearDownDoubleBufferTesting." << std::endl;
    }

};


TEST_F(TestDoubleBuffer, test_DoubleBuffer)  {
    ASSERT_EQ(1, 1);
    //test init
    const std::string DB_FILE = "../config.yaml";
    std::unique_ptr<::inf::utils::YamlLoader> loader_ptr = std::make_unique<::inf::utils::YamlLoader>();
    loader_ptr->init(DB_FILE.c_str());
    ::inf::utils::DoubleData<YAML::Node, ::inf::utils::YamlLoader> data(std::move(loader_ptr));
    YAML::Node conf = YAML::LoadFile(DB_FILE.c_str());
    data.init();
    auto cur = data.get_current();
    
    
    // //front
    // ASSERT_EQ((*cur)["redis"]["redis_domain"].as<std::string>() , conf["redis"]["redis_domain"].as<std::string>());
    
    // //backup
    // ASSERT_EQ((*backup)["redis"]["redis_domain"].as<std::string>() , conf["redis"]["redis_domain"].as<std::string>()); 

    // //backup and front
    // sleep(2);
    // ASSERT_EQ((*backup)["redis"]["redis_domain"].as<std::string>() , (*cur)["redis"]["redis_domain"].as<std::string>()); 

    //after swap
    bool swap_ret = data.swap_data();
    auto backup = data.get_backup();
    ASSERT_EQ(true, swap_ret);
    ASSERT_EQ((*backup)["redis"]["redis_domain"].as<std::string>() , (*cur)["redis"]["redis_domain"].as<std::string>()); 

    

    
}

TEST_F(TestDoubleBuffer, test_SwithMonitor)  {
    //init
    const std::string DB_FILE = "../config.yaml";
    ::inf::utils::SwitchMonitor monitor;
    monitor.init(DB_FILE);

    //asert file number
    auto file_list = monitor.get_monitor_file_list();
    size_t ret = file_list.count(DB_FILE);
    ASSERT_EQ(1, ret);
    struct stat test_file_status;
    stat(DB_FILE.c_str(), &test_file_status);
    //asert file stat
    ASSERT_EQ(test_file_status.st_mtime, file_list[DB_FILE]);
}

//test YamlLoader
TEST_F(TestDoubleBuffer, test_YamlLoader) {
    const std::string DB_FILE = "../config.yaml";
    YAML::Node conf = YAML::LoadFile(DB_FILE.c_str());
    inf::utils::YamlLoader loader;
    loader.init(DB_FILE);
    YAML::Node loader_conf = loader.load();
    
    ASSERT_EQ(loader_conf["redis"]["redis_domain"].as<std::string>() , conf["redis"]["redis_domain"].as<std::string>() );

}
