#include <tonc.h>
#include <stdio.h>
#include <stdlib.h>

#define BLINK_RATE 30   //How many frames should the cursor blink
#define MOVE_RATE 10    //How many frames per movement on held movements, applies to both bitmap field and color editor

/*Helper functions*/
int clampi(int value, int min, int max);                    //Clamp integer value to specified min and max
int expand_range(int bitvalue);                             //Expand a [0, 31] value to a [0, 255] value
int constrict_range(int bitvalue);                          //Compress a [0, 255] value to a [0, 31] value
void modify_color_channel(int dv, int channel, int index);  //Modify the value of a color channel
void init_color_palette(void);                              //Creates default values inside Palette

/*Interupt functions*/
void on_irq(void);              //VBlank interupt callback
void init_vblank_irq(void);     //Setup VBlank interupt

/*Input functions*/
void input_bitmap(void);        //Input bitmap (Every cycle)
void input_color(void);         //Input color field (Every cycle)
void input_bitmap_vblank(void); //Input bitmap (Every VBlank)
void input_color_vblank(void);  //Input color field (Every VBlank)

/*Memory functions*/
typedef struct {
    int bitmap[32][32];
    COLOR palette[16];
} APFILE;

void save(COLOR palette[16], int bitmap[32][32]);   //Save AP2 image
APFILE* load(void);                                 //Load AP2 image