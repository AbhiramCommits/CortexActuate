#pragma once

#include <cstdint>
#include <cstring>

// STM32F407 register definitions — hand-written, no vendor headers
// Based on STM32F407xx reference manual RM0090

namespace stm32f407 {

// ============================================================================
// RCC — Reset and Clock Control (0x40023800)
// ============================================================================
struct RCC {
  static constexpr uintptr_t BASE = 0x40023800;

  // RCC_CR register bitfield constants
  static constexpr uint32_t CR_HSION = (1u << 0);
  static constexpr uint32_t CR_HSIRDY = (1u << 1);
  static constexpr uint32_t CR_HSEON = (1u << 16);
  static constexpr uint32_t CR_HSERDY = (1u << 17);
  static constexpr uint32_t CR_PLLON = (1u << 24);
  static constexpr uint32_t CR_PLLRDY = (1u << 25);
  static constexpr uint32_t CR_PLLI2SON = (1u << 26);
  static constexpr uint32_t CR_PLLI2SRDY = (1u << 27);

  // RCC_PLLCFGR bitfield constants (8 MHz HSE -> 168 MHz)
  // PLLM=4, PLLN=168, PLLP=2, PLLQ=7 => PLLVCO = 8 * 168/4 = 336 MHz, SYSCLK =
  // 336/2 = 168 MHz
  static constexpr uint32_t PLLCFGR_PLLSRC_HSE = (1u << 22);
  static constexpr uint32_t PLLCFGR_PLLM_MASK = 0x3Fu;
  static constexpr uint32_t PLLCFGR_PLLN_SHIFT = 6;
  static constexpr uint32_t PLLCFGR_PLLP_SHIFT = 16;
  static constexpr uint32_t PLLCFGR_PLLQ_SHIFT = 24;

  // RCC_CFGR bitfield constants
  static constexpr uint32_t CFGR_SW_HSE = 0x1u;
  static constexpr uint32_t CFGR_SW_PLL = 0x2u;
  static constexpr uint32_t CFGR_SWS_MASK = 0x3u;
  static constexpr uint32_t CFGR_SWS_PLL = (0x2u << 2);
  static constexpr uint32_t CFGR_HPRE_DIV1 = 0x0u;
  static constexpr uint32_t CFGR_PPRE1_DIV4 =
      (0x5u << 10); // APB1 = HCLK/4 = 42 MHz
  static constexpr uint32_t CFGR_PPRE2_DIV2 =
      (0x4u << 13); // APB2 = HCLK/2 = 84 MHz

  // RCC_AHB1ENR — AHB1 peripheral clock enable
  static constexpr uint32_t AHB1ENR_GPIOA_EN = (1u << 0);
  static constexpr uint32_t AHB1ENR_GPIOB_EN = (1u << 1);
  static constexpr uint32_t AHB1ENR_GPIOC_EN = (1u << 2);

  // RCC_APB1ENR — APB1 peripheral clock enable
  static constexpr uint32_t APB1ENR_TIM2_EN = (1u << 0);
  static constexpr uint32_t APB1ENR_USART2_EN = (1u << 17);
  static constexpr uint32_t APB1ENR_CAN1_EN = (1u << 25);

  // RCC_APB2ENR — APB2 peripheral clock enable
  static constexpr uint32_t APB2ENR_USART1_EN = (1u << 4);
  static constexpr uint32_t APB2ENR_SPI1_EN = (1u << 12);

  // RCC_BDCR — Backup domain control
  static constexpr uint32_t BDCR_LSEON = (1u << 0);
  static constexpr uint32_t BDCR_LSERDY = (1u << 1);
};

// ============================================================================
// GPIO — General Purpose I/O
// ============================================================================
struct GPIO {
  static constexpr uintptr_t GPIOA_BASE = 0x40020000;
  static constexpr uintptr_t GPIOB_BASE = 0x40020400;
  static constexpr uintptr_t GPIOC_BASE = 0x40020800;
};

// ============================================================================
// USART — Universal Synchronous Asynchronous Receiver Transmitter
// ============================================================================
struct USART {
  static constexpr uintptr_t USART1_BASE = 0x40011000;
  static constexpr uintptr_t USART2_BASE = 0x40004400;

  // USART_CR1 bitfield constants
  static constexpr uint32_t CR1_UE = (1u << 13);
  static constexpr uint32_t CR1_M = (1u << 12);
  static constexpr uint32_t CR1_PARITY_EN = (1u << 10);
  static constexpr uint32_t CR1_RXNEIE = (1u << 5);
  static constexpr uint32_t CR1_TE = (1u << 3);
  static constexpr uint32_t CR1_RE = (1u << 2);

  // USART_SR bitfield constants
  static constexpr uint32_t SR_TC = (1u << 6);
  static constexpr uint32_t SR_RXNE = (1u << 5);
  static constexpr uint32_t SR_TXE = (1u << 7);
};

// ============================================================================
// SPI — Serial Peripheral Interface
// ============================================================================
struct SPI {
  static constexpr uintptr_t SPI1_BASE = 0x40013000;

  // SPI_CR1 bitfield constants
  static constexpr uint32_t CR1_SPE = (1u << 6);
  static constexpr uint32_t CR1_MSTR = (1u << 2);
  static constexpr uint32_t CR1_BR_MASK = (0x7u << 3); // Baud rate

