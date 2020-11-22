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
}


TEST_F(TestDoubleBuffer, test_YamlLoader) {
    const std::string DB_FILE = "../config.yaml";
    YAML::Node conf = YAML::LoadFile(DB_FILE.c_str());
    inf::utils::YamlLoader loader;
    loader.init(DB_FILE);
    YAML::Node loader_conf = loader.load();
    
    ASSERT_EQ(loader_conf["redis"]["redis_domain"].as<std::string>() , conf["redis"]["redis_domain"].as<std::string>() );

}
