#include "unity.h"

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[utils]", false);
    UNITY_END();
}