/*
Warning: 
This code is held together with nothing but off-brand clear tape and my desire to finish this stupid joke.
If you have a stroke from reading the code, its your own fault.

Licensed under the: do-whatever-you-want-I-dont-care licence
2021 Thicc Industries.
*/

#include "draw.h"
#include "main.h"

//Yeah this uses a stupid amount of globals for application state, deal with it.

int frame; //Technically a frame counter, but gets reset a lot

int cur_x = 0;  //Cursor position on the bitmap
int cur_y = 0;  //-----------------------------

int selectedIndex = 0;      //Selected color in the palette. [0, 15]
int selectedChannel = 0;    //Selected channel for colors.   [0, 3]

int cur_dx; //How many spaces to move the cursor on the next MOVE_RATE 
int cur_dy; //--------------------------------------------------------

int chan_dv; //How many spaces to move the channel on the next MOVE_RATE

int bitmap[32][32]; //Bitmap grid of color incidies
COLOR palette[16];  //The color palette

bool blink;     //Should blinking objects be displayed
bool colormode; //Editing colors or bitmap (true : false)

int main()
{
    init_vblank_irq();      //Init VBlank Interupt
    init_color_palette();   //Populate palette array with default values

    //Set display mode (Mode 3 Bitmap, BG layer 2)
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

    //Draw UI borders
    m3_fill(RGB15(2, 2, 2));
    m3_rect(0, 0, 132, 144, RGB15(7, 7, 7));
    m3_rect(0, 0, 128, 140, RGB15(2, 2, 2));

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

    //Draw palette selection
    for (int i = 0; i < 16; i++)
    {
        draw_pal_pxl(i, palette);
        draw_pal_select(i, RGB15(7, 7, 7));
    }

    //Draw color editor
    draw_grad(0);   //Red gradient
    draw_grad(1);   //Green gradient
    draw_grad(2);   //Blue gradient
    draw_grad_value(0, palette, selectedIndex); //Red selection
    draw_grad_value(1, palette, selectedIndex); //Green selection
    draw_grad_value(2, palette, selectedIndex); //Blue selection
    draw_grad_sel(0);

    draw_pal_select(selectedIndex, RGB15(31, 31, 31)); //Draw palette indicator

    //Loop every clock cycle
    while (1)
    {
        key_poll(); //Poll inputs

        if (key_hit(KEY_L)) //Change color left
        {
            draw_pal_select(selectedIndex, RGB15(7, 7, 7));
            selectedIndex = clampi(selectedIndex - 1, 0, 15);
            draw_grad_value(0, palette, selectedIndex);
            draw_grad_value(1, palette, selectedIndex);
            draw_grad_value(2, palette, selectedIndex);
            draw_pal_select(selectedIndex, RGB15(31, 31, 31));
        }

        if (key_hit(KEY_R)) //Change color right
        {
            draw_pal_select(selectedIndex, RGB15(7, 7, 7));
            selectedIndex = clampi(selectedIndex + 1, 0, 15);
            draw_grad_value(0, palette, selectedIndex);
            draw_grad_value(1, palette, selectedIndex);
            draw_grad_value(2, palette, selectedIndex);
            draw_pal_select(selectedIndex, RGB15(31, 31, 31));
        }

        if (colormode)
        {
            input_color();  //Run application cycle for color mode
        }
        else
        {
            input_bitmap(); //Run application cycle for bitmap mode
        }

        if (key_hit(KEY_START))
            save(palette, bitmap);

        if (key_hit(KEY_SELECT))
        {
            APFILE *loadedFile = load();

            //Load palette
            for (int i = 0; i < 16; i++)
            {
                palette[i] = loadedFile -> palette[i];
                draw_pal_pxl(i, palette);
            }

            //Load bitmap
            for (int i = 0; i < 32 * 32; i++)
            {
                bitmap[i % 32][i / 32] = loadedFile -> bitmap[i % 32][i / 32];
                draw_pxl(i % 32, i / 32, bitmap, palette);
            }

            //refresh color preview
            draw_grad_value(0, loadedFile->palette, selectedIndex);
            draw_grad_value(1, loadedFile->palette, selectedIndex);
            draw_grad_value(2, loadedFile->palette, selectedIndex);

            //Free up memory
            free(loadedFile);
        }
    }

    return 0;
}

