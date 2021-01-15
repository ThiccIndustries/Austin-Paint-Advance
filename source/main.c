/*Massive spaghetti dont read*/

#include <tonc.h>
#include <stdio.h>
#include <stdlib.h>
#include "draw.h"

#define BLINK_RATE 30
#define MOVE_RATE 10

int clampi(int value, int min, int max);
int expandRange(int bitvalue);
int constrictRange(int bitvalue);

void on_irq(void);
void init_color_palette(void);
void init_vblank_irq(void);

void input_bitmap(void);
void input_color(void);
void input_bitmap_vblank(void);
void input_color_vblank(void);

void save(COLOR palette[16], int bitmap[32][32]);
void load(void);

int frame;

int cur_x = 0;
int cur_y = 0;
int selectedIndex = 0;
int selectedChannel = 0;

int cur_dx, cur_dy;
int bitmap[32][32];
COLOR palette[16];

bool blink;
bool colormode;

#define GAMEPAK_RAM ((u8 *)0x0E000000)

int main()
{
    init_vblank_irq();
    init_color_palette();

    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    //Draw borders
    m3_fill(RGB15(2, 2, 2));
    m3_rect(0, 0, 132, 144, RGB15(7, 7, 7)); //Bitmap border
    m3_rect(0, 0, 128, 140, RGB15(2, 2, 2)); //Bitmap border

    m3_rect(128, 0, 240, 80, RGB15(7, 7, 7));
    m3_rect(132, 0, 240, 76, RGB15(2, 2, 2));

    //Fill and display bitmap
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 32; x++)
        {
            bitmap[x][y] = 0;
            draw_pxl(x, y, bitmap, palette);
        }
    }

    //Draw palette
    for (int i = 0; i < 16; i++)
    {
        draw_pal_pxl(i, palette);
        draw_pal_select(i, RGB15(7, 7, 7));
    }

    //Draw palette edit
    draw_grad(0);
    draw_grad(1);
    draw_grad(2);
    draw_grad_value(0, palette, selectedIndex);
    draw_grad_value(1, palette, selectedIndex);
    draw_grad_value(2, palette, selectedIndex);
    draw_grad_sel(0);

    //This loop happens every frame
    while (1)
    {
        key_poll();

        if (key_hit(KEY_L))
        {
            draw_pal_select(selectedIndex, RGB15(7, 7, 7));
            selectedIndex = clampi(selectedIndex - 1, 0, 15);
            draw_grad_value(0, palette, selectedIndex);
            draw_grad_value(1, palette, selectedIndex);
            draw_grad_value(2, palette, selectedIndex);
        }

        if (key_hit(KEY_R))
        {
            draw_pal_select(selectedIndex, RGB15(7, 7, 7));
            selectedIndex = clampi(selectedIndex + 1, 0, 15);
            draw_grad_value(0, palette, selectedIndex);
            draw_grad_value(1, palette, selectedIndex);
            draw_grad_value(2, palette, selectedIndex);
        }

        draw_pal_select(selectedIndex, RGB15(31, 31, 31));

        if (colormode)
        {
            input_color();
        }
        else
        {
            input_bitmap();
        }

        if (key_hit(KEY_START))
            save(palette, bitmap);

        if (key_hit(KEY_SELECT))
            load();
    }

    return 0;
}

