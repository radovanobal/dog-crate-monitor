#include "unity.h"
#include "epaper_port.h"

#include "display/display_types.h"
#include "widgets/input_keyboard.h"

TEST_CASE("Input keyboard initializes with current text as a null pointer", "[widgets]")
{
    char currentText[MAX_TEXT_INPUT_LENGTH] = {0};
    inputKeyboard_init(currentText);
    TEST_ASSERT_TRUE(true); // If we reached this point without crashing, the test passes
    inputKeyboard_deinit();
}

TEST_CASE("Input keyboard initializes with current text", "[widgets]")
{
    char currentText[MAX_TEXT_INPUT_LENGTH] = "Hello, World!";
    inputKeyboard_init(currentText);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", inputKeyboard_getText());
    inputKeyboard_deinit();
}

TEST_CASE("Input keyboard sets the pixel space correctly", "[widgets]")
{
    char currentText[MAX_TEXT_INPUT_LENGTH] = {0};
    inputKeyboard_init(currentText);

    DisplayRenderPlan renderPlan = {0};
    inputKeyboard_buildRenderPlan(&renderPlan);

    TEST_ASSERT_EQUAL_INT(0, renderPlan.regions[0].pixelRegion.x);
    TEST_ASSERT_EQUAL_INT(0, renderPlan.regions[0].pixelRegion.y);  
    TEST_ASSERT_EQUAL_INT(EPD_WIDTH, renderPlan.regions[0].pixelRegion.width);
    TEST_ASSERT_EQUAL_INT(EPD_HEIGHT / 6, renderPlan.regions[0].pixelRegion.height);

    TEST_ASSERT_EQUAL_INT(0, renderPlan.regions[1].pixelRegion.x);
    TEST_ASSERT_EQUAL_INT(EPD_HEIGHT / 6, renderPlan.regions[1].pixelRegion.y);  
    TEST_ASSERT_EQUAL_INT(EPD_WIDTH, renderPlan.regions[1].pixelRegion.width);
    TEST_ASSERT_EQUAL_INT(EPD_HEIGHT * 5 / 6, renderPlan.regions[1].pixelRegion.height);

    inputKeyboard_deinit();
}

TEST_CASE("Input keyboard should create a render plan with input text and keyboard scenes", "[widgets]")
{
    char currentText[MAX_TEXT_INPUT_LENGTH] = "Test";
    inputKeyboard_init(currentText);

    DisplayRenderPlan renderPlan = {0};
    inputKeyboard_buildRenderPlan(&renderPlan);

    bool hasInputTextScene = false;
    bool hasKeyboardScene = false;

    for (size_t i = 0; i < renderPlan.count; i++) {
        if (renderPlan.regions[i].regionId == 0) { // DISPLAY_REGION_SLOT_INPUT_TEXT
            hasInputTextScene = true;
        } else if (renderPlan.regions[i].regionId == 1) { // DISPLAY_REGION_SLOT_KEYBOARD
            hasKeyboardScene = true;
        }
    }

    TEST_ASSERT_TRUE(hasInputTextScene);
    TEST_ASSERT_TRUE(hasKeyboardScene);

    inputKeyboard_deinit();
}

TEST_CASE("Input keyboard should update the text when setText is called", "[widgets]")
{
    char currentText[MAX_TEXT_INPUT_LENGTH] = "Initial";
    inputKeyboard_init(currentText);

    inputKeyboard_setText("Updated Text");
    TEST_ASSERT_EQUAL_STRING("Updated Text", inputKeyboard_getText());

    inputKeyboard_deinit();
}