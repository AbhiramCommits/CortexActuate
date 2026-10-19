/* STM32F407 System Clock Initialization */
/* Configures clock tree: HSE 8MHz -> PLL -> 168MHz SYSCLK */

#include "../include/stm32f407_regs.hpp"
#include <stdint.h>

using namespace stm32f407;

extern "C" void SystemInit(void) {
  /* Enable HSE (external 8 MHz crystal) */
  volatile uint32_t &rcc_cr = reg32(RCC::BASE + 0x00);
  rcc_cr |= RCC::CR_HSEON;

  /* Wait for HSE to stabilize */
  while (!(rcc_cr & RCC::CR_HSERDY)) {
    asm volatile("nop");
  }

  /* Configure FLASH latency for 168 MHz (5 wait states) */
  volatile uint32_t &flash_acr = reg32(FLASH::BASE + 0x00);
  flash_acr = (flash_acr & ~FLASH::ACR_LATENCY_MASK) | 5;
  flash_acr |= FLASH::ACR_PRFTEN | FLASH::ACR_ICEN | FLASH::ACR_DCEN;

  /* Configure PLL: HSE 8MHz -> PLLVCO 336MHz -> SYSCLK 168MHz */
  /* PLLCFGR: PLLM=4, PLLN=168, PLLP=2, PLLQ=7 */
  /* Source = HSE, PLLVCO = 8 * (168/4) = 336 MHz, SYSCLK = 336/2 = 168 MHz, USB
   * = 336/7 = 48 MHz */
  volatile uint32_t &rcc_pllcfgr = reg32(RCC::BASE + 0x04);
  rcc_pllcfgr =
      (4 & RCC::PLLCFGR_PLLM_MASK) | /* PLLM occupies bits [5:0] */
      (168 << RCC::PLLCFGR_PLLN_SHIFT) |
      (0 << RCC::PLLCFGR_PLLP_SHIFT) | /* PLLP = 2 (div by 2) is code 00 */
      (7 << RCC::PLLCFGR_PLLQ_SHIFT) | RCC::PLLCFGR_PLLSRC_HSE;

  /* Enable PLL */
  rcc_cr |= RCC::CR_PLLON;

  /* Wait for PLL to lock */
  while (!(rcc_cr & RCC::CR_PLLRDY)) {
    asm volatile("nop");
  }

  /* Configure APB1 and APB2 prescalers */
  /* CFGR: HPRE=0 (AHB=168MHz), PPRE1=5 (APB1=168/4=42MHz), PPRE2=4
   * (APB2=168/2=84MHz) */
  volatile uint32_t &rcc_cfgr = reg32(RCC::BASE + 0x08);
  rcc_cfgr |= RCC::CFGR_PPRE1_DIV4 | RCC::CFGR_PPRE2_DIV2;

  /* Select PLL as SYSCLK source */
  rcc_cfgr = (rcc_cfgr & ~0x3u) | RCC::CFGR_SW_PLL;

  /* Wait for PLL to become SYSCLK source */
  while ((rcc_cfgr & RCC::CFGR_SWS_MASK) != RCC::CFGR_SWS_PLL) {
    asm volatile("nop");
  }

  /* Enable peripheral clocks needed for drivers */
  volatile uint32_t &rcc_ahb1enr = reg32(RCC::BASE + 0x30);
  rcc_ahb1enr |=
      RCC::AHB1ENR_GPIOA_EN | RCC::AHB1ENR_GPIOB_EN | RCC::AHB1ENR_GPIOC_EN;

  volatile uint32_t &rcc_apb1enr = reg32(RCC::BASE + 0x40);
  rcc_apb1enr |= RCC::APB1ENR_TIM2_EN | RCC::APB1ENR_CAN1_EN;

  volatile uint32_t &rcc_apb2enr = reg32(RCC::BASE + 0x44);
  rcc_apb2enr |= RCC::APB2ENR_USART1_EN | RCC::APB2ENR_SPI1_EN;
}
