#include "unity.h"
#include "utils/utils.h"
#include "utils/macros.h"

TEST_CASE("max returns the maximum of two integers", "[utils]")
{
    TEST_ASSERT_EQUAL_INT(5, max(3, 5));
    TEST_ASSERT_EQUAL_INT(-1, max(-1, -5));
    TEST_ASSERT_EQUAL_INT(0, max(0, -10));
    TEST_ASSERT_EQUAL_INT(10, max(10, 10));
}

TEST_CASE("ARRAY_SIZE return the correct size of the array", "[utils]")
{
    int testArray[5];
    int testArrayContent[] = { 1, 2, 3, 4, 5 };

    TEST_ASSERT_EQUAL_UINT32(5, ARRAY_SIZE(testArray));
    TEST_ASSERT_EQUAL_UINT32(5, ARRAY_SIZE(testArrayContent));
}

TEST_CASE("ARRAY_SIZE returns 0 for empty arrays", "[utils]")
{
    int emptyArray[0];
    TEST_ASSERT_EQUAL_UINT32(0, ARRAY_SIZE(emptyArray));
}

TEST_CASE("ARRAY_SIZE works with const arrays", "[utils]")
{
    const char *const testStrings[] = { "one", "two", "three" };
    TEST_ASSERT_EQUAL_UINT32(3, ARRAY_SIZE(testStrings));
}

