/* Main entry stub for STM32F407 firmware */

extern "C" {

int main(void) {
    /* Placeholder main — will be replaced by target_main.cpp in FreeRTOS integration */
    while(1) {
        asm volatile("nop");
    }
    return 0;
}

}
