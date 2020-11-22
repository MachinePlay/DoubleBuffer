#include "double_buffer_test.h"
int main(int argc, char* argv[]) {
    testing::AddGlobalTestEnvironment(new DemoServerTest);
    testing::InitGoogleTest(&argc, argv);
    std::cout << "Testing Double Buffer" << std::endl;


    return RUN_ALL_TESTS();
}