/*
 * graphics.h
 *
 *  Created on: Jun 29, 2019
 *      Author: george
 */

#ifndef UI_GRAPHICS_H_
#define UI_GRAPHICS_H_

#include <ti/grlib/grlib.h>

void fadeRectangle(Graphics_Context *gr_context, Graphics_Rectangle *rect);
void fadeRectangle_xy(Graphics_Context *gr_context, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void fillRectangle(Graphics_Context *gr_context, Graphics_Rectangle *rect);
void drawRectangle_xy(Graphics_Context *gr_context, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void fillRectangle_xy(Graphics_Context *gr_context, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void qc16gr_drawProgressBar(Graphics_Context *gr_context, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t progress, uint32_t total);
void qc16gr_drawImage(const Graphics_Context *context,
                      const Graphics_Image *bitmap,
                      int16_t x,
                      int16_t y);
void qc16gr_drawImageFromFile(const Graphics_Context *context,
                              char *pathname,
                              int16_t x,
                              int16_t y);
uint16_t qc16gr_get_image_size(const Graphics_Image *bitmap);
void qc16gr_draw_larrow(Graphics_Context *gr_context, uint16_t x, uint16_t y, uint16_t height);
void qc16gr_draw_rarrow(Graphics_Context *gr_context, uint16_t x, uint16_t y, uint16_t height);

#endif /* UI_GRAPHICS_H_ */