//no matter how much i use C, just up and writing to memory never sits right
void save(COLOR palette[16], int bitmap[32][32])
{
    u8 *pSaveMemory = GAMEPAK_RAM;

    //Write austin paint header
    char header[16] = {0x41, 0x55, 0x53, 0x54, 0x49, 0x4E, 0x50, 0x41, 0x49, 0x4E, 0x54, 0x00, 0x56, 0x32, 0x2E, 0x30};
    for (int i = 0; i < 16; i++)
    {
        pSaveMemory[i] = header[i];
    }
    //save palette
    for (int i = 0; i < 16 * 3; i += 3)
    {
        int blu = expandRange((palette[i / 3] & 0x7C00) >> 10);
        int grn = expandRange((palette[i / 3] & 0x03E0) >> 5);
        int red = expandRange((palette[i / 3] & 0x001F));

        //Make compadible with AP2 color space
        pSaveMemory[i + 16] = red;
        pSaveMemory[i + 16 + 1] = grn;
        pSaveMemory[i + 16 + 2] = blu;
    }

    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            int index = (y * 16) + x + (16 * 4);

            char topPixel = bitmap[x * 2][y] << 4;
            char bottomPixel = bitmap[(x * 2) + 1][y];
            pSaveMemory[index] = topPixel + bottomPixel;
        }
    }
}

void load()
{
    u8 *pSaveMemory = GAMEPAK_RAM;

    for (int i = 0; i < 16 * 3; i += 3)
    {
        palette[i / 3] = RGB15(0, 0, 0);

        int red = constrictRange(pSaveMemory[i + 16]);
        int grn = constrictRange(pSaveMemory[i + 16 + 1]);
        int blu = constrictRange(pSaveMemory[i + 16 + 2]);

        palette[i / 3] = RGB15(red, grn, blu);
        draw_pal_pxl(i / 3, palette);
        draw_grad_value(0, palette, i / 3);
        draw_grad_value(1, palette, i / 3);
        draw_grad_value(2, palette, i / 3);
    }

    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            int paletteIndex = (y * 16) + x + (4 * 16);

            int topPixel = (pSaveMemory[paletteIndex] & 0xF0) >> 4;
            int botPixel = (pSaveMemory[paletteIndex] & 0x0F);

            bitmap[x * 2][y] = topPixel;
            bitmap[(x * 2) + 1][y] = botPixel;

            draw_pxl(x * 2, y, bitmap, palette);
            draw_pxl((x * 2) + 1, y, bitmap, palette);
        }
    }
}

//IRQ Handler, the only enabled interupt is vblank so this is fine
void on_irq(void)
{
    REG_IF = IRQ_VBLANK;
    REG_IFBIOS |= IRQ_VBLANK;

    if (colormode)
    {
        input_color_vblank();
    }
    else
    {
        input_bitmap_vblank();
    }
    frame++;
}

//Enable vblank interupt
void init_vblank_irq(void)
{
    REG_IME = 0x00;
    REG_ISR_MAIN = on_irq;
    REG_DISPSTAT |= DSTAT_VBL_IRQ;
    REG_IE = IRQ_VBLANK;
    REG_IME = 0x01;
}

//Fill the color palette with the default. this is dumb
void init_color_palette(void)
{
    palette[0] = RGB15(0, 0, 0);
    palette[1] = RGB15(7, 7, 7);
    palette[2] = RGB15(15, 15, 15);
    palette[3] = RGB15(31, 31, 31);
    palette[4] = RGB15(0, 0, 31);
    palette[5] = RGB15(0, 0, 15);
    palette[6] = RGB15(0, 31, 0);
    palette[7] = RGB15(0, 15, 0);
    palette[8] = RGB15(31, 0, 0);
    palette[9] = RGB15(15, 0, 0);
    palette[10] = RGB15(0, 31, 31);
    palette[11] = RGB15(0, 15, 15);
    palette[12] = RGB15(31, 31, 0);
    palette[13] = RGB15(15, 15, 0);
    palette[14] = RGB15(31, 0, 31);
    palette[15] = RGB15(15, 0, 15);
}

//Color screen input
void input_color(void)
{
    if (key_hit(KEY_DOWN))
    {
        selectedChannel = clampi(selectedChannel + 1, 0, 2);
        draw_grad_sel(selectedChannel);
    }
    if (key_hit(KEY_UP))
    {
        selectedChannel = clampi(selectedChannel - 1, 0, 2);
        draw_grad_sel(selectedChannel);
    }

    if (key_hit(KEY_LEFT))
    {
        int blu = (palette[selectedIndex] & 0x7C00) >> 10;
        int grn = (palette[selectedIndex] & 0x03E0) >> 5;
        int red = (palette[selectedIndex] & 0x001F);

        switch (selectedChannel)
        {
        case 0:
            red = clampi(red - 1, 0, 31);
            break;
        case 1:
            grn = clampi(grn - 1, 0, 31);
            break;
        case 2:
            blu = clampi(blu - 1, 0, 31);
            break;
        }

        palette[selectedIndex] = RGB15(red, grn, blu);
        draw_grad_value(selectedChannel, palette, selectedIndex);
        draw_pal_pxl(selectedIndex, palette);
    }

    if (key_hit(KEY_RIGHT))
    {
        int blu = (palette[selectedIndex] & 0x7C00) >> 10;
        int grn = (palette[selectedIndex] & 0x03E0) >> 5;
        int red = (palette[selectedIndex] & 0x001F);

        switch (selectedChannel)
        {
        case 0:
            red = clampi(red + 1, 0, 31);
            break;
        case 1:
            grn = clampi(grn + 1, 0, 31);
            break;
        case 2:
            blu = clampi(blu + 1, 0, 31);
            break;
        }

        palette[selectedIndex] = RGB15(red, grn, blu);
        draw_grad_value(selectedChannel, palette, selectedIndex);
        draw_pal_pxl(selectedIndex, palette);
    }

    if (key_hit(KEY_B))
    {
        for (int x = 0; x < 32; x++)
        {
            for (int y = 0; y < 32; y++)
            {
                draw_pxl(x, y, bitmap, palette);
            }
        }
        colormode = false;
    }
}

//Bitmap input
void input_bitmap(void)
{
    if (key_is_down(KEY_A))
    {
        bitmap[cur_x][cur_y] = selectedIndex;
        draw_pxl(cur_x, cur_y, bitmap, palette);
    }

    if (key_hit(KEY_UP) || key_hit(KEY_DOWN) || key_hit(KEY_RIGHT) || key_hit(KEY_LEFT))
    {
        frame = 1;
    }

    if (key_is_down(KEY_UP))
        cur_dy = -1;

    if (key_is_down(KEY_DOWN))
        cur_dy = 1;

    if (key_is_down(KEY_RIGHT))
        cur_dx = 1;

    if (key_is_down(KEY_LEFT))
        cur_dx = -1;

    if (key_hit(KEY_B))
    {
        colormode = true;
    }
}

//This runs on every new frame
void input_color_vblank(void)
{
}

//This runs on every new frame
void input_bitmap_vblank(void)
{

    if (frame % MOVE_RATE == 0)
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
        cur_x = clampi(cur_x + cur_dx, 0, 31);
        cur_y = clampi(cur_y + cur_dy, 0, 31);

        cur_dx = 0;
        cur_dy = 0;

        blink = true;
    }

    //draw the pixel under the cursor on the falling frame of blink
    if ((frame - 1) % BLINK_RATE == 0 && !blink)
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
    }
    //Draw the cursor if blinking
    if (blink)
    {
        draw_cursor(cur_x, cur_y, RGB15(15, 15, 15));
    }

    //toggle blink every BLINK_RATE frames
    if (frame % BLINK_RATE == 0)
        blink = !blink;
}

//Clamp a value to specified max and min
int clampi(int value, int min, int max)
{
    if (value > max)
        return max;

    if (value < min)
        return min;

    return value;
}

//Expand the [0, 31] color channel range to the [0, 255] range used by the Austin Paint 2 standard
int expandRange(int bitvalue)
{
    return bitvalue == 0 ? 0 : ((bitvalue + 1) * 8) - 1;
}

//Constrict the [0, 255] color channel range of Austin Paint 2 to the [0, 31] range used by Austin Paint Advance
int constrictRange(int bitvalue)
{
    return bitvalue == 0 ? 0 : ((bitvalue + 1) / 8) - 1;
}