#include "FT02_IconRenderer.h"

void FT02_DrawIcon(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    bool invert
)
{
    for(int row = 0; row < icon.height; row++)
    {
        for(int col = 0; col < icon.width; col++)
        {
            int byteIndex = row * icon.bytesPerRow + col / 8;
            uint8_t data = icon.bitmap[byteIndex];
            uint8_t mask = 0x80 >> (col % 8);
            bool on = (data & mask) != 0;

            if(invert)
            {
                on = !on;
            }

            if(on)
            {
                display.drawPixel(
                    x + col,
                    y + row,
                    GxEPD_BLACK
                );
            }
        }
    }
}


void FT02_DrawIconScaled(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    int scale,
    bool invert
)
{
    for(int row = 0; row < icon.height; row++)
    {
        for(int col = 0; col < icon.width; col++)
        {
            int byteIndex = row * icon.bytesPerRow + col / 8;
            uint8_t mask = 0x80 >> (col % 8);

            bool on = (icon.bitmap[byteIndex] & mask) != 0;

            if(invert)
            {
                on = !on;
            }

            if(on)
            {
                display.fillRect(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    GxEPD_BLACK
                );
            }
        }
    }
}


void FT02_DrawIconSize(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    int targetSize,
    bool invert
)
{
    for(int py = 0; py < targetSize; py++)
    {
        for(int px = 0; px < targetSize; px++)
        {
            int sx = px * icon.width / targetSize;
            int sy = py * icon.height / targetSize;

            int byteIndex = sy * icon.bytesPerRow + sx / 8;
            uint8_t mask = 0x80 >> (sx % 8);

            bool on = (icon.bitmap[byteIndex] & mask) != 0;

            if(invert)
                on = !on;

            if(on)
            {
                display.drawPixel(
                    x + px,
                    y + py,
                    GxEPD_BLACK
                );
            }
        }
    }
}
