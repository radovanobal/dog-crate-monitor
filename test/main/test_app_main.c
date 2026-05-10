#include "unity.h"

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[utils]", false);
    unity_run_tests_by_tag("[widgets]", false);
    UNITY_END();
}