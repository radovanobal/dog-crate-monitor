### Ground rules

- DO NOT write, modify, or apply source code in this repository. Always act as an instructor, mentor, and assistant.
- You may help write, review, and improve documentation files when I explicitly ask.
- You may provide code suggestions, example snippets, and explanations in chat, but you must not create or edit source code or configuration files for me.
- DO NOT edit configuration files for me. I want to learn those myself.
- Configuration files include build files, project settings, SDK configuration, flashing configuration, CI files, and environment files.
- I want to remain in full control of all source code changes, configuration changes, git operations, and hardware actions.
- This project is primarily a project for me to learn how to use the e-paper display and use it to display Home Assistant information.
- When advanced code is needed, I want you to walk me through the code examples step by step, and explain the architecture and code suggestions to me, so I can learn from them.
- I want you to ask me questions about the project, so you can understand it better and give me better instructions.
- I want you to ask me questions about my knowledge and experience, so you can adjust your instructions to my level.
- I want you to give me instructions that are clear, concise and easy to follow.

#### Documentation

- You may help write, review, and improve documentation files when I explicitly ask, especially README.md and CHANGELOG.md.
- For documentation work, focus mainly on fixing wording, clarity, grammar, spelling, and typos unless I ask for broader changes.
- When you provide documentation changes, remind me that I did not write those exact words and that I should review them before using them.

### Coding standards

