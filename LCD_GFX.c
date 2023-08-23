/*
 * LCD_GFX.c
 *
 *  Author: Tarunyaa Sivakumar
 */ 

#include <stdlib.h>
#include <math.h>
#include "../lib/LCD_GFX.h"
#include "../lib/ST7735.h"
#include <string.h>

/******************************************************************************
* Local Functions
******************************************************************************/



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
    LCD_setAddr(x,y,x,y);
    SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor){
    uint16_t row = character - 0x20;		//Determine row of ASCII table starting at space
    int i, j;
    if ((LCD_WIDTH-x>7)&&(LCD_HEIGHT-y>7)){
        for(i=0;i<5;i++){
            uint8_t pixels = ASCII[row][i]; //Go through the list of pixels
            for(j=0;j<8;j++){
                if ((pixels>>j)&1==1){
                    LCD_drawPixel(x+i,y+j,fColor);
                }
                else {
                    LCD_drawPixel(x+i,y+j,bColor);
                }
            }
        }
    }
}

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color) {
    int x_begin = x0 - radius;
    int x_end = x0 + radius;
    int y_begin = y0 - radius;
    int y_end = y0 + radius;

    LCD_setAddr(x_begin, y_begin, x_end, y_end);

    for (int x = x_begin; x <= x_end; x++) {
        for (int y = y_begin; y <= y_end; y++) {
            if ((x-x0)*(x-x0) + (y-y0)*(y-y0) - radius*radius > 0) {
                //black background in game
                SPI_ControllerTx_16bit(rgb565(0, 0, 0));
            } else {
                SPI_ControllerTx_16bit(color);
            }
        }
    }

}



/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/

void plotLineLow(short x0,short y0,short x1,short y1, uint16_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yii = 1;
    if (dy<0) {
        yii = -1;
        dy = - dy;
    }
    int D = 2*dy - dx;
    int yi = y0;
    int xiprev = x0;
    int yiprev = y0;

    for (int xi = x0; xi <= x1; xi++){
        LCD_setAddr(xiprev,yiprev,xi,yi);

        for (int x = xiprev; x <= xi; x++) {
            for (int y = yiprev; y <= yi; y++) {
                SPI_ControllerTx_16bit(color);
            }
        }

        if (D > 0) {
            yi = yi + yii;
            D = D - 2*(dy - dx);
        } else {
            D = D + 2*dy;
        }

        xiprev = xi;
        yiprev = yi;

    }

}

void plotLineHigh(short x0,short y0,short x1,short y1, uint16_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xii = 1;
    if (dx<0) {
        xii = -1;
        dx = - dx;
    }
    int D = 2*dx - dy;
    int xi = x0;
    int xiprev = x0;
    int yiprev = y0;

    for (int yi = y0; yi >= y1; yi--){
        LCD_setAddr(xiprev,yiprev,xi,yi);

        for (int x = xiprev; x <= xi; x++) {
            for (int y = yiprev; y >= yi; y--) {
                SPI_ControllerTx_16bit(color);
            }
        }

        if (D > 0) {
            xi = xi + xii;
            D = D + 2*(dx - dy);
        } else {
            D = D + 2*dx;
        }

        xiprev = xi;
        yiprev = yi;

    }

}

void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t color)
{
    if (abs(y1 - y0) < abs(x1 - x0)){
        if (x0 > x1) {
            plotLineLow(x1, y1, x0, y0, color);
        } else {
            plotLineLow(x0, y0, x1, y1, color);
        }
    } else {
        if (y0 > y1) {
            plotLineHigh(x1, y1, x0, y0, color);
        } else {
            plotLineHigh(x0, y0, x1, y1, color);
        }

    }
}



/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
{

    if (x1 < x0) {
        uint8_t prevx0 = x0;
        x0 = x1;
        x1 = prevx0;
    }
    if (y1 < y0) {
        uint8_t prevy0 = y0;
        y0 = y1;
        y1 = prevy0;
    }
    LCD_setAddr(x0,y0,x1,y1);

    for (int x = x0; x <= x1; x++) {
        for (int y = y0; y <= y1; y++) {
            SPI_ControllerTx_16bit(color);
        }
    }
}

/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color)
{
    LCD_setAddr(0,0,500,500);

    for (int x = 0; x <= 180; x++) {
        for (int y = 0; y <= 128; y++) {
            SPI_ControllerTx_16bit(color);
        }
    }

}

/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
{
    int count = 0;
    for (int i  = 0; i < strlen(str); i++){
        LCD_drawChar(x + count, y, str[i], fg, bg);
        count += 10;
    }