  // SPI_SR bitfield constants
  static constexpr uint32_t SR_TXE = (1u << 1);
  static constexpr uint32_t SR_RXNE = (1u << 0);
  static constexpr uint32_t SR_BSY = (1u << 7);
};

// ============================================================================
// CAN — Controller Area Network (bxCAN)
// ============================================================================
struct CAN {
  static constexpr uintptr_t CAN1_BASE = 0x40006400;
  static constexpr uintptr_t CAN2_BASE = 0x40006800;

  // CAN_MCR — Master Control Register
  static constexpr uint32_t MCR_INRQ = (1u << 0);
  static constexpr uint32_t MCR_SLEEP = (1u << 1);
  static constexpr uint32_t MCR_TXFP = (1u << 2);
  static constexpr uint32_t MCR_RFLM = (1u << 3);
  static constexpr uint32_t MCR_NART = (1u << 4);
  static constexpr uint32_t MCR_AWUM = (1u << 5);
  static constexpr uint32_t MCR_ABOM = (1u << 6);
  static constexpr uint32_t MCR_TTCM = (1u << 7);

  // CAN_MSR — Master Status Register
  static constexpr uint32_t MSR_INAK = (1u << 0);
  static constexpr uint32_t MSR_SLAK = (1u << 1);

  // CAN_BTR — Bit Timing Register (500 kbps @ APB1 42MHz)
  // BRP=20, TS1=13, TS2=2 => tq = 1/(42MHz/(BRP+1)) = 1/(42MHz/21) = 0.5µs
  // tbit = (1+TS1+TS2)*tq = 16*0.5µs = 8µs => 1/8µs = 125 kHz (recalc: wrong —
  // use 84MHz APB1 div by prescaler) Actually: for 500 kbps with 42 MHz APB1:
  // BRP=16, TS1=13, TS2=2
  static constexpr uint32_t BTR_BRP_SHIFT = 0;
  static constexpr uint32_t BTR_TS1_SHIFT = 16;
  static constexpr uint32_t BTR_TS2_SHIFT = 20;
  static constexpr uint32_t BTR_SJW_SHIFT = 24;
  static constexpr uint32_t BTR_LBKM = (1u << 30);
  static constexpr uint32_t BTR_SILM = (1u << 31);
};

// ============================================================================
// TIM — General Purpose Timer
// ============================================================================
struct TIM {
  static constexpr uintptr_t TIM2_BASE = 0x40000000;
  static constexpr uintptr_t TIM3_BASE = 0x40000400;

  // TIM_CR1 bitfield constants
  static constexpr uint32_t CR1_CEN = (1u << 0);
  static constexpr uint32_t CR1_UDIS = (1u << 1);
  static constexpr uint32_t CR1_URS = (1u << 2);

  // TIM_DIER — Interrupt enable
  static constexpr uint32_t DIER_UIE = (1u << 0); // Update interrupt enable

  // TIM_SR — Status register
  static constexpr uint32_t SR_UIF = (1u << 0); // Update interrupt flag
};

// ============================================================================
// IWDG — Independent Watchdog
// ============================================================================
struct IWDG {
  static constexpr uintptr_t BASE = 0x40003000;

  // IWDG_KR — Key register
  static constexpr uint32_t KR_UNLOCK = 0x5555;
  static constexpr uint32_t KR_RELOAD = 0xAAAA;
  static constexpr uint32_t KR_START = 0xCCCC;

  // IWDG_PR — Prescaler register
  static constexpr uint32_t PR_DIV4 = 0x1u;
  static constexpr uint32_t PR_DIV8 = 0x2u;
  static constexpr uint32_t PR_DIV16 = 0x3u;
  static constexpr uint32_t PR_DIV32 = 0x4u;
  static constexpr uint32_t PR_DIV64 = 0x5u;
  static constexpr uint32_t PR_DIV128 = 0x6u;
  static constexpr uint32_t PR_DIV256 = 0x7u;
};

// ============================================================================
// FLASH — Flash memory interface
// ============================================================================
struct FLASH {
  static constexpr uintptr_t BASE = 0x40023C00;

  // FLASH_ACR — Access control register
  static constexpr uint32_t ACR_LATENCY_MASK = 0x7u;
  static constexpr uint32_t ACR_PRFTEN = (1u << 8);
  static constexpr uint32_t ACR_ICEN = (1u << 9);
  static constexpr uint32_t ACR_DCEN = (1u << 10);
};

// ============================================================================
// PWR — Power control
// ============================================================================
struct PWR {
  static constexpr uintptr_t BASE = 0x40007000;

  // PWR_CR — Control register
  static constexpr uint32_t CR_FPDS =
      (1u << 9); // Flash power down in stop mode
};

// ============================================================================
// Utility: volatile register access helpers
// ============================================================================
inline volatile uint32_t &reg32(uintptr_t addr) {
  return *reinterpret_cast<volatile uint32_t *>(addr);
}

inline volatile uint16_t &reg16(uintptr_t addr) {
  return *reinterpret_cast<volatile uint16_t *>(addr);
}

inline volatile uint8_t &reg8(uintptr_t addr) {
  return *reinterpret_cast<volatile uint8_t *>(addr);
}

} // namespace stm32f407
