#include "tonc.h"

#define PXL_SCALE 4                     //The scale of each bitmap "pixel" in real pixels
#define BMP_OFFSET 0                    //Horizontal offset of the BMP field
#define PAL_OFFSET 32 * PXL_SCALE       //Vertical offset of the palette field
#define COL_OFFSET (3 * PXL_SCALE) - 4  //Horizontal offset of the color gradient field
#define COL_OFFSET_X 10                 //Ok im not sure what either of these do now

/*Drawing functions*/
void draw_pxl(int pxlx, int pxly, int bitmap[32][32], COLOR palette[16]);   //Draw a pixel on the bitmap field
void draw_pal_pxl(int index, COLOR palette[16]);                            //Draw a palette selection pixel
void draw_pal_select(int index, COLOR col);                                 //Draw the current color selector
void draw_cursor(int curx, int cury, COLOR col);                            //Draw cursor
void draw_grad(int channel);                                                //Draw gradient of specified channel
void draw_grad_value(int channel, COLOR palette[16], int index);            //Draw gradient value display
void draw_grad_sel(int channel);                                            //Draw color channel display