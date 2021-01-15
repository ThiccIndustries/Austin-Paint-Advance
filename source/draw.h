#include <tonc.h>
#define PXL_SCALE 4
#define BMP_OFFSET 0
#define PAL_OFFSET 32 * PXL_SCALE
#define COL_OFFSET (3 * PXL_SCALE) - 4
#define COL_OFFSET_X 10


void draw_pxl(int pxlx, int pxly, int bitmap[32][32], COLOR palette[16]);
void draw_pal_pxl(int index, COLOR palette[16]);
void draw_pal_select(int index, COLOR col);
void draw_cursor(int curx, int cury, COLOR col);
void draw_grad(int channel);


void draw_pxl(int pxlx, int pxly, int bitmap[32][32], COLOR palette[16])
{

    COLOR pxl_clr = palette[bitmap[pxlx][pxly]];
    for (int y = 0; y < PXL_SCALE; y++)
    {
        for (int x = 0; x < PXL_SCALE; x++)
        {
            int xcoord = (pxlx * PXL_SCALE) + x + BMP_OFFSET;
            int ycoord = (pxly * PXL_SCALE) + y;

            m3_plot(xcoord, ycoord, pxl_clr);
        }
    }
}

void draw_pal_pxl(int index, COLOR palette[16])
{
    COLOR pxl_clr = palette[index];
    for (int y = 0; y < PXL_SCALE; y++)
    {
        for (int x = 0; x < PXL_SCALE * 2; x++)
        {
            int xcoord = (index * (PXL_SCALE * 2)) + x + BMP_OFFSET;
            int ycoord = PAL_OFFSET + y;

            m3_plot(xcoord, ycoord, pxl_clr);
        }
    }
}

void draw_cursor(int curx, int cury, COLOR col)
{
    for (int y = 0; y < PXL_SCALE; y++)
    {
        for (int x = 0; x < PXL_SCALE; x++)
        {
            int xcoord = (curx * PXL_SCALE) + x + BMP_OFFSET;
            int ycoord = (cury * PXL_SCALE) + y;

            m3_plot(xcoord, ycoord, col);
        }
    }
}

void draw_pal_select(int index, COLOR col)
{
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 2; x++)
        {
            int xcoord = (index * (PXL_SCALE * 2)) + x + BMP_OFFSET + 3;
            int ycoord = PAL_OFFSET + (PXL_SCALE * 1.5) + y;

            m3_plot(xcoord, ycoord, col);
        }
    }
}

void draw_grad(int channel)
{
    for (int x = 0; x < 32 * 3; x++)
    {
        for (int y = 0; y < PXL_SCALE * 3; y++)
        {
            int xcoord = BMP_OFFSET + (32 * PXL_SCALE) + x + COL_OFFSET_X;
            int ycoord = y + (channel * PXL_SCALE * 6) + COL_OFFSET;

            switch (channel)
            {
            case 0:
                m3_plot(xcoord, ycoord, RGB15(x / 3, 0, 0));
                break;
            case 1:
                m3_plot(xcoord, ycoord, RGB15(0, x / 3, 0));
                break;
            case 2:
                m3_plot(xcoord, ycoord, RGB15(0, 0, x / 3));
                break;
            }
        }
    }
}

void draw_grad_value(int channel, COLOR palette[16], int index)
{

    //Fill BG
    int blu = (palette[index] & 0x7C00) >> 10;
    int grn = (palette[index] & 0x03E0) >> 5;
    int red = (palette[index] & 0x001F);

    int x1 = BMP_OFFSET + (32 * PXL_SCALE) + COL_OFFSET_X;
    int x2 = x1 + 32 * 3;

    int y1 = (channel * PXL_SCALE * 6) + (3 * PXL_SCALE) + COL_OFFSET;
    int y2 = y1 + 3;

    m3_rect(x1, y1, x2, y2, RGB15(4, 4, 4));

    //this is gross
    x1 = BMP_OFFSET + (32 * PXL_SCALE) + COL_OFFSET_X;

    switch (channel)
    {
    case 0:
        x1 += (red * 3);
        break;
    case 1:
        x1 += (grn * 3);
        break;
    case 2:
        x1 += (blu * 3);
        break;
    }

    x2 = x1 + 3;

    m3_rect(x1, y1, x2, y2, RGB15(31, 31, 31));
}

void draw_grad_sel(int channel)
{
    int x1 = BMP_OFFSET + (32 * PXL_SCALE) + 7;
    int x2 = x1 + 3;

    int y1 = COL_OFFSET;
    int y2 = (18 * PXL_SCALE) + COL_OFFSET - 6;

    m3_rect(x1, y1, x2, y2, RGB15(2, 2, 2));

    y1 = (channel * PXL_SCALE * 6) + COL_OFFSET;
    y2 = y1 + (3 * PXL_SCALE);

    m3_rect(x1, y1, x2, y2, RGB15(31, 31, 31));
}