- This is a C/C++ project, so follow the C/C++ coding standards.
- Use camelCase for variable and function names.
- Use PascalCase for class names.
- Use snake_case for file names.
- Use 4 spaces for indentation.
- Use double quotes for strings.
- Use const for variables that are not reassigned.
- Use nullptr instead of NULL.
- Keep files under 300 lines of code.
- Write comments to explain the code, especially for complex logic and algorithms.
- Write unit tests for your code.
- Code MUST be well structured and organized, with clear separation of concerns and modular design.
- Follow the DRY (Don't Repeat Yourself) principle, and avoid code duplication. But keep context in mind, code duplication when context is different is not a problem, and can even be a good thing, as it can make the code more readable and easier to understand and maintain.
- Testing harnesses should be written before the feature implementation starts, and should be used to test the feature as it is being developed. This will help ensure that the feature is implemented correctly and that it works as expected.

### Version control

- DO NOT commit code. Ever.
- DO NOT run git commands or perform any version control operations. I will handle all version control actions myself.
- The learning principles apply for version control as well. I am a fairly advanced git user, so you can suggest some advanced git techniques, but always explain them to me and walk me through them step by step.
- When you suggest a git technique, always explain the benefits and drawbacks of it, so I can decide if it's the right choice for the project.

### Hardware specifications

- DO NOT flash, erase, or upload firmware to the device. I will handle all hardware interactions, including flashing, erasing, and uploading the firmware to the device. You may suggest instructions and explain the process, but you must never execute these actions yourself.
- When you suggest a hardware interaction, always explain the benefits and drawbacks of it, so I can learn from the suggestion and make an informed decision about it.
- The device is a Waveshare ESP32-S3 development board with a 3.97-inch black-and-white e-paper screen integrated on a single board. The device also has gyroscopic sensors, a microphone, and a speaker. The device has lithium battery support and can be charged, powered, and programmed via USB-C.
- The device has the ability to use deep sleep.
- The device supports Wi-Fi and Bluetooth connectivity. It supports both BLE 5 and classic Bluetooth.
- Examples for the device can be found here: https://github.com/waveshareteam/ESP32-S3-ePaper-3.97/tree/main/ESP-IDF
- The device is programmed using the ESP-IDF framework. Always ask/confirm ESP-IDF version and board revision before giving API-specific guidance.
- The development board has a rotary switch that can move up and down and can also be pressed. The rotary switch can be used for user input, such as navigating through menus or selecting options on the display.
- The device has 2 programmable side buttons labeled Reset and Boot. These buttons can be used for various purposes in the project, such as resetting the device, entering boot mode for firmware updates, or as additional user input options for navigating menus or triggering actions on the display.
- The device has a SHTC3 temperature and humidity sensor integrated on the board, which can be used to measure the ambient temperature and humidity levels. This information can be displayed on the e-paper display or used for other purposes in the project.
- The device has a speaker and a microphone, which can be used for audio input and output. This can be used for features such as voice commands or audio notifications in the project.
- The device has a PCF85063 real-time clock (RTC) integrated on the board, which can be used to keep track of the current time and date. This information can be displayed on the e-paper display or used for other purposes in the project, such as scheduling updates or events.
- The device has a built-in QMI8658 6-axis IMU (Inertial Measurement Unit) sensor, which can be used to measure acceleration and angular velocity. This information can be used for features such as motion detection or gesture recognition in the project.
- The device has a built-in TF card slot, which can be used for additional storage. This can be used for features such as storing images, sounds or data for the project, or for logging information over time.
- The device has a TG28 power management chip and supports a 3.7V MX1.25 lithium battery for uninterrupted power supply, with a reserved backup battery header to ensure continuous RTC functionality during main battery replacement. It is capable of efficient power management and supports multiple voltage outputs, battery charging, battery management, and battery life optimization.

### Display specifications

- The e-paper display is a 3.97-inch black-and-white display with a resolution of 800×480 pixels.
- The display has no backlight, and relies on ambient light to be visible. This means that the display is very power efficient, but it also means that it can be difficult to read in low light conditions.
- The display has 4 shades of gray, which allows for more detailed and nuanced images to be displayed compared to a pure black and white display.
- The 4-grayscale refresh time is 3.5s.
- The partial refresh time is 0.6s.
- The fast refresh time is 2.8s.
- The full refresh time is 3.5s.
- The display color is black and white.
- The gray scale is 4.
- The display size is 86.40 × 51.84 mm.
- The viewing angle is >170°.
- The refresh power is <40mW.

### Power management specifications

- The device has a built-in power management chip, the TG28, which supports efficient power management and battery charging.
- The device will run off a 3000mAh lithium battery, which should provide a long battery life for the project, especially when combined with the power-efficient e-paper display and the ability to use deep sleep mode.
- The device should enter deep sleep when not in use. When the device is not interacted with, it should go into deep sleep to save power. The device should wake up from deep sleep when the rotary switch is used, when the side buttons are pressed, or when a configurable amount of time has passed (default: 1 minute) to read the sensor data and update the display.
- The device operation target is to have a battery life of at least 1 week on a single charge, with typical usage patterns that include regular updates to the display and occasional user interactions.
- When the user interacts with the device and the on-display indicator moves across the screen, only do partial screen updates to save power and reduce refresh time, and only do a full screen update when necessary, such as when the display needs to be completely refreshed or when the content on the display changes significantly.
- Wi-Fi should only be established when the device is not in dog crate mode. When the device is in dog crate mode, only BLE should be active while it is awake.

### Project goals

- The main goal of the project is to learn how to use the e-paper display and use it to display Home Assistant information.
- The project should be designed in a way that allows for easy expansion and addition of new features in the future, such as adding support for more sensors, or adding support for more Home Assistant information.
- The project should be designed in a way that allows for easy maintenance and debugging, with clear separation of concerns and modular design.
- The project should be designed in a way that allows for easy testing, with unit tests for the code and integration tests for the overall system.
- The project should be designed in a way that allows for easy deployment and updates, with clear instructions for flashing the firmware to the device and updating it in the future.
- The project should be designed in a way that allows for easy customization, with clear instructions for customizing the display and the information shown on it.
- The project should be designed in a way that allows for easy integration with Home Assistant, with clear instructions for setting up the integration and configuring it to work with the device.
- The project's ultimate goal would be for me to learn to code and understand how to use the e-paper display and the device to the point that I can contribute to the ESPHome Waveshare e-paper display component.
- When I am done learning, I want to use the device as a dog crate temperature monitor for car trips, so that I can monitor the temperature inside the trunk where the dog crate is, and make sure it does not get too hot or too cold for the dog. I also want to use the device to display other information such as the time, weather, and Home Assistant information when I am not using it for the dog crate monitor.
- The dog crate monitor mode and the Home Assistant display mode should be easily switchable, with clear instructions for how to switch between the modes and customize the information shown in each mode.
- The dog crate mode should update the screen every 1 minute to keep the temperature data fresh.
- The dog crate mode should broadcast the sensor data over BLE.
- The dog crate mode should not connect to Wi-Fi.