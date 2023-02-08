#include "ssd1306.h"
#include "ssd1306_cfg.h"


// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t SSD1306_CMD_Buffer[3];
// Screen object
static volatile SSD1306_t SSD1306;


//
//  Send a byte to the command register
//
static uint8_t ssd1306_WriteCommand(uint8_t command)
{
    return HAL_I2C_Mem_Write(&SSD1306_HI2C, SSD1306_I2C_ADDR, 0x00, 1, &command, 1, 10);
}


//
//  Initialize the oled screen
//
uint8_t ssd1306_Init()
{
    // Wait for the screen to boot
    HAL_Delay(100);
    int status = 0;

    // Init LCD
    status += ssd1306_WriteCommand(0xAE);   // Display off
    status += ssd1306_WriteCommand(0x20);   // Set Memory Addressing Mode
    status += ssd1306_WriteCommand(0x10);   // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    status += ssd1306_WriteCommand(0xB0);   // Set Page Start Address for Page Addressing Mode,0-7
    status += ssd1306_WriteCommand(0xC8);   // Set COM Output Scan Direction
    status += ssd1306_WriteCommand(0x00);   // Set low column address
    status += ssd1306_WriteCommand(0x10);   // Set high column address
    status += ssd1306_WriteCommand(0x40);   // Set start line address
    status += ssd1306_WriteCommand(0x81);   // set contrast control register
    status += ssd1306_WriteCommand(0xFF);
    status += ssd1306_WriteCommand(0xA1);   // Set segment re-map 0 to 127
    status += ssd1306_WriteCommand(0xA6);   // Set normal display

    status += ssd1306_WriteCommand(0xA8);   // Set multiplex ratio(1 to 64)
    status += ssd1306_WriteCommand(SSD1306_HEIGHT - 1);

    status += ssd1306_WriteCommand(0xA4);   // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    status += ssd1306_WriteCommand(0xD3);   // Set display offset
    status += ssd1306_WriteCommand(0x00);   // No offset
    status += ssd1306_WriteCommand(0xD5);   // Set display clock divide ratio/oscillator frequency
    status += ssd1306_WriteCommand(0xF0);   // Set divide ratio
    status += ssd1306_WriteCommand(0xD9);   // Set pre-charge period
    status += ssd1306_WriteCommand(0x22);

    status += ssd1306_WriteCommand(0xDA);   // Set com pins hardware configuration
#ifdef SSD1306_COM_LR_REMAP
    status += ssd1306_WriteCommand(0x32);   // Enable COM left/right remap
#else
    status += ssd1306_WriteCommand(0x12);   // Do not use COM left/right remap
#endif // SSD1306_COM_LR_REMAP

    status += ssd1306_WriteCommand(0xDB);   // Set vcomh
    status += ssd1306_WriteCommand(0x20);   // 0x20,0.77xVcc
    status += ssd1306_WriteCommand(0x8D);   // Set DC-DC enable
    status += ssd1306_WriteCommand(0x14);   //
    status += ssd1306_WriteCommand(0xAF);   // Turn on SSD1306 panel

    if (status != 0) {
        return 1;
    }

    // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    ssd1306_UpdateScreen();

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;

    return 0;
}

//
//  Fill the whole screen with the given color
//
void ssd1306_Fill(SSD1306_COLOR color)
{
    // Fill screenbuffer with a constant value (color)
    uint32_t i;

    for(i = 0; i < sizeof(SSD1306_Buffer); i++)
    {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

//
//  Write the screenbuffer with changed to the screen
//
void ssd1306_UpdateScreen()
{


    SSD1306.Dma_sendFinished = 0;
    SSD1306.Dma_iter = 0;
    SSD1306.Dma_cmd_Nbuf = 1;

    SSD1306_CMD_Buffer[0] = 0xB0 + SSD1306.Dma_iter;
    SSD1306_CMD_Buffer[1] = 0x00;
    SSD1306_CMD_Buffer[2] = 0x10;

    HAL_I2C_Mem_Write_DMA(&SSD1306_HI2C, SSD1306_I2C_ADDR, 0x00, 0, SSD1306_CMD_Buffer, 3);

}

//
//  Draw one pixel in the screenbuffer
//  X => X Coordinate
//  Y => Y Coordinate
//  color => Pixel color
//
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    {
        // Don't write outside the buffer
        return;
    }

    // Check if pixel should be inverted
    if (SSD1306.Inverted)
    {
        color = (SSD1306_COLOR)!color;
    }
    // calculate idx
    #ifdef SSD1306_ROTATION_180
    size_t bufIdx = (SSD1306_WIDTH-x) + ((SSD1306_HEIGHT-y) / 8) * SSD1306_WIDTH;
    uint8_t bitIdx = ((SSD1306_HEIGHT-y) % 8);
    #else
    size_t bufIdx = (x) + ((y) / 8) * SSD1306_WIDTH;
    uint8_t bitIdx = ((y) % 8);
    #endif
    // Draw in the correct color
    if (color == White)
    {
        SSD1306_Buffer[bufIdx] |= 1 << bitIdx;
    }
    else
    {
        SSD1306_Buffer[bufIdx] &= ~(1 << bitIdx);
    }
}


//
//  Draw 1 char to the screen buffer
//  ch      => Character to write
//  Font    => Font to use
//  color   => Black or White
//
char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color)
{
    uint32_t i, b, j;

    // Check remaining space on current line
    if (SSD1306_WIDTH <= (SSD1306.CurrentX + Font.FontWidth) ||
        SSD1306_HEIGHT <= (SSD1306.CurrentY + Font.FontHeight))
    {
        // Not enough space on current line
        return 0;
    }

    // Translate font to screenbuffer
    for (i = 0; i < Font.FontHeight; i++)
    {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for (j = 0; j < Font.FontWidth; j++)
        {
            if ((b << j) & 0x8000)
            {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            }
            else
            {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    // The current space is now taken
    SSD1306.CurrentX += Font.FontWidth;

    // Return written char for validation
    return ch;
}

//
//  Write full string to screenbuffer
//
char ssd1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color)
{
    // Write until null-byte
    while (*str)
    {
        if (ssd1306_WriteChar(*str, Font, color) != *str)
        {
            // Char could not be written
            return *str;
        }

        // Next char
        str++;
    }

    // Everything ok
    return *str;
}

//
//  Invert background/foreground colors
//
void ssd1306_InvertColors(void)
{
    SSD1306.Inverted = !SSD1306.Inverted;
}

//
//  Set cursor position
//
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

/**
 * @brief display callback to put in  *void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef * hi2c)* function
 * 
 */
void ssd1306_DMA_callback()
{
    if (SSD1306.Dma_iter == 7 && SSD1306.Dma_cmd_Nbuf == 0){
        SSD1306.Dma_sendFinished = 1;
    }
    else if (SSD1306.Dma_cmd_Nbuf == 1) {
        SSD1306.Dma_cmd_Nbuf = 0;
        HAL_I2C_Mem_Write_DMA(&SSD1306_HI2C, SSD1306_I2C_ADDR, 0x40, 1, 
            &SSD1306_Buffer[SSD1306_WIDTH * SSD1306.Dma_iter], SSD1306_WIDTH);
    }
    else {
        SSD1306.Dma_iter++;
        SSD1306_CMD_Buffer[0] = 0xB0 + SSD1306.Dma_iter;
        SSD1306.Dma_cmd_Nbuf = 1;
        HAL_I2C_Mem_Write_DMA(&SSD1306_HI2C, SSD1306_I2C_ADDR, 0x00, 0, SSD1306_CMD_Buffer, 3);
    }
}


bool ssd1306_IsUpdateFinished(void)
{
    return SSD1306.Dma_sendFinished;
}


void ssd1306_waitForUpdate(void)
{ 
    while(!ssd1306_IsUpdateFinished()); 
}