#include <nds/bios.h>
#include <nds/card.h>
#include <nds/memory.h>

#include "read_card_init_ntr.arm9.h"

static void cardWriteCommand9(const u8 *command)
{
    REG_AUXSPICNTH = CARD_CR1_ENABLE | CARD_CR1_IRQ;

    for (int index = 0; index < 8; index++)
        REG_CARD_COMMAND[7 - index] = command[index];
}

static void cardReset9(void)
{
    const u8 cmdData[8] = { 0, 0, 0, 0, 0, 0, 0, CARD_CMD_DUMMY };

    cardWriteCommand9(cmdData);
    REG_ROMCTRL = CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(5)
                  | CARD_DELAY2(0x18);
    u32 read = 0;

    do
    {
        if (REG_ROMCTRL & CARD_DATA_READY)
        {
            if (read < 0x2000)
            {
                u32 data = REG_CARD_DATA_RD;
                (void)data;
                read += 4;
            }
        }
    } while (REG_ROMCTRL & CARD_BUSY);
}

void executeCardReset9()
{
    REG_ROMCTRL = 0;
    REG_AUXSPICNTH = 0;

    swiDelay(167550);

    REG_AUXSPICNTH = CARD_CR1_ENABLE | CARD_CR1_IRQ;
    REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;

    while (REG_ROMCTRL & CARD_BUSY);

    cardReset9();

    while (REG_ROMCTRL & CARD_BUSY);
}
