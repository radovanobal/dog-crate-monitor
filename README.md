# Project: Dog Crate Monitor
Dog Crate Monitor is a device being developed to monitor the environment inside a dog crate. My personal use case is to have a screen visible in my rearview mirror so I can check the temperature in the trunk where the dog crate is located. This helps me monitor conditions in the back of the car and adjust the AC as needed.

This is a project where I want to increase my experience in C development and create a working open-source tool that will actually help me (and hopefully others) when traveling with dogs.

Currently, the device displays temperature, humidity, and time. Eventually, I want to add features like flipping the screen so I can read the numbers from the rearview mirror. I also plan to include all the step files for the 3D-printed casing and mounting mechanisms for my dog crate.

In the future, I want to connect it to Home Assistant through a mobile hotspot and get notifications if the temperature goes higher than my automation settings. In summer, trunk temperatures can get uncomfortable quickly, and I want to be alerted before they do.

A distant goal is to broadcast the environmental data over BLE so I can display the temperature in Android Auto.

# Development Environment

Development is done in [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) v5.5.3

# Running Tests

This project currently uses an ESP-IDF Unity test app under `test/` to run targeted tests against app source files that still live under `main/`.

Current test layout:

- `test/CMakeLists.txt` defines the standalone test app.
- `test/main/test_app_main.c` is the Unity test runner entry point.
- `test/main/test_*.c` contains the test cases.
- `test/main/CMakeLists.txt` explicitly lists which source files from `main/` are compiled into the test app.

To build the test app without flashing the board:

```bash
cd test
idf.py set-target esp32s3
idf.py build
```

To flash the test app and open the monitor:

```bash
cd test
idf.py flash monitor
```

The current runner in `test/main/test_app_main.c` executes tests by tag using:

```c
unity_run_tests_by_tag("[utils]", false);
```

That means the test app will only run tests tagged `[utils]` until the runner is changed.

## Using the Interactive Unity Menu

If `test/main/test_app_main.c` uses `unity_run_menu()`, the test app waits for input from the active console.

If the menu prints output but does not accept input, check the test app console settings:

```bash
cd test
idf.py menuconfig
```

Recommended settings for the interactive menu:

- Set the primary console to the same interface you use for `idf.py monitor`.
- On this ESP32-S3 setup, `USB Serial/JTAG` is usually the simplest choice for interactive use.

Menu path:

- `Component config` -> `ESP System Settings` -> `Channel for console output`

If the Unity menu works but the board resets with a task watchdog timeout while waiting for input, adjust the test app watchdog settings:

- `Component config` -> `ESP System Settings` -> `Task Watchdog`
- Disable `Check Idle Task CPU0`
- Disable `Check Idle Task CPU1`

Those settings apply to the test app under `test/` and can be different from the main firmware configuration.

# Adding More Tests

To add another test for app code that still lives under `main/`:

1. Add a new `test_*.c` file under `test/main/`.
2. Add that new test file to `SRCS` in `test/main/CMakeLists.txt`.
3. Add the app source file you want to test from `../../main/...` to `SRCS` in `test/main/CMakeLists.txt` if it is not already there.
4. Make sure any needed include paths are available through `INCLUDE_DIRS`.
5. Tag the test with a Unity tag such as `[utils]`, or update `test/main/test_app_main.c` to run a different tag.

Minimal test example:

```c
#include "unity.h"
#include "utils/utils.h"

TEST_CASE("max returns the maximum of two integers", "[utils]")
{
	TEST_ASSERT_EQUAL_INT(5, max(3, 5));
	TEST_ASSERT_EQUAL_INT(-1, max(-1, -5));
}
```

If a test file compiles but Unity reports `0 Tests`, check that:

- the test file is listed in `test/main/CMakeLists.txt`
- the app source under test is also listed in `test/main/CMakeLists.txt`
- `WHOLE_ARCHIVE` is enabled in `test/main/CMakeLists.txt`
- the test uses `TEST_CASE(...)`

# Hardware

This device is based on the [Waveshare ESP32-S3 3.97-inch e-paper display](https://www.waveshare.com/esp32-s3-epaper-3.97.htm). This hardware was chosen because it has extremely low power consumption and long battery life—my car doesn't have a 12V port in the trunk. Since our trips sometimes last many hours with minimal charging opportunities, long battery life is essential.

The board integrates the e-paper screen, temperature and humidity sensors, and all the connectivity features like BLE and Wi-Fi that this project needs—all in one neat package. Plus, it has incredibly low power consumption, which is perfect for a device that potentially needs to run for days on a single charge.