void save(COLOR palette[16], int bitmap[32][32])
{
    //Save memory location
    u8 *pSaveMemory = ((u8 *)0x0E000000);

    //Write austin paint header
    char header[16] =  {0x41, 0x55, 0x53, 0x54, 0x49, 0x4E, 0x50, 0x41,
                        0x49, 0x4E, 0x54, 0x00, 0x56, 0x32, 0x2E, 0x30};
    for (int i = 0; i < 16; i++)
    {
        pSaveMemory[i] = header[i];
    }
    //save palette
    for (int i = 0; i < 16 * 3; i += 3)
    {
        //Make compadible with AP2 color space
        int blu = expand_range((palette[i / 3] & 0x7C00) >> 10);
        int grn = expand_range((palette[i / 3] & 0x03E0) >> 5);
        int red = expand_range((palette[i / 3] & 0x001F));

        
        pSaveMemory[i + 16 + 0] = red;
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

APFILE* load(void)
{
    //Save memory location
    u8 *pSaveMemory = ((u8 *)0x0E000000);
    APFILE *loadedFile = malloc( sizeof(APFILE) );

    for (int i = 0; i < 16 * 3; i += 3)
    {
        loadedFile -> palette[i / 3] = RGB15(0, 0, 0);

        int red = constrict_range(pSaveMemory[i + 16]);
        int grn = constrict_range(pSaveMemory[i + 16 + 1]);
        int blu = constrict_range(pSaveMemory[i + 16 + 2]);

        loadedFile -> palette[i / 3] = RGB15(red, grn, blu);
    }

    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            int paletteIndex = (y * 16) + x + (4 * 16);

            int topPixel = (pSaveMemory[paletteIndex] & 0xF0) >> 4;
            int botPixel = (pSaveMemory[paletteIndex] & 0x0F);

            loadedFile -> bitmap[x * 2][y] = topPixel;
            loadedFile -> bitmap[(x * 2) + 1][y] = botPixel;
        }
    }

    return loadedFile;
}

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

void init_vblank_irq(void)
{
    REG_IME = 0x00;
    REG_ISR_MAIN = on_irq;
    REG_DISPSTAT |= DSTAT_VBL_IRQ;
    REG_IE = IRQ_VBLANK;
    REG_IME = 0x01;
}

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

    if(key_hit(KEY_LEFT)){
        modify_color_channel(-1, selectedChannel, selectedIndex);
        frame = 1; //Stupid hack to prevent double inputs
    }

    if(key_hit(KEY_RIGHT)){
        modify_color_channel(1, selectedChannel, selectedIndex);
        frame = 1; //Stupid hack to prevent double inputs
    }


    if(key_is_down(KEY_LEFT)){
        chan_dv = -1;
    }

    if(key_is_down(KEY_RIGHT)){
        chan_dv = 1;
    }

    //Its really stupid that this needs to be here, and i have literally no idea why it crashes when put where it SHOULD be
    draw_grad_value(selectedChannel, palette, selectedIndex);
    draw_pal_pxl(selectedIndex, palette);

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

void input_bitmap(void)
{
    if (key_is_down(KEY_A))
    {
        bitmap[cur_x][cur_y] = selectedIndex;
        draw_pxl(cur_x, cur_y, bitmap, palette);
    }

    //Single hit
    if (key_hit(KEY_UP))
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
        cur_y = clampi(cur_y - 1, 0, 31);
        blink = true;
        frame = 1;
    }

    if (key_hit(KEY_DOWN))
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
        cur_y = clampi(cur_y + 1, 0, 31);
        blink = true;
        frame = 1;
    }

    if (key_hit(KEY_RIGHT))
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
        cur_x = clampi(cur_x + 1, 0, 31);
        blink = true;
        frame = 1;
    }

    if (key_hit(KEY_LEFT))
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
        cur_x = clampi(cur_x - 1, 0, 31);
        blink = true;
        frame = 1;
    }

    //Repeat movements
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

void input_color_vblank(void)
{

    if (frame % MOVE_RATE == 0)
    {
        modify_color_channel(chan_dv, selectedChannel, selectedIndex);
    }

    chan_dv = 0;
}

void input_bitmap_vblank(void)
{

    if (frame % MOVE_RATE == 0)
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
        cur_x = clampi(cur_x + cur_dx, 0, 31);
        cur_y = clampi(cur_y + cur_dy, 0, 31);
        blink = true;
    }
    
    cur_dx = 0;
    cur_dy = 0;

    //draw the pixel under the cursor on the falling frame of blink
    if ((frame - 1) % BLINK_RATE == 0 && !blink)
    {
        draw_pxl(cur_x, cur_y, bitmap, palette);
    }
    //Draw the cursor if blinking
    if (blink)
    {
        //Warning: this is dumb

        for(int x = -1; x < 1; x++){
            for(int y = -1; y < 1; y++){
                draw_pxl( clampi(cur_x + x, 0, 31),
                          clampi(cur_y + y, 0, 31), 
                          bitmap, palette);
            }
        }
        draw_cursor(cur_x, cur_y, RGB15(15, 15, 15));
    }

    //toggle blink every BLINK_RATE frames
    if (frame % BLINK_RATE == 0)
        blink = !blink;
}

void modify_color_channel(int dv, int channel, int index){
    int blu = (palette[index] & 0x7C00) >> 10;
    int grn = (palette[index] & 0x03E0) >> 5;
    int red = (palette[index] & 0x001F);

    switch (channel)
    {
    case 0:
        red = clampi(red + dv, 0, 31);
        break;
    case 1:
        grn = clampi(grn + dv, 0, 31);
        break;
    case 2:
        blu = clampi(blu + dv, 0, 31);
        break;
    }

    palette[index] = RGB15(red, grn, blu);
}

int clampi(int value, int min, int max)
{
    if (value > max)
        return max;

    if (value < min)
        return min;

    return value;
}

int expand_range(int bitvalue)
{
    return bitvalue == 0 ? 0 : ((bitvalue + 1) * 8) - 1;
}

int constrict_range(int bitvalue)
{
    return bitvalue == 0 ? 0 : ((bitvalue + 1) / 8) - 1;
}