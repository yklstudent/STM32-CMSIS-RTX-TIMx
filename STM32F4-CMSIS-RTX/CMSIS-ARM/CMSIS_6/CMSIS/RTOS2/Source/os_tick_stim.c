/**************************************************************************//**
 * @file     os_systick.c
 * @brief    CMSIS OS Tick SysTick implementation
 * @version  V1.0.5
 * @date     16. October 2023
 ******************************************************************************/
/*
 * Copyright (c) 2017-2023 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "os_tick.h"
#include "main.h"

//lint -emacro((923,9078),SCB,SysTick) "cast from unsigned long to pointer"

#if defined(_RTE_)
#include "RTE_Components.h"
#elif !defined(CMSIS_device_header)
#error "CMSIS_device_header must be defined to point to CMSIS device header"
#endif

#include CMSIS_device_header

#if (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 0))

#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  load = (SystemCoreClock / freq) - 1U;
  if (load > 0x00FFFFFFU) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  // Set SysTick Interrupt Priority
#if   ((defined(__ARM_ARCH_8M_MAIN__)   && (__ARM_ARCH_8M_MAIN__   != 0)) || \
       (defined(__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ != 0)) || \
       (defined(__CORTEX_M)             && (__CORTEX_M             == 7U)))
  SCB->SHPR[11] = SYSTICK_IRQ_PRIORITY;
#elif  (defined(__ARM_ARCH_8M_BASE__)   && (__ARM_ARCH_8M_BASE__   != 0))
  SCB->SHPR[1] |= ((uint32_t)SYSTICK_IRQ_PRIORITY << 24);
#elif ((defined(__ARM_ARCH_7M__)        && (__ARM_ARCH_7M__        != 0)) || \
       (defined(__ARM_ARCH_7EM__)       && (__ARM_ARCH_7EM__       != 0)))
  SCB->SHPR[11]  = SYSTICK_IRQ_PRIORITY;
#elif  (defined(__ARM_ARCH_6M__)        && (__ARM_ARCH_6M__        != 0))
  SCB->SHPR[1]  |= ((uint32_t)SYSTICK_IRQ_PRIORITY << 24);
#else
#error "Unknown ARM Core!"
#endif

  SysTick->CTRL =  SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
  SysTick->LOAD =  load;
  SysTick->VAL  =  0U;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
  }

  SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

  if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0U) {
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)SysTick->CTRL;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)SysTick_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (SysTick->LOAD + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = SysTick->VAL;
  if (val != 0U) {
    count = (SysTick->LOAD - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) >> SCB_ICSR_PENDSTSET_Pos);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 1))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
  
  /* Configure the NVIC to handle TIM1 update interrupt */
  NVIC_SetPriority(TIM1_UP_TIM10_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  
  /* Set the pre-scaler value to have TIM1 counter clock equal to 1000 kHz      */
  /*
    In this example TIM1 input clock (TIM1CLK)  is set to APB2 clock (PCLK2),
    since APB2 prescaler is equal to 1.
      TIM1CLK = PCLK2 * 2
      PCLK2 = HCLK
      => TIM2CLK = HCLK = SystemCoreClock
    To get TIM1 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM1CLK / TIM1 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM1->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM1CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock, TIM1->PSC, freq);
  TIM1->ARR = load;
  
  /* Enable the update interrupt */
  TIM1->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM1->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM1->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM1->DIER |= TIM_DIER_UIE;
  }

  TIM1->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM1->CR1 &= ~TIM_CR1_CEN;

  if ((TIM1->DIER & TIM_DIER_UIE) != 0U) {
    TIM1->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM1->CR1;
  (void)TIM1->CR2;

  TIM1->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM1_UP_TIM10_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM1->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM1->CNT;
  if (val != 0U) {
    count = (TIM1->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM1->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 2))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
  
  /* Configure the NVIC to handle TIM2 update interrupt */
  NVIC_SetPriority(TIM2_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM2_IRQn);
  
  /* Set the pre-scaler value to have TIM2 counter clock equal to 1000 kHz      */
  /*
    In this example TIM2 input clock (TIM2CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM2CLK = PCLK1 * 2
      PCLK1 = HCLK / 4
      => TIM2CLK = HCLK / 2 = SystemCoreClock / 2
    To get TIM2 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM2 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM2->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM2CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM2->PSC, freq);
  TIM2->ARR = load;
  
  /* Enable the update interrupt */
  TIM2->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM2->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM2->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM2->DIER |= TIM_DIER_UIE;
  }

  TIM2->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM2->CR1 &= ~TIM_CR1_CEN;

  if ((TIM2->DIER & TIM_DIER_UIE) != 0U) {
    TIM2->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM2->CR1;
  (void)TIM2->CR2;

  TIM2->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM2_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM2->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM2->CNT;
  if (val != 0U) {
    count = (TIM2->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM2->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 3))

#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
  
  /* Configure the NVIC to handle TIM3 update interrupt */
  NVIC_SetPriority(TIM3_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM3_IRQn);
  
  /* Set the pre-scaler value to have TIM3 counter clock equal to 1000 kHz      */
  /*
    In this example TIM3 input clock (TIM3CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM3CLK = PCLK1 * 2
      PCLK1 = HCLK / 4
      => TIM3CLK = HCLK / 2 = SystemCoreClock / 2
    To get TIM3 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM3->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM3CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM3->PSC, freq);
  TIM3->ARR = load;
  
  /* Enable the update interrupt */
  TIM3->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM3->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM3->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM3->DIER |= TIM_DIER_UIE;
  }

  TIM3->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM3->CR1 &= ~TIM_CR1_CEN;

  if ((TIM3->DIER & TIM_DIER_UIE) != 0U) {
    TIM3->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM3->CR1;
  (void)TIM3->CR2;

  TIM3->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM3_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM3->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM3->CNT;
  if (val != 0U) {
    count = (TIM3->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM3->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 4))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
  
  /* Configure the NVIC to handle TIM3 update interrupt */
  NVIC_SetPriority(TIM4_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM4_IRQn);
  
  /* Set the pre-scaler value to have TIM4 counter clock equal to 1000 kHz      */
  /*
    In this example TIM4 input clock (TIM4CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM4CLK = PCLK1 * 2
      PCLK1 = HCLK / 4
      => TIM4CLK = HCLK / 2 = SystemCoreClock / 2
    To get TIM4 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM4CLK / TIM4 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM4->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM4CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM4->PSC, freq);
  TIM4->ARR = load;
  
  /* Enable the update interrupt */
  TIM4->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM4->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM4->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM4->DIER |= TIM_DIER_UIE;
  }

  TIM4->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM4->CR1 &= ~TIM_CR1_CEN;

  if ((TIM4->DIER & TIM_DIER_UIE) != 0U) {
    TIM4->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM4->CR1;
  (void)TIM4->CR2;

  TIM4->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM4_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM4->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM4->CNT;
  if (val != 0U) {
    count = (TIM4->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM4->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 5))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM5);
  
  /* Configure the NVIC to handle TIM5 update interrupt */
  NVIC_SetPriority(TIM5_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM5_IRQn);
  
  /* Set the pre-scaler value to have TIM5 counter clock equal to 1000 kHz      */
  /*
    In this example TIM5 input clock (TIM5CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM5CLK = PCLK1 * 2
      PCLK1 = HCLK / 4
      => TIM5CLK = HCLK / 2 = SystemCoreClock / 2
    To get TIM3 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM5CLK / TIM5 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM5->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM5CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM5->PSC, freq);
  TIM5->ARR = load;
  
  /* Enable the update interrupt */
  TIM5->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM5->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM5->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM5->DIER |= TIM_DIER_UIE;
  }

  TIM5->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM5->CR1 &= ~TIM_CR1_CEN;

  if ((TIM5->DIER & TIM_DIER_UIE) != 0U) {
    TIM5->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM5->CR1;
  (void)TIM5->CR2;

  TIM5->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM5_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM5->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM5->CNT;
  if (val != 0U) {
    count = (TIM5->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM5->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 6))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);
  
  /* Configure the NVIC to handle TIM3 update interrupt */
  NVIC_SetPriority(TIM6_DAC_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM6_DAC_IRQn);
  
  /* Set the pre-scaler value to have TIM6 counter clock equal to 1000 kHz      */
  /*
    In this example TIM6 input clock (TIM3CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM6CLK = PCLK1 * 2
      PCLK1 = HCLK / 4
      => TIM6CLK = HCLK / 2 = SystemCoreClock / 2
    To get TIM6 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM6CLK / TIM6 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM6->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM6CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM6->PSC, freq);
  TIM6->ARR = load;
  
  /* Enable the update interrupt */
  TIM6->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM6->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM6->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM6->DIER |= TIM_DIER_UIE;
  }

  TIM6->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM6->CR1 &= ~TIM_CR1_CEN;

  if ((TIM6->DIER & TIM_DIER_UIE) != 0U) {
    TIM6->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM6->CR1;
  (void)TIM6->CR2;

  TIM6->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM6_DAC_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM6->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM6->CNT;
  if (val != 0U) {
    count = (TIM6->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM6->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 7))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM7);
  
  /* Configure the NVIC to handle TIM7 update interrupt */
  NVIC_SetPriority(TIM7_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM7_IRQn);
  
  /* Set the pre-scaler value to have TIM7 counter clock equal to 1000 kHz      */
  /*
    In this example TIM7 input clock (TIM7CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM7CLK = PCLK1 * 2
      PCLK1 = HCLK / 4
      => TIM7CLK = HCLK / 2 = SystemCoreClock / 2
    To get TIM7 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM7CLK / TIM7 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM7->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM7CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM7->PSC, freq);
  TIM7->ARR = load;
  
  /* Enable the update interrupt */
  TIM7->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM7->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM7->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM7->DIER |= TIM_DIER_UIE;
  }

  TIM7->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM7->CR1 &= ~TIM_CR1_CEN;

  if ((TIM7->DIER & TIM_DIER_UIE) != 0U) {
    TIM7->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM7->CR1;
  (void)TIM7->CR2;

  TIM7->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM7_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM7->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM7->CNT;
  if (val != 0U) {
    count = (TIM7->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM7->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 8))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8);
  
  /* Configure the NVIC to handle TIM8 update interrupt */
  NVIC_SetPriority(TIM8_UP_TIM13_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
  
  /* Set the pre-scaler value to have TIM8 counter clock equal to 1000 kHz      */
  /*
    In this example TIM8 input clock (TIM8CLK)  is set to APB2 clock (PCLK2),
    since APB2 prescaler is equal to 1.
      TIM8CLK = PCLK2 * 2
      PCLK2 = HCLK / 2
      => TIM8CLK = HCLK = SystemCoreClock
    To get TIM8 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM8CLK / TIM8 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM8->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM8CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock, TIM8->PSC, freq);
  TIM8->ARR = load;
  
  /* Enable the update interrupt */
  TIM8->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM8->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM8->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM8->DIER |= TIM_DIER_UIE;
  }

  TIM8->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM8->CR1 &= ~TIM_CR1_CEN;

  if ((TIM8->DIER & TIM_DIER_UIE) != 0U) {
    TIM8->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM8->CR1;
  (void)TIM8->CR2;

  TIM8->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM8_UP_TIM13_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM8->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM8->CNT;
  if (val != 0U) {
    count = (TIM8->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM8->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 9))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM9);
  
  /* Configure the NVIC to handle TIM9 update interrupt */
  NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
  
  /* Set the pre-scaler value to have TIM9 counter clock equal to 1000 kHz      */
  /*
    In this example TIM9 input clock (TIM9CLK)  is set to APB2 clock (PCLK2),
    since APB2 prescaler is equal to 1.
      TIM9CLK = PCLK2 * 2
      PCLK2 = HCLK / 2
      => TIM9CLK = HCLK = SystemCoreClock
    To get TIM9 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM9CLK / TIM9 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM9->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM9CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock, TIM9->PSC, freq);
  TIM9->ARR = load;
  
  /* Enable the update interrupt */
  TIM9->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM9->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM9->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM9->DIER |= TIM_DIER_UIE;
  }

  TIM9->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM9->CR1 &= ~TIM_CR1_CEN;

  if ((TIM9->DIER & TIM_DIER_UIE) != 0U) {
    TIM9->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM9->CR1;
  (void)TIM9->CR2;

  TIM9->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM1_BRK_TIM9_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM9->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM9->CNT;
  if (val != 0U) {
    count = (TIM9->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM9->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 10))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM10);
  
  /* Configure the NVIC to handle TIM10 update interrupt */
  NVIC_SetPriority(TIM1_UP_TIM10_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  
  /* Set the pre-scaler value to have TIM10 counter clock equal to 1000 kHz      */
  /*
    In this example TIM10 input clock (TIM10CLK)  is set to APB2 clock (PCLK2),
    since APB2 prescaler is equal to 1.
      TIM10CLK = PCLK2 * 2
      PCLK1 = HCLK / 2
      => TIM10CLK = HCLK = SystemCoreClock
    To get TIM10 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM10CLK / TIM10 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM10->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM10CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock, TIM10->PSC, freq);
  TIM10->ARR = load;
  
  /* Enable the update interrupt */
  TIM10->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM10->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM10->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM10->DIER |= TIM_DIER_UIE;
  }

  TIM10->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM10->CR1 &= ~TIM_CR1_CEN;

  if ((TIM10->DIER & TIM_DIER_UIE) != 0U) {
    TIM10->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM10->CR1;
  (void)TIM10->CR2;

  TIM10->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM1_UP_TIM10_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM10->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM10->CNT;
  if (val != 0U) {
    count = (TIM10->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM10->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 11))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM11);
  
  /* Configure the NVIC to handle TIM11 update interrupt */
  NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
  
  /* Set the pre-scaler value to have TIM11 counter clock equal to 1000 kHz      */
  /*
    In this example TIM11 input clock (TIM11CLK)  is set to APB2 clock (PCLK2),
    since APB2 prescaler is equal to 1.
      TIM11CLK = PCLK2 * 2
      PCLK2 = HCLK / 2
      => TIM11CLK = HCLK = SystemCoreClock
    To get TIM11 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM11CLK / TIM11 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM11->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM11CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock, TIM11->PSC, freq);
  TIM11->ARR = load;
  
  /* Enable the update interrupt */
  TIM11->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM11->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM11->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM11->DIER |= TIM_DIER_UIE;
  }

  TIM11->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM11->CR1 &= ~TIM_CR1_CEN;

  if ((TIM11->DIER & TIM_DIER_UIE) != 0U) {
    TIM11->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM11->CR1;
  (void)TIM11->CR2;

  TIM11->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM1_TRG_COM_TIM11_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM11->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM11->CNT;
  if (val != 0U) {
    count = (TIM11->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM11->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 12))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM12);
  
  /* Configure the NVIC to handle TIM12 update interrupt */
  NVIC_SetPriority(TIM8_BRK_TIM12_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);
  
  /* Set the pre-scaler value to have TIM12 counter clock equal to 1000 kHz      */
  /*
    In this example TIM12 input clock (TIM12CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM12CLK = PCLK1
      PCLK1 = HCLK
      => TIM12CLK = HCLK = SystemCoreClock
    To get TIM12 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM12CLK / TIM12 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM12->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM12CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM12->PSC, freq);
  TIM12->ARR = load;
  
  /* Enable the update interrupt */
  TIM12->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM12->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM12->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM12->DIER |= TIM_DIER_UIE;
  }

  TIM12->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM12->CR1 &= ~TIM_CR1_CEN;

  if ((TIM12->DIER & TIM_DIER_UIE) != 0U) {
    TIM12->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM12->CR1;
  (void)TIM12->CR2;

  TIM12->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM8_BRK_TIM12_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM12->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM12->CNT;
  if (val != 0U) {
    count = (TIM12->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM12->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 13))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM13);
  
  /* Configure the NVIC to handle TIM13 update interrupt */
  NVIC_SetPriority(TIM8_UP_TIM13_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
  
  /* Set the pre-scaler value to have TIM13 counter clock equal to 1000 kHz      */
  /*
    In this example TIM13 input clock (TIM13CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM13CLK = PCLK1
      PCLK1 = HCLK
      => TIM13CLK = HCLK = SystemCoreClock
    To get TIM13 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM13CLK / TIM13 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM13->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM13CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM13->PSC, freq);
  TIM13->ARR = load;
  
  /* Enable the update interrupt */
  TIM13->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM13->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM13->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM13->DIER |= TIM_DIER_UIE;
  }

  TIM13->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM13->CR1 &= ~TIM_CR1_CEN;

  if ((TIM13->DIER & TIM_DIER_UIE) != 0U) {
    TIM13->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM13->CR1;
  (void)TIM13->CR2;

  TIM13->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM8_UP_TIM13_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM13->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM13->CNT;
  if (val != 0U) {
    count = (TIM13->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM13->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#elif (defined(CMSIS_RTX_OS_TICK) && (CMSIS_RTX_OS_TICK == 14))


#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t prescaler;
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  /* Enable the timer peripheral clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);
  
  /* Configure the NVIC to handle TIM14 update interrupt */
  NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, SYSTICK_IRQ_PRIORITY);
  NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
  
  /* Set the pre-scaler value to have TIM14 counter clock equal to 1000 kHz      */
  /*
    In this example TIM14 input clock (TIM14CLK)  is set to APB1 clock (PCLK1),
    since APB1 prescaler is equal to 1.
      TIM14CLK = PCLK1
      PCLK1 = HCLK
      => TIM14CLK = HCLK = SystemCoreClock
    To get TIM14 counter clock at 1000 KHz, the Prescaler is computed as following:
    Prescaler = (TIM14CLK / TIM14 counter clock) - 1
    Prescaler = (SystemCoreClock /1000 KHz) - 1
  */
  prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 1000000);
  TIM14->PSC = prescaler;

  /* Set the auto-reload value to have an initial update event frequency of 1000 Hz */
    /* TIM14CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
  
  load = __LL_TIM_CALC_ARR(SystemCoreClock / 2, TIM14->PSC, freq);
  TIM14->ARR = load;
  
  /* Enable the update interrupt */
  TIM14->DIER |= TIM_DIER_UIE;
  
  /* Enable counter */
  // TIM14->CR1 |= TIM_CR1_CEN;
  
  /* Force update generation */
  TIM14->EGR |= TIM_EGR_UG;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    TIM14->DIER |= TIM_DIER_UIE;
  }

  TIM14->CR1 |= TIM_CR1_CEN;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  TIM14->CR1 &= ~TIM_CR1_CEN;

  if ((TIM14->DIER & TIM_DIER_UIE) != 0U) {
    TIM14->DIER &= ~TIM_DIER_UIE;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)TIM14->CR1;
  (void)TIM14->CR2;

  TIM14->SR &= ~TIM_SR_UIF;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)TIM8_TRG_COM_TIM14_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock / 2);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (TIM14->CNT + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = TIM14->CNT;
  if (val != 0U) {
    count = (TIM14->ARR - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((TIM14->SR & TIM_SR_UIF) == TIM_SR_UIF);
}

#else

#ifndef SYSTICK_IRQ_PRIORITY
#define SYSTICK_IRQ_PRIORITY    0xFFU
#endif

static uint8_t PendST __attribute__((section(".bss.os")));

// Setup OS Tick.
__WEAK int32_t OS_Tick_Setup (uint32_t freq, IRQHandler_t handler) {
  uint32_t load;
  (void)handler;

  if (freq == 0U) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  load = (SystemCoreClock / freq) - 1U;
  if (load > 0x00FFFFFFU) {
    //lint -e{904} "Return statement before end of function"
    return (-1);
  }

  // Set SysTick Interrupt Priority
#if   ((defined(__ARM_ARCH_8M_MAIN__)   && (__ARM_ARCH_8M_MAIN__   != 0)) || \
       (defined(__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ != 0)) || \
       (defined(__CORTEX_M)             && (__CORTEX_M             == 7U)))
  SCB->SHPR[11] = SYSTICK_IRQ_PRIORITY;
#elif  (defined(__ARM_ARCH_8M_BASE__)   && (__ARM_ARCH_8M_BASE__   != 0))
  SCB->SHPR[1] |= ((uint32_t)SYSTICK_IRQ_PRIORITY << 24);
#elif ((defined(__ARM_ARCH_7M__)        && (__ARM_ARCH_7M__        != 0)) || \
       (defined(__ARM_ARCH_7EM__)       && (__ARM_ARCH_7EM__       != 0)))
  SCB->SHPR[11]  = SYSTICK_IRQ_PRIORITY;
#elif  (defined(__ARM_ARCH_6M__)        && (__ARM_ARCH_6M__        != 0))
  SCB->SHPR[1]  |= ((uint32_t)SYSTICK_IRQ_PRIORITY << 24);
#else
#error "Unknown ARM Core!"
#endif

  SysTick->CTRL =  SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
  SysTick->LOAD =  load;
  SysTick->VAL  =  0U;

  PendST = 0U;

  return (0);
}

