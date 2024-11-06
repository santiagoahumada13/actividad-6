/*Librerias*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"
/*Caracteristicas de la pantalla*/
#define screen_h 64
#define screen_w 128
#define i2c_address _u(0x3C)
#define screen_clk 400
/*Comandos del controlador SSD1306*/
#define set_mem_mode _u(0x20)
#define set_col_addr _u(0x21)
#define set_page_addr _u(0x22)
#define set_scroll _u(0x2E)
#define set_seg_remap _u(0xA0)
#define set_mux_ratio _u(0xA8)
#define set_allOn _u(0xA5)
#define set_entire_on _u(0xA4)
#define set_charge_pump _u(0x8D)
#define set_disp _u(0xAE)
#define set_disp_start_line _u(0x40)
#define set_disp_offset _u(0xD3)
#define set_contrast _u(0x81)
#define set_com_out_dir _u(0xC0)
#define set_com_pin_cfg _u(0xDA)
#define set_disp_clk_div _u(0xD5)
#define set_precharge _u(0xD9)
#define set_norm_disp _u(0xA6)
#define set_vcom_desel _u(0xDB)

#define page_h _u(8)
#define num_pages (screen_h/page_h)
#define buf_len (num_pages * screen_w)

#define write_mode _u(0xFE)
#define read_mode -u(0xFF)

struct render_area{
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

void calc_render_area_buflen(struct render_area *area){
    //Calcula que tan largo sera el bufer para el area de dibujo
    area->buflen=(area->buflen - area->start_col+1) * (area->end_page-area->start_page+1);
}

#ifdef i2c_default

void send_cmd(uint8_t cmd){
    /*El proceso I2C espera un byte de control seguido de los datos
    estos datos pueden ser datos como tal o comandos
    Co=1,D/C=0=> el driver espera un comando
    */
    uint8_t buf[2]= {0x80,cmd};
    i2c_write_blocking(i2c_default, i2c_address,buf,2,false);
}

void send_cmd_list(uint8_t *buf, int num){
    for (int i=0;i<num;i++)
        send_cmd(buf[i]);
}

void send_buf(uint8_t buf[], int buflen){
    /* en modo horizontal de direccionamiento, la direccion del apuntador de columna se auto incrementa
    y se almacena para la siguiente pagina, asi podemos enviar completo el bufer de frame en un solo comando
    */
   uint8_t *temp_buf =malloc(buflen+1);
   temp_buf[0]=0x40;
   memcpy(temp_buf+1,buf,buflen);

   i2c_write_blocking(i2c_default,i2c_address,temp_buf,buflen+1,false);
   free(temp_buf);
}

void screen_init(){

    uint8_t cmds[]={
        set_disp, //apaga el display
        /*mapeo de memoria*/
        set_mem_mode,0x00, /*modo horizontal=0,vertical=1,pagina=2*/
        /*resolucion y layout*/
        set_disp_start_line,
        set_seg_remap | 0x01,
        set_mux_ratio,
        screen_h - 1,
        set_com_out_dir | 0x08,
        set_disp_offset,0x00,
        set_com_pin_cfg,
    #if((screen_w==128)&&(screen_h==32))
        0x02,
    #elif ((screen_w==128)&&(screen_h==64))
        0x12,
    #else  
        0x02,
    #endif
        /*sincronizacion y esquema de direcciones*/
        set_disp_clk_div, 0x80,
        set_precharge, 0xF1,
        set_vcom_desel, 0x30,
        /*display*/
        set_contrast,0xFF,
        set_entire_on,
        set_norm_disp,
        set_charge_pump,0x14,
        set_scroll,0x00,
        set_disp | 0x01,
    };

    send_cmd_list(cmds,count_of(cmds));
}
void render(uint8_t *buf,struct render_area *area){
    uint8_t cmds[]={
            set_col_addr,
            area->start_col,
            area->end_col,
            set_page_addr,
            area->start_page,
            area->end_page,
    };
    send_cmd_list(cmds,count_of(cmds));
    send_buf(buf,area->buflen);
}
static inline int getFont(uint8_t ch){
    if (ch >= 'A' && ch <='Z') {
        return  ch - 'A' + 1;
    }
    else if (ch >= '0' && ch <='9') {
        return  ch - '0' + 27;
    }
    else return  0;
}
static void writeChar(uint8_t *buf,int16_t x, int16_t y,uint8_t ch){
    if(x>screen_w-8||y>screen_h-8)
        return;
    y=y/8;

    ch=toupper(ch);
    int idx=getFont(ch);
    int fb_idx=y*128+x;

    for (int i=0;i<8;i++){
        buf[fb_idx++]=font[idx*8+i];
    }
}

static void writeString (uint8_t *buf,int16_t x,int16_t y,char *str){
    if(x>screen_w-8||y>screen_h-8)
        return;
    while(*str){
        writeChar(buf,x,y,*str++);
        x+=8;
    }
}
#endif

int main(){
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c / SSD1306_i2d example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("SSD1306 OLED driver I2C example for the Raspberry Pi Pico"));

    i2c_init(i2c_default, screen_clk * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    screen_init();

    struct render_area frame_area = {
        start_col: 0,
        end_col : screen_w - 1,
        start_page : 0,
        end_page : num_pages - 1
        };

    calc_render_area_buflen(&frame_area);

    uint8_t buf[buf_len];
    memset(buf, 0, buf_len);
    render(buf, &frame_area);

    char *text[]={
        "Santiago Ahumada",
        "en OLED!"
    };
    int y=0;
    for(uint i=0;i<count_of(text);i++){
        writeString(buf,5,y,text[i]);
        y+=8;
    }
    render(buf,&frame_area);
#endif  
    return 0;
}