#include "stm32f030x6.h"
#include "user_i2c.h"
#include "boot.h"


#define I2C_BASE_ADDR           0x2d

#define I2C_TIMEOUT             2

#define PIN_SCL                 9
#define PIN_SDA                 10
#define PIN_STBY                2

#define MODE_IN                 0x00
#define MODE_OUT                0x01
#define MODE_AF                 0x02
#define MODE_AN                 0x03
#define MODER(mode, pin)        ((mode) << (2 * (pin)))

#define MAX_PKT_LEN             32

#define ERR_TIMEOUT             (-1)
#define ERR_ABORTED             (-2)
#define ERR_NOTIMPL             (-3)


static uint8_t buf[MAX_PKT_LEN];
volatile uint32_t timeout = 0;


void SysTick_Handler(void)
{
    if (timeout)
        timeout--;
}

static int receive_cmd(uint8_t *buf, uint8_t size)
{
    uint8_t *pbuf = buf;
    uint8_t len = 0;

    /* Reenable interface to reset it`s state */
    I2C1->CR1 = 0;
    while (I2C1->CR1 & I2C_CR1_PE);
    I2C1->CR1 = I2C_CR1_PE;
    while ((I2C1->CR1 & I2C_CR1_PE) == 0);

    /* Clear all interrupt flags */
    I2C1->ICR = 0xffffffff;

    /* Wait own addr match */
    while ((I2C1->ISR & I2C_ISR_ADDR) == 0);
    I2C1->ICR = I2C_ICR_ADDRCF;

    /* Send responce for a general read command */
    if (I2C1->ISR & I2C_ISR_DIR) {
        /* Just reply own addr back */
        I2C1->TXDR = (I2C1->OAR1 & 0xFF) >> 1;
        while(!(I2C1->ISR & I2C_ISR_TXE));

        timeout = I2C_TIMEOUT;
        while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
        if (!timeout)
            return ERR_TIMEOUT;

        return 0;
    }

    /* Receive 1st byte of data */
    timeout = 2 * I2C_TIMEOUT;
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && 
        (I2C1->ISR & I2C_ISR_STOPF) == 0 && 
        timeout);
    if (!timeout)
        return ERR_TIMEOUT;
    if (I2C1->ISR & I2C_ISR_STOPF)
        return ERR_ABORTED;
    *buf++ = I2C1->RXDR;

    /* Wait the rest of data bytes or 2nd start bit */
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && (I2C1->ISR & I2C_ISR_ADDR) == 0);

    /* 2nd start bit received */
    if (I2C1->ISR & I2C_ISR_ADDR) {
        I2C1->ICR = I2C_ICR_ADDRCF;

        /* Read direction */
        if (I2C1->ISR & I2C_ISR_DIR) {
            uint8_t len = handle_cmd(CMD_READ, pbuf, size);
            /* Send a reply */
            for (uint8_t i = 0; i < len; i++) {
                I2C1->TXDR = pbuf[i];
                timeout = I2C_TIMEOUT;
                while(!(I2C1->ISR & I2C_ISR_TXE) && !(I2C1->ISR & I2C_ISR_NACKF) && (timeout));
                /* Check if master has interrupted the transmission */
                if ((I2C1->ISR & I2C_ISR_NACKF)) {
                    break;
                }
                if (!timeout)
                    return ERR_TIMEOUT;
            }

            timeout = I2C_TIMEOUT;
            while ((I2C1->ISR & I2C_ISR_BUSY) && (I2C1->ISR & I2C_ISR_STOPF) == 0 &&
                !(I2C1->ISR & I2C_ISR_NACKF) && (timeout));
            if (!timeout)
                return ERR_TIMEOUT;

            return 0;
        } else {
            /* 2nd start for write direction is not supported */
            return ERR_NOTIMPL;
        }
    }

    /* Receive the rest of data bytes */
    timeout = I2C_TIMEOUT;
    for (len = 1; len < MAX_PKT_LEN; len++) {
        while ((I2C1->ISR & I2C_ISR_RXNE) == 0 && 
            (I2C1->ISR & I2C_ISR_STOPF) == 0 && 
            timeout);
        if (!timeout)
            return ERR_TIMEOUT;
        if (I2C1->ISR & I2C_ISR_STOPF)
            return len;
        *buf++ = I2C1->RXDR;
    }

    while (((I2C1->ISR & I2C_ISR_STOPF) == 0) && (timeout));
    if (!timeout)
        return ERR_TIMEOUT;

    I2C1->ICR = I2C_ICR_STOPCF;

    return len;
}

static void init_i2c(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_TIM3EN;

    GPIOA->MODER |= MODER(MODE_AF, PIN_SCL) | MODER(MODE_AF, PIN_SDA) |
        MODER(MODE_OUT, PIN_STBY);

    GPIOA->AFR[0] |= (1 << GPIO_AFRH_AFRH6_Pos) | (1 << GPIO_AFRH_AFRH7_Pos);
    GPIOA->AFR[1] |= (4 << GPIO_AFRH_AFRH1_Pos) | (4 << GPIO_AFRH_AFRH2_Pos);

    GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0;

    RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
    GPIOF->MODER  |= MODER(MODE_IN, 0) | MODER(MODE_IN, 1);
    GPIOF->PUPDR  |= GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR1_0;
    I2C1->OAR1 = I2C_OAR1_OA1EN | ((I2C_BASE_ADDR + (GPIOF->IDR & 3)) << 1);
    
    I2C1->CR1 = I2C_CR1_PE;
}

int main()
{
    /* Check boot flag and flash first bytes */
    if (boot_flag != 0xAABBCCDD &&
        (((*(__IO uint32_t*)MAIN_PROGRAM_START_ADDRESS) & 0x2FFE0000 ) == 0x20000000)) {
        JUMP_TO(MAIN_PROGRAM_START_ADDRESS);
    }

    init_i2c();

    SysTick_Config(8000);

    while (1) {
        int8_t len = receive_cmd(buf, sizeof(buf));
        if (len > 0) {
            handle_cmd(CMD_WRITE, buf, len);
        }
    }

    return 0;
}