/// Enable OS Tick.
__WEAK void OS_Tick_Enable (void) {

  if (PendST != 0U) {
    PendST = 0U;
    SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
  }

  SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;
}

/// Disable OS Tick.
__WEAK void OS_Tick_Disable (void) {

  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

  if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0U) {
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
    PendST = 1U;
  }
}

// Acknowledge OS Tick IRQ.
__WEAK void OS_Tick_AcknowledgeIRQ (void) {
  (void)SysTick->CTRL;
}

// Get OS Tick IRQ number.
__WEAK int32_t  OS_Tick_GetIRQn (void) {
  return ((int32_t)SysTick_IRQn);
}

// Get OS Tick clock.
__WEAK uint32_t OS_Tick_GetClock (void) {
  return (SystemCoreClock);
}

// Get OS Tick interval.
__WEAK uint32_t OS_Tick_GetInterval (void) {
  return (SysTick->LOAD + 1U);
}

// Get OS Tick count value.
__WEAK uint32_t OS_Tick_GetCount (void) {
  uint32_t val;
  uint32_t count;

  val = SysTick->VAL;
  if (val != 0U) {
    count = (SysTick->LOAD - val) + 1U;
  } else {
    count = 0U;
  }

  return (count);
}

// Get OS Tick overflow status.
__WEAK uint32_t OS_Tick_GetOverflow (void) {
  return ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) >> SCB_ICSR_PENDSTSET_Pos);
}

#endif
