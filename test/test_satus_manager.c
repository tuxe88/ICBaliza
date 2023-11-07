#include <unity.h>
#include "status_manager.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_status_0_on_init() {
    TEST_ASSERT_TRUE(1);
}

void app_main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_status_0_on_init);
    UNITY_END();
}