#include "epaper_port.h"

esp_err_t initDisplay(void);
void renderToDisplay(float stateTemperatureC, float stateRelativeHumidity, const char *clockText);
