#include "FT02_IconRenderer.h"

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
