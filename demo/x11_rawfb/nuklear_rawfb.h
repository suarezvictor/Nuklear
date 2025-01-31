/*
 * MIT License
 *
 * Copyright (c) 2016-2017 Patrick Rudolph <siro@das-labor.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_RAWFB_H_
#define NK_RAWFB_H_

struct rawfb_context;

typedef enum rawfb_pixel_layout {
    PIXEL_LAYOUT_XRGB_8888,
    PIXEL_LAYOUT_RGBX_8888
}
rawfb_pl;


/* All functions are thread-safe */
NK_API struct rawfb_context *nk_rawfb_init(void *fb, void *tex_mem, const unsigned int w, const unsigned int h, const unsigned int pitch, const rawfb_pl pl);
NK_API void                  nk_rawfb_render(const struct rawfb_context *rawfb, const struct nk_color clear, const unsigned char enable_clear);
NK_API void                  nk_rawfb_shutdown(struct rawfb_context *rawfb);
NK_API void                  nk_rawfb_resize_fb(struct rawfb_context *rawfb, void *fb, const unsigned int w, const unsigned int h, const unsigned int pitch, const rawfb_pl pl);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_RAWFB_IMPLEMENTATION

/*
//RAW API
nk_internal_clear(rawfb, col)
nk_internal_fill_rect(rawfb, x0, y0, x1, y1, col); 
nk_internal_line_horizontal(rawfb, x0, y, x1, col);
nk_internal_line_vertical(rawfb, x, y0, y1, col); 
nk_internal_line(rawfb, x0, y0, x1, y1, col);
nk_internal_img_setpixel(ptr, pitch, x, y, col)
nk_internal_blit_alpha_mask(ptr, pitch, src_alpha, x0, y0, x1, y1, src_pitch, col)
nk_internal_setpixel(rawfb, x, y, col); //clips internally, only used in ARC
*/


struct rawfb_image {
    void *pixels;
    int w, h, pitch;
    rawfb_pl pl;
    enum nk_font_atlas_format format;
};
struct rawfb_context {
    struct nk_context ctx;
    struct nk_rect scissors;
    struct rawfb_image fb;
    struct rawfb_image font_tex;
    struct nk_font_atlas atlas;
};

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

#if 0
#define DISABLE_CLEAR
#define DISABLE_RECT
#define DISABLE_RECT_MULTICOLOR
#define DISABLE_LINE_HORIZONTAL
#define DISABLE_LINE_VERTICAL
#define DISABLE_BLIT_ALPHA
#define DISABLE_LINE
#define DISABLE_SETPIXEL
#define DISABLE_ARC //calls setpixel
#endif


static void
nk_internal_img_setpixel(unsigned char *pixels, size_t pitch,
    const short x, const short y, unsigned int c)
{
    pixels += y * pitch;
#ifndef DISABLE_IMG_SETPIXEL
    ((unsigned int *)pixels)[x] = c;
#endif
}


static void
nk_internal_setpixel(const struct rawfb_context *rawfb,
    const short x, const short y, unsigned int c)
{
    if(x < rawfb->scissors.x || x >= rawfb->scissors.w)
        return;
    if(y < rawfb->scissors.y || y >= rawfb->scissors.h)
        return;
#ifndef DISABLE_SETPIXEL
    nk_internal_img_setpixel(rawfb->fb.pixels, rawfb->fb.pitch, x, y, c);
#endif
}

static void
nk_internal_line(const struct rawfb_context *rawfb,
    short x0, short y0, short x1, short y1, const unsigned int col)
{
    /* This function does not check for scissors or image borders.
     * The caller has to make sure it does no exceed bounds. */

	uint8_t *pixels = rawfb->fb.pixels;
	size_t pitch = rawfb->fb.pitch;

    int dx = x1 - x0;
    int dy = y1 - y0;
    int stepx, stepy, fraction;
    
    if (dx < 0) {
        dx = -dx;
        stepx = -1;
    } else stepx = 1;
    dx <<= 1;

    if (dy < 0) {
        dy = -dy;
        stepy = -1;
    } else stepy = 1;
    dy <<= 1;

#ifndef DISABLE_LINE
    nk_internal_img_setpixel(pixels, pitch, x0, y0, col);
    
    if (dx > dy)
    {
        fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            nk_internal_img_setpixel(pixels, pitch, x0, y0, col);
        }
    }
    else
    {
        fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            nk_internal_img_setpixel(pixels, pitch, x0, y0, col);
        }
    }
#endif
}

static void
nk_internal_line_vertical(const struct rawfb_context *rawfb,
    short x, short y0, short y1, unsigned col)
{
    /* This function does not check for scissors or image borders.
     * The caller has to make sure it does no exceed bounds. */

#ifndef DISABLE_LINE_VERTICAL
    unsigned int i, n;
    unsigned char *pixels = rawfb->fb.pixels;

    pixels += y0 * rawfb->fb.pitch + x*sizeof(col);
    n = y1 - y0;
    for (i = 0; i < n; i++)
    {
        *(unsigned *)pixels = col;
        pixels += rawfb->fb.pitch;
    }
#endif
}

static void
nk_internal_fill_rect(const struct rawfb_context *rawfb,
    const short x0, const short y0, const short x1, const short y1,
    const unsigned int col)
{
    /* This function does not check for scissors or image borders.
     * The caller has to make sure it does no exceed bounds. */

    unsigned int i, n;
    unsigned int c[16];
    unsigned char *pixels = rawfb->fb.pixels;
    unsigned int *ptr;
    for (i = 0; i < sizeof(c) / sizeof(c[0]); i++)
        c[i] = col;

    pixels += y0 * rawfb->fb.pitch;
    for (int y = y0; y < y1; y++)
    {
        ptr = (unsigned int *)pixels + x0;

        n = x1 - x0;
#ifndef DISABLE_RECT
        while (n > 16) { //TODO: alignment
            memcpy((void *)ptr, c, sizeof(c));
            n -= 16; ptr += 16;
        }
        for (i = 0; i < n; i++)
            ptr[i] = c[i];
#endif
        pixels += rawfb->fb.pitch;
    }
}

static void
nk_internal_line_horizontal(const struct rawfb_context *rawfb,
    const short x0, const short y, const short x1, unsigned int col)
{
    /* This function does not check for scissors or image borders.
     * The caller has to make sure it does no exceed bounds. */
#ifndef DISABLE_LINE_HORIZONTAL
  nk_internal_fill_rect(rawfb, x0, y, x1, y+1, col);
#endif
}

static void
nk_internal_clear(const struct rawfb_context *rawfb, unsigned int col)
{
#ifndef DISABLE_CLEAR
    nk_internal_fill_rect(rawfb, 0, 0, rawfb->fb.w, rawfb->fb.h, col);
#endif
}

static void
nk_internal_blit(uint8_t *dst, size_t dst_pitch, const uint8_t *src_rgba,
    short x0, short y0, short x1, short y1, size_t src_pitch, uint8_t src_stride, struct nk_color fg, struct nk_color bg)
{
    /* This function does not check for scissors or image borders.
     * The caller has to make sure it does no exceed bounds. */
     
    //rect is dst
    dst += y0*dst_pitch;
    for (int y = y0; y < y1; y++)
    {
        const uint8_t *src = src_rgba;
        unsigned *dstp = (unsigned *)dst;
        dstp += x0;
        for (int x = x0; x < x1; x++)
        {
            unsigned src_c = *(unsigned *)src;
            struct nk_color sc = { src_c>>24, src_c>>16, src_c>>8, src_c};
            if(sc.g || sc.b || sc.r)
            {
            	*dstp++ = -1;
            }
            else
            	*dstp++ = 0;
         
/*
            struct nk_color col;
            uint8_t a = sc.a;
            uint8_t inv_a = a^255;
            col.r = (fg.r*a + sc.r*inv_a)>>8;
            col.g = (fg.g*a + sc.g*inv_a)>>8;
            col.b = (fg.b*a + sc.b*inv_a)>>8;
            col.a = 0xFF;
            
            col = sc;

            unsigned int c = 0;
	        c |= col.a << 24; //assumes XRGB
	        c |= col.r << 16;
	        c |= col.g << 8;
	        c |= col.b;

          	*dstp++ = c;
          	*/
          	src += src_stride;
        }
        src_rgba += src_pitch;
        dst += dst_pitch;
    }
}

static void
nk_internal_blit_alpha_mask(uint8_t *dst, size_t dst_pitch, const uint8_t *src_alpha,
    short x0, short y0, short x1, short y1, size_t src_pitch, uint8_t src_stride, struct nk_color fg, struct nk_color bg)
{
    /* This function does not check for scissors or image borders.
     * The caller has to make sure it does no exceed bounds. */
     
    //rect is dst
    dst += y0*dst_pitch;
    for (int y = y0; y < y1; y++)
    {
        const uint8_t *src = src_alpha;
        unsigned *dstp = (unsigned *)dst;
        dstp += x0;
        for (int x = x0; x < x1; x++)
        {
            struct nk_color col;
            uint8_t a = *src;
            uint8_t inv_a = a^255;
            
            col.r = (fg.r*a + bg.r*inv_a)>>8;
            col.g = (fg.g*a + bg.g*inv_a)>>8;
            col.b = (fg.b*a + bg.b*inv_a)>>8;
            col.a = 0xFF;

            unsigned int c = 0;
	        c |= col.a << 24; //assumes XRGB
	        c |= col.r << 16;
	        c |= col.g << 8;
	        c |= col.b;

          	*dstp++ = c;
          	src += src_stride;
        }
        src_alpha += src_pitch;
        dst += dst_pitch;
    }
}

/*
 * ==============================================================
 *
 *                          GENERIC IMPLEMENTATION
 *
 * ===============================================================
 */

static unsigned int
nk_rawfb_color2int(const struct nk_color c, rawfb_pl pl)
{
    unsigned int res = 0;

    switch (pl) {
    case PIXEL_LAYOUT_RGBX_8888:
	res |= c.r << 24;
	res |= c.g << 16;
	res |= c.b << 8;
	res |= c.a;
	break;
    case PIXEL_LAYOUT_XRGB_8888:
	res |= c.a << 24;
	res |= c.r << 16;
	res |= c.g << 8;
	res |= c.b;
	break;

    default:
	perror("nk_rawfb_color2int(): Unsupported pixel layout.\n");
	break;
    }
    return (res);
}

static struct nk_color
nk_rawfb_int2color(const unsigned int i, rawfb_pl pl)
{
    struct nk_color col = {0,0,0,0};

    switch (pl) {
    case PIXEL_LAYOUT_RGBX_8888:
	col.r = (i >> 24) & 0xff;
	col.g = (i >> 16) & 0xff;
	col.b = (i >> 8) & 0xff;
	col.a = i & 0xff;
	break;
    case PIXEL_LAYOUT_XRGB_8888:
	col.a = (i >> 24) & 0xff;
	col.r = (i >> 16) & 0xff;
	col.g = (i >> 8) & 0xff;
	col.b = i & 0xff;
	break;

    default:
	perror("nk_rawfb_int2color(): Unsupported pixel layout.\n");
	break;
    }
    return col;
}

static void
nk_rawfb_line_vertical(const struct rawfb_context *rawfb,
    short x, short y0, short y1, const struct nk_color col)
{
    if(x < rawfb->scissors.x || x >= rawfb->scissors.w)
        return;
     
    y0 = MAX(rawfb->scissors.y, MIN(rawfb->scissors.h, y0));
    y1 = MAX(rawfb->scissors.y, MIN(rawfb->scissors.h, y1));
     
    unsigned int c = nk_rawfb_color2int(col, rawfb->fb.pl);
    nk_internal_line_vertical(rawfb, x, y0, y1, c);
}

static void	
nk_rawfb_line_horizontal(const struct rawfb_context *rawfb,
    short x0, short y, short x1, const struct nk_color col)
{
    if(y < rawfb->scissors.y || y >= rawfb->scissors.h)
        return;

    x0 = MAX(rawfb->scissors.x, MIN(rawfb->scissors.w, x0));
    x1 = MAX(rawfb->scissors.x, MIN(rawfb->scissors.w, x1));

    unsigned c = nk_rawfb_color2int(col, rawfb->fb.pl);
    nk_internal_line_horizontal(rawfb, x0, y, x1, c);
}

static void
nk_rawfb_img_setpixel(const struct rawfb_image *img,
    const int x0, const int y0, const struct nk_color col)
{
    unsigned int c = nk_rawfb_color2int(col, img->pl);
    unsigned char *ptr;
    NK_ASSERT(img);
    if (y0 < img->h && y0 >= 0 && x0 >= 0 && x0 < img->w) {
        ptr = (unsigned char *)img->pixels + (img->pitch * y0);
        if (img->format == NK_FONT_ATLAS_ALPHA8) {
            ptr[x0] = col.a;
        }
        else {
			nk_internal_img_setpixel(img->pixels, img->pitch, x0, y0, c); //TODO: needs testing
        }
    }
}

static struct nk_color
nk_rawfb_img_getpixel(const struct rawfb_image *img, const int x0, const int y0)
{
    struct nk_color col = {0, 0, 0, 0};
    unsigned char *ptr;
    unsigned int pixel;
    NK_ASSERT(img);
    if (y0 < img->h && y0 >= 0 && x0 >= 0 && x0 < img->w) {
        ptr = (unsigned char *)img->pixels + (img->pitch * y0);

        if (img->format == NK_FONT_ATLAS_ALPHA8) {
            col.a = ptr[x0];
            col.b = col.g = col.r = 0xff;
        } else {
	    pixel = ((unsigned int *)ptr)[x0];
	    col = nk_rawfb_int2color(pixel, img->pl);
        }
    } return col;
}
static void
nk_rawfb_img_blendpixel(const struct rawfb_image *img,
    const int x0, const int y0, struct nk_color col)
{
    struct nk_color col2;
    unsigned char inv_a;
    if (col.a == 0)
        return;

    inv_a = 0xff - col.a;
    col2 = nk_rawfb_img_getpixel(img, x0, y0);
    col.r = (col.r * col.a + col2.r * inv_a) >> 8;
    col.g = (col.g * col.a + col2.g * inv_a) >> 8;
    col.b = (col.b * col.a + col2.b * inv_a) >> 8;
    nk_rawfb_img_setpixel(img, x0, y0, col);
}

static void
nk_rawfb_scissor(struct rawfb_context *rawfb,
                 const float x,
                 const float y,
                 const float w,
                 const float h)
{
    rawfb->scissors.x = MIN(MAX(x, 0), rawfb->fb.w);
    rawfb->scissors.y = MIN(MAX(y, 0), rawfb->fb.h);
    rawfb->scissors.w = MIN(MAX(w + x, 0), rawfb->fb.w);
    rawfb->scissors.h = MIN(MAX(h + y, 0), rawfb->fb.h);
}

//written by ChatGPT with prompt "Liang–Barsky in C"
int liang_barsky(short *x0p, short *y0p, short *x1p, short *y1p,
                 float xmin, float ymin, float xmax, float ymax)
{
    float x0 = *x0p, y0 = *y0p, x1 = *x1p, y1 = *y1p;
    
	//chatGPT code BEGIN
    float t0 = 0.0;
    float t1 = 1.0;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float p, q, r;

    for (int edge = 0; edge < 4; edge++)
    {
        if (edge == 0) {  // left edge
            p = -dx;
            q = x0 - xmin;
        }
        else if (edge == 1) {  // right edge
            p = dx;
            q = xmax - x0;
        }
        else if (edge == 2) {  // bottom edge
            p = -dy;
            q = y0 - ymin;
        }
        else {  // top edge
            p = dy;
            q = ymax - y0;
        }


        if (p == 0)
        {
        	if(q < 0)
				return 0;  // line is parallel to edge and outside
        }
        else if (p < 0) {
	        r = q / p; //this moved here from chatGPT to avoid division by zero
            if (r > t1)
            	return 0;
            if (r > t0)
            	t0 = r;
        }
        else if (p > 0) {
	        r = q / p; //this moved here from chatGPT to avoid division by zero
            if (r < t0)
            	return 0;
            if (r < t1)
            	t1 = r;
        }
    }

    // line is partially inside
    float x1_new = x0 + t0 * dx;
    float y1_new = y0 + t0 * dy;
    float x2_new = x0 + t1 * dx;
    float y2_new = y0 + t1 * dy;

    //chatGPT code END

    *x0p = x1_new;
    *y0p = y1_new;
    *x1p = x2_new;
    *y1p = y2_new;
    return 1;
}

static void
nk_rawfb_stroke_line(const struct rawfb_context *rawfb,
    short x0, short y0, short x1, short y1,
    const unsigned int line_thickness, const struct nk_color col)
{
    short tmp;
    unsigned int c = nk_rawfb_color2int(col, rawfb->fb.pl);

    NK_UNUSED(line_thickness);

    /* fast path */
    if (y0 == y1) {
        if (x0 == x1)
            return;

        if (x0 > x1) {
            /* swap x0 and x1 */
            tmp = x1;
            x1 = x0;
            x0 = tmp;
        }
        nk_rawfb_line_horizontal(rawfb, x0, y0, x1, col);
        return;
    }
    if(x0 == x1)
    {
        if (y0 > y1) {
            /* swap x0 and x1 */
            tmp = y1;
            y1 = y0;
            y0 = tmp;
        }
        nk_rawfb_line_vertical(rawfb, x0, y0, y1, col);
        return;
    }

	if(!liang_barsky(&x0, &y0, &x1, &y1,
		rawfb->scissors.x, rawfb->scissors.y, rawfb->scissors.w-1, rawfb->scissors.h-1))
		return;

    nk_internal_line(rawfb, x0, y0, x1, y1, c);
}

static void
nk_rawfb_fill_polygon(const struct rawfb_context *rawfb,
    const struct nk_vec2i *pnts, int count, const struct nk_color col)
{
    int i = 0;
    #define MAX_POINTS 64
    int left = 10000, top = 10000, bottom = 0, right = 0;
    int nodes, nodeX[MAX_POINTS], j, swap;

    if (count == 0) return;
    if (count > MAX_POINTS)
        count = MAX_POINTS;

    /* Get polygon dimensions */
    for (i = 0; i < count; i++) {
        if (left > pnts[i].x)
            left = pnts[i].x;
        if (right < pnts[i].x)
            right = pnts[i].x;
        if (top > pnts[i].y)
            top = pnts[i].y;
        if (bottom < pnts[i].y)
            bottom = pnts[i].y;
    } bottom++; right++;

    /* Polygon scanline algorithm released under public-domain by Darel Rex Finley, 2007 */
    /*  Loop through the rows of the image. */
    for (int pixelY = top; pixelY < bottom; pixelY ++) {
#warning optimize this
        if(pixelY < rawfb->scissors.y || pixelY >= rawfb->scissors.h)
          continue; //FIXME: draw only within y range

        nodes = 0; /*  Build a list of nodes. */
        j = count - 1;
        for (i = 0; i < count; i++) {
            if (((pnts[i].y < pixelY) && (pnts[j].y >= pixelY)) ||
                ((pnts[j].y < pixelY) && (pnts[i].y >= pixelY))) {
                nodeX[nodes++]= (int)((float)pnts[i].x
                     + ((float)pixelY - (float)pnts[i].y) / ((float)pnts[j].y - (float)pnts[i].y)
                     * ((float)pnts[j].x - (float)pnts[i].x));
            } j = i;
        }

        /*  Sort the nodes, via a simple “Bubble” sort. */
        i = 0;
        while (i < nodes - 1) {
            if (nodeX[i] > nodeX[i+1]) {
                swap = nodeX[i];
                nodeX[i] = nodeX[i+1];
                nodeX[i+1] = swap;
                if (i) i--;
            } else i++;
        }
        /*  Fill the pixels between node pairs. */
        for (i = 0; i < nodes; i += 2) {
            if (nodeX[i+0] >= right) break;
            if (nodeX[i+1] > left) {
                if (nodeX[i+0] < left) nodeX[i+0] = left ;
                if (nodeX[i+1] > right) nodeX[i+1] = right;
                nk_rawfb_line_horizontal(rawfb, nodeX[i], pixelY, nodeX[i + 1], col);
            }
        }
    }
    #undef MAX_POINTS
}


static void
nk_rawfb_stroke_arc(const struct rawfb_context *rawfb,
    short x0, short y0, short w, short h, const short s,
    const short line_thickness, const struct nk_color col)
{
    unsigned int c = nk_rawfb_color2int(col, rawfb->fb.pl);
    
    /* Bresenham's ellipses - modified to draw one quarter */
    const int a2 = (w * w) / 4;
    const int b2 = (h * h) / 4;
    const int fa2 = 4 * a2, fb2 = 4 * b2;
    int x, y, sigma;

    NK_UNUSED(line_thickness);

    if (s != 0 && s != 90 && s != 180 && s != 270) return;
    if (w < 1 || h < 1) return;

    /* Convert upper left to center */
    h = (h + 1) / 2;
    w = (w + 1) / 2;
    x0 += w; y0 += h;

#ifndef DISABLE_ARC
    /* First half */
    for (x = 0, y = h, sigma = 2*b2+a2*(1-2*h); b2*x <= a2*y; x++) {
        if (s == 180)
            nk_internal_setpixel(rawfb, x0 + x, y0 + y, c);
        else if (s == 270)
            nk_internal_setpixel(rawfb, x0 - x, y0 + y, c);
        else if (s == 0)
            nk_internal_setpixel(rawfb, x0 + x, y0 - y, c);
        else if (s == 90)
            nk_internal_setpixel(rawfb, x0 - x, y0 - y, c);
        if (sigma >= 0) {
            sigma += fa2 * (1 - y);
            y--;
        } sigma += b2 * ((4 * x) + 6);
    }

    /* Second half */
    for (x = w, y = 0, sigma = 2*a2+b2*(1-2*w); a2*y <= b2*x; y++) {
        if (s == 180)
            nk_internal_setpixel(rawfb, x0 + x, y0 + y, c);
        else if (s == 270)
            nk_internal_setpixel(rawfb, x0 - x, y0 + y, c);
        else if (s == 0)
            nk_internal_setpixel(rawfb, x0 + x, y0 - y, c);
        else if (s == 90)
            nk_internal_setpixel(rawfb, x0 - x, y0 - y, c);
        if (sigma >= 0) {
            sigma += fb2 * (1 - x);
            x--;
        } sigma += a2 * ((4 * y) + 6);
    }
#endif
}

static void
nk_rawfb_fill_arc(const struct rawfb_context *rawfb, short x0, short y0,
    short w, short h, const short s, const struct nk_color col)
{
    /* Bresenham's ellipses - modified to fill one quarter */
    const int a2 = (w * w) / 4;
    const int b2 = (h * h) / 4;
    const int fa2 = 4 * a2, fb2 = 4 * b2;
    int x, y, sigma;
    struct nk_vec2i pnts[3];
    if (w < 1 || h < 1) return;
    if (s != 0 && s != 90 && s != 180 && s != 270)
        return;

    /* Convert upper left to center */
    h = (h + 1) / 2;
    w = (w + 1) / 2;
    x0 += w;
    y0 += h;

    pnts[0].x = x0;
    pnts[0].y = y0;
    pnts[2].x = x0;
    pnts[2].y = y0;

    /* First half */
    for (x = 0, y = h, sigma = 2*b2+a2*(1-2*h); b2*x <= a2*y; x++) {
        if (s == 180) {
            pnts[1].x = x0 + x; pnts[1].y = y0 + y;
        } else if (s == 270) {
            pnts[1].x = x0 - x; pnts[1].y = y0 + y;
        } else if (s == 0) {
            pnts[1].x = x0 + x; pnts[1].y = y0 - y;
        } else if (s == 90) {
            pnts[1].x = x0 - x; pnts[1].y = y0 - y;
        }
        nk_rawfb_fill_polygon(rawfb, pnts, 3, col);
        pnts[2] = pnts[1];
        if (sigma >= 0) {
            sigma += fa2 * (1 - y);
            y--;
        } sigma += b2 * ((4 * x) + 6);
    }

    /* Second half */
    for (x = w, y = 0, sigma = 2*a2+b2*(1-2*w); a2*y <= b2*x; y++) {
        if (s == 180) {
            pnts[1].x = x0 + x; pnts[1].y = y0 + y;
        } else if (s == 270) {
            pnts[1].x = x0 - x; pnts[1].y = y0 + y;
        } else if (s == 0) {
            pnts[1].x = x0 + x; pnts[1].y = y0 - y;
        } else if (s == 90) {
            pnts[1].x = x0 - x; pnts[1].y = y0 - y;
        }
        nk_rawfb_fill_polygon(rawfb, pnts, 3, col);
        pnts[2] = pnts[1];
        if (sigma >= 0) {
            sigma += fb2 * (1 - x);
            x--;
        } sigma += a2 * ((4 * y) + 6);
    }
}

static void
nk_rawfb_stroke_rect(const struct rawfb_context *rawfb,
    const short x, const short y, const short w, const short h,
    const short r, const short line_thickness, const struct nk_color col)
{
#warning reenable thickness
    if (r == 0) {
        nk_rawfb_line_horizontal(rawfb, x, y, x + w, /*line_thickness,*/ col);
        nk_rawfb_line_vertical(rawfb, x, y + h, y + h/*, line_thickness*/, col);
        nk_rawfb_line_horizontal(rawfb, x, y, x + w, /*line_thickness,*/ col);
        nk_rawfb_line_vertical(rawfb, x + w, y, y + h/*, line_thickness*/, col);
    } else {
        const short xc = x + r;
        const short yc = y + r;
        const short wc = (short)(w - 2 * r);
        const short hc = (short)(h - 2 * r);

        nk_rawfb_line_horizontal(rawfb, xc, y, xc + wc/*, line_thickness*/, col);
        nk_rawfb_line_vertical(rawfb, x + w, yc, yc + hc/*, line_thickness*/, col);
        nk_rawfb_line_horizontal(rawfb, xc, y + h, xc + wc/*, line_thickness*/, col);
        nk_rawfb_line_vertical(rawfb, x, yc, yc + hc/*, line_thickness*/, col);

        nk_rawfb_stroke_arc(rawfb, xc + wc - r, y,
                (unsigned)r*2, (unsigned)r*2, 0 , line_thickness, col);
        nk_rawfb_stroke_arc(rawfb, x, y,
                (unsigned)r*2, (unsigned)r*2, 90 , line_thickness, col);
        nk_rawfb_stroke_arc(rawfb, x, yc + hc - r,
                (unsigned)r*2, (unsigned)r*2, 270 , line_thickness, col);
        nk_rawfb_stroke_arc(rawfb, xc + wc - r, yc + hc - r,
                (unsigned)r*2, (unsigned)r*2, 180 , line_thickness, col);
    }
}

static void
nk_rawfb_fill_rect(const struct rawfb_context *rawfb,
    const short x, const short y, short w, short h,
    short r, const struct nk_color col)
{
    if (r == 0) {
        //clip
        int x0 = MAX(rawfb->scissors.x, MIN(rawfb->scissors.w, x));
        int y0 = MAX(rawfb->scissors.y, MIN(rawfb->scissors.h, y));
        int x1 = MAX(rawfb->scissors.x, MIN(rawfb->scissors.w, x+w));
        int y1 = MAX(rawfb->scissors.y, MIN(rawfb->scissors.h, y+h));
        unsigned int c = nk_rawfb_color2int(col, rawfb->fb.pl);
        nk_internal_fill_rect(rawfb, x0, y0, x1, y1, c);
    } else {
        const short xc = x + r;
        const short yc = y + r;
        const short wc = (short)(w - 2 * r);
        const short hc = (short)(h - 2 * r);
#if 1
        //recursive calls but with radius 0
        nk_rawfb_fill_arc(rawfb, x, y, r*2, r*2, 90 , col); //90 correct
        nk_rawfb_fill_rect(rawfb, xc, y, wc, r, 0, col);
        nk_rawfb_fill_arc(rawfb, x + wc, y, r*2, r*2, 0 , col); //0 correct
        nk_rawfb_fill_rect(rawfb, x, yc, w, hc+1, 0, col);
        nk_rawfb_fill_arc(rawfb, x, y + hc, r*2, r*2, 270, col);
        nk_rawfb_fill_rect(rawfb, xc, yc + hc, wc, r, 0, col); //270 correct
        nk_rawfb_fill_arc(rawfb, x + wc, y + hc, r*2, r*2, 180 , col); //180 correct
#else
        struct nk_vec2i pnts[12];
        pnts[0].x = x;
        pnts[0].y = yc;
        pnts[1].x = xc;
        pnts[1].y = yc;
        pnts[2].x = xc;
        pnts[2].y = y;

        pnts[3].x = xc + wc;
        pnts[3].y = y;
        pnts[4].x = xc + wc;
        pnts[4].y = yc;
        pnts[5].x = x + w;
        pnts[5].y = yc;

        pnts[6].x = x + w;
        pnts[6].y = yc + hc;
        pnts[7].x = xc + wc;
        pnts[7].y = yc + hc;
        pnts[8].x = xc + wc;
        pnts[8].y = y + h;

        pnts[9].x = xc;
        pnts[9].y = y + h;
        pnts[10].x = xc;
        pnts[10].y = yc + hc;
        pnts[11].x = x;
        pnts[11].y = yc + hc;

        nk_rawfb_fill_polygon(rawfb, pnts, 12, col);
        nk_rawfb_fill_arc(rawfb, xc + wc - r, y,
                (unsigned)r*2, (unsigned)r*2, 0 , col);
        nk_rawfb_fill_arc(rawfb, x, y,
                (unsigned)r*2, (unsigned)r*2, 90 , col);
        nk_rawfb_fill_arc(rawfb, x, yc + hc - r,
                (unsigned)r*2, (unsigned)r*2, 270 , col);
        nk_rawfb_fill_arc(rawfb, xc + wc - r, yc + hc - r,
                (unsigned)r*2, (unsigned)r*2, 180 , col);
#endif
    }
}

NK_API void
nk_rawfb_draw_rect_multi_color(const struct rawfb_context *rawfb,
    const short x, const short y, const short w, const short h, struct nk_color tl,
    struct nk_color tr, struct nk_color br, struct nk_color bl)
{
    int i, j;
    struct nk_color *edge_buf;
    struct nk_color *edge_t;
    struct nk_color *edge_b;
    struct nk_color *edge_l;
    struct nk_color *edge_r;
    struct nk_color pixel;

    edge_buf = malloc(((2*w) + (2*h)) * sizeof(struct nk_color));
    if (edge_buf == NULL)
	return;

    edge_t = edge_buf;
    edge_b = edge_buf + w;
    edge_l = edge_buf + (w*2);
    edge_r = edge_buf + (w*2) + h;

    /* Top and bottom edge gradients */
    for (i=0; i<w; i++)
    {
	edge_t[i].r = (((((float)tr.r - tl.r)/(w-1))*i) + 0.5) + tl.r;
	edge_t[i].g = (((((float)tr.g - tl.g)/(w-1))*i) + 0.5) + tl.g;
	edge_t[i].b = (((((float)tr.b - tl.b)/(w-1))*i) + 0.5) + tl.b;
	edge_t[i].a = (((((float)tr.a - tl.a)/(w-1))*i) + 0.5) + tl.a;

	edge_b[i].r = (((((float)br.r - bl.r)/(w-1))*i) + 0.5) + bl.r;
	edge_b[i].g = (((((float)br.g - bl.g)/(w-1))*i) + 0.5) + bl.g;
	edge_b[i].b = (((((float)br.b - bl.b)/(w-1))*i) + 0.5) + bl.b;
	edge_b[i].a = (((((float)br.a - bl.a)/(w-1))*i) + 0.5) + bl.a;
    }

    /* Left and right edge gradients */
    for (i=0; i<h; i++)
    {
	edge_l[i].r = (((((float)bl.r - tl.r)/(h-1))*i) + 0.5) + tl.r;
	edge_l[i].g = (((((float)bl.g - tl.g)/(h-1))*i) + 0.5) + tl.g;
	edge_l[i].b = (((((float)bl.b - tl.b)/(h-1))*i) + 0.5) + tl.b;
	edge_l[i].a = (((((float)bl.a - tl.a)/(h-1))*i) + 0.5) + tl.a;

	edge_r[i].r = (((((float)br.r - tr.r)/(h-1))*i) + 0.5) + tr.r;
	edge_r[i].g = (((((float)br.g - tr.g)/(h-1))*i) + 0.5) + tr.g;
	edge_r[i].b = (((((float)br.b - tr.b)/(h-1))*i) + 0.5) + tr.b;
	edge_r[i].a = (((((float)br.a - tr.a)/(h-1))*i) + 0.5) + tr.a;
    }
#ifndef DISABLE_RECT_MULTICOLOR
    for (i=0; i<h; i++) {
	for (j=0; j<w; j++) {
	    if (i==0) {
		nk_rawfb_img_blendpixel(&rawfb->fb, x+j, y+i, edge_t[j]);
	    } else if (i==h-1) {
		nk_rawfb_img_blendpixel(&rawfb->fb, x+j, y+i, edge_b[j]);
	    } else {
		if (j==0) {
		    nk_rawfb_img_blendpixel(&rawfb->fb, x+j, y+i, edge_l[i]);
		} else if (j==w-1) {
		    nk_rawfb_img_blendpixel(&rawfb->fb, x+j, y+i, edge_r[i]);
		} else {
		    pixel.r = (((((float)edge_r[i].r - edge_l[i].r)/(w-1))*j) + 0.5) + edge_l[i].r;
		    pixel.g = (((((float)edge_r[i].g - edge_l[i].g)/(w-1))*j) + 0.5) + edge_l[i].g;
		    pixel.b = (((((float)edge_r[i].b - edge_l[i].b)/(w-1))*j) + 0.5) + edge_l[i].b;
		    pixel.a = (((((float)edge_r[i].a - edge_l[i].a)/(w-1))*j) + 0.5) + edge_l[i].a;
		    nk_rawfb_img_blendpixel(&rawfb->fb, x+j, y+i, pixel);
		}
	    }
	}
    }
#endif
    free(edge_buf);
}

static void
nk_rawfb_fill_triangle(const struct rawfb_context *rawfb,
    const short x0, const short y0, const short x1, const short y1,
    const short x2, const short y2, const struct nk_color col)
{
    struct nk_vec2i pnts[3];
    pnts[0].x = x0;
    pnts[0].y = y0;
    pnts[1].x = x1;
    pnts[1].y = y1;
    pnts[2].x = x2;
    pnts[2].y = y2;
    nk_rawfb_fill_polygon(rawfb, pnts, 3, col);
}

static void
nk_rawfb_stroke_triangle(const struct rawfb_context *rawfb,
    const short x0, const short y0, const short x1, const short y1,
    const short x2, const short y2, const unsigned short line_thickness,
    const struct nk_color col)
{
    nk_rawfb_stroke_line(rawfb, x0, y0, x1, y1, line_thickness, col);
    nk_rawfb_stroke_line(rawfb, x1, y1, x2, y2, line_thickness, col);
    nk_rawfb_stroke_line(rawfb, x2, y2, x0, y0, line_thickness, col);
}

static void
nk_rawfb_stroke_polygon(const struct rawfb_context *rawfb,
    const struct nk_vec2i *pnts, const int count,
    const unsigned short line_thickness, const struct nk_color col)
{
    int i;
    for (i = 1; i < count; ++i)
        nk_rawfb_stroke_line(rawfb, pnts[i-1].x, pnts[i-1].y, pnts[i].x,
                pnts[i].y, line_thickness, col);
    nk_rawfb_stroke_line(rawfb, pnts[count-1].x, pnts[count-1].y,
            pnts[0].x, pnts[0].y, line_thickness, col);
}

static void
nk_rawfb_stroke_polyline(const struct rawfb_context *rawfb,
    const struct nk_vec2i *pnts, const int count,
    const unsigned short line_thickness, const struct nk_color col)
{
    int i;
    for (i = 0; i < count-1; ++i)
        nk_rawfb_stroke_line(rawfb, pnts[i].x, pnts[i].y,
                 pnts[i+1].x, pnts[i+1].y, line_thickness, col);
}

static void
nk_rawfb_fill_circle(const struct rawfb_context *rawfb,
    short x0, short y0, short w, short h, const struct nk_color col)
{
    /* Bresenham's ellipses */
    const int a2 = (w * w) / 4;
    const int b2 = (h * h) / 4;
    const int fa2 = 4 * a2, fb2 = 4 * b2;
    int x, y, sigma;

    /* Convert upper left to center */
    h = (h + 1) / 2;
    w = (w + 1) / 2;
    x0 += w;
    y0 += h;

    /* First half */
    for (x = 0, y = h, sigma = 2*b2+a2*(1-2*h); b2*x <= a2*y; x++) {
        nk_rawfb_stroke_line(rawfb, x0 - x, y0 + y, x0 + x, y0 + y, 1, col);
        nk_rawfb_stroke_line(rawfb, x0 - x, y0 - y, x0 + x, y0 - y, 1, col);
        if (sigma >= 0) {
            sigma += fa2 * (1 - y);
            y--;
        } sigma += b2 * ((4 * x) + 6);
    }
    /* Second half */
    for (x = w, y = 0, sigma = 2*a2+b2*(1-2*w); a2*y <= b2*x; y++) {
        nk_rawfb_stroke_line(rawfb, x0 - x, y0 + y, x0 + x, y0 + y, 1, col);
        nk_rawfb_stroke_line(rawfb, x0 - x, y0 - y, x0 + x, y0 - y, 1, col);
        if (sigma >= 0) {
            sigma += fb2 * (1 - x);
            x--;
        } sigma += a2 * ((4 * y) + 6);
    }
}

static void
nk_rawfb_stroke_circle(const struct rawfb_context *rawfb,
    short x0, short y0, short w, short h, const short line_thickness,
    const struct nk_color col)
{
#if 0
    /* Bresenham's ellipses */
    const int a2 = (w * w) / 4;
    const int b2 = (h * h) / 4;
    const int fa2 = 4 * a2, fb2 = 4 * b2;
    int x, y, sigma;

    NK_UNUSED(line_thickness);

    /* Convert upper left to center */
    h = (h + 1) / 2;
    w = (w + 1) / 2;
    x0 += w;
    y0 += h;

    /* First half */
    for (x = 0, y = h, sigma = 2*b2+a2*(1-2*h); b2*x <= a2*y; x++) {
        nk_internal_ctx_setpixel(rawfb, x0 + x, y0 + y, col);
        nk_internal_ctx_setpixel(rawfb, x0 - x, y0 + y, col);
        nk_internal_ctx_setpixel(rawfb, x0 + x, y0 - y, col);
        nk_internal_ctx_setpixel(rawfb, x0 - x, y0 - y, col);
        if (sigma >= 0) {
            sigma += fa2 * (1 - y);
            y--;
        } sigma += b2 * ((4 * x) + 6);
    }
    /* Second half */
    for (x = w, y = 0, sigma = 2*a2+b2*(1-2*w); a2*y <= b2*x; y++) {
        nk_internal_ctx_setpixel(rawfb, x0 + x, y0 + y, col);
        nk_internal_ctx_setpixel(rawfb, x0 - x, y0 + y, col);
        nk_internal_ctx_setpixel(rawfb, x0 + x, y0 - y, col);
        nk_internal_ctx_setpixel(rawfb, x0 - x, y0 - y, col);
        if (sigma >= 0) {
            sigma += fb2 * (1 - x);
            x--;
        } sigma += a2 * ((4 * y) + 6);
    }
#else
    nk_rawfb_stroke_arc(rawfb, x0, y0, w, h,   0, line_thickness, col);
    nk_rawfb_stroke_arc(rawfb, x0, y0, w, h,  90, line_thickness, col);
    nk_rawfb_stroke_arc(rawfb, x0, y0, w, h, 180, line_thickness, col);
    nk_rawfb_stroke_arc(rawfb, x0, y0, w, h, 270, line_thickness, col);
#endif
}

static void
nk_rawfb_stroke_curve(const struct rawfb_context *rawfb,
    const struct nk_vec2i p1, const struct nk_vec2i p2,
    const struct nk_vec2i p3, const struct nk_vec2i p4,
    const unsigned int num_segments, const unsigned short line_thickness,
    const struct nk_color col)
{
    unsigned int i_step, segments;
    float t_step;
    struct nk_vec2i last = p1;

    segments = MAX(num_segments, 1);
    t_step = 1.0f/(float)segments;
    for (i_step = 1; i_step <= segments; ++i_step) {
        float t = t_step * (float)i_step;
        float u = 1.0f - t;
        float w1 = u*u*u;
        float w2 = 3*u*u*t;
        float w3 = 3*u*t*t;
        float w4 = t * t *t;
        float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
        float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
        nk_rawfb_stroke_line(rawfb, last.x, last.y,
                (short)x, (short)y, line_thickness,col);
        last.x = (short)x; last.y = (short)y;
    }
}

static void
nk_rawfb_clear(const struct rawfb_context *rawfb, const struct nk_color col)
{
    nk_internal_clear(rawfb, nk_rawfb_color2int(col, rawfb->fb.pl));
}

NK_API struct rawfb_context*
nk_rawfb_init(void *fb, void *tex_mem, const unsigned int w, const unsigned int h,
    const unsigned int pitch, const rawfb_pl pl)
{
    const void *tex;
    struct rawfb_context *rawfb;
    rawfb = malloc(sizeof(struct rawfb_context));
    if (!rawfb)
        return NULL;

    memset(rawfb, 0, sizeof(struct rawfb_context));
    rawfb->font_tex.pixels = tex_mem;
    //rawfb->font_tex.format = NK_FONT_ATLAS_ALPHA8;
    rawfb->font_tex.format = NK_FONT_ATLAS_RGBA32;
    rawfb->font_tex.w = rawfb->font_tex.h = 0;

    rawfb->fb.pixels = fb;
    rawfb->fb.w= w;
    rawfb->fb.h = h;
    rawfb->fb.pl = pl;

    if (pl == PIXEL_LAYOUT_RGBX_8888 || pl == PIXEL_LAYOUT_XRGB_8888) {
    rawfb->fb.format = NK_FONT_ATLAS_RGBA32;
    rawfb->fb.pitch = pitch;
    }
    else {
	perror("nk_rawfb_init(): Unsupported pixel layout.\n");
	free(rawfb);
	return NULL;
    }

    if (0 == nk_init_default(&rawfb->ctx, 0)) {
	free(rawfb);
	return NULL;
    }

    nk_font_atlas_init_default(&rawfb->atlas);
    nk_font_atlas_begin(&rawfb->atlas);
    tex = nk_font_atlas_bake(&rawfb->atlas, &rawfb->font_tex.w, &rawfb->font_tex.h, rawfb->font_tex.format);
    if (!tex) {
	free(rawfb);
	return NULL;
    }

    switch(rawfb->font_tex.format) {
    case NK_FONT_ATLAS_ALPHA8:
        rawfb->font_tex.pitch = rawfb->font_tex.w * 1;
        break;
    case NK_FONT_ATLAS_RGBA32:
        rawfb->font_tex.pitch = rawfb->font_tex.w * 4;
        break;
    };
    /* Store the font texture in tex scratch memory */
    memcpy(rawfb->font_tex.pixels, tex, rawfb->font_tex.pitch * rawfb->font_tex.h);
    nk_font_atlas_end(&rawfb->atlas, nk_handle_ptr(NULL), NULL);
    if (rawfb->atlas.default_font)
        nk_style_set_font(&rawfb->ctx, &rawfb->atlas.default_font->handle);
    nk_style_load_all_cursors(&rawfb->ctx, rawfb->atlas.cursors);
    nk_rawfb_scissor(rawfb, 0, 0, rawfb->fb.w, rawfb->fb.h);
    return rawfb;
}

static void
nk_rawfb_stretch_image(const struct rawfb_image *dst,
    const struct rawfb_image *src, const struct nk_rect *dst_rect,
    const struct nk_rect *src_rect, const struct nk_rect *dst_scissors,
    const struct nk_color *fg)
{
    short i, j;
    static struct nk_color background = { 0x28, 0x28, 0x28, 0xFF}; //FIXME:match background
    struct nk_color *bg = &background;

    if(src_rect->h == dst_rect->h
      && bg != NULL
      && dst->pl == PIXEL_LAYOUT_XRGB_8888
      && dst_scissors
      )
    {
#ifndef DISABLE_BLIT_ALPHA
        int x0 = (short)dst_rect->x;
        int y0 = (short)dst_rect->y;
        int x1 = (short)(dst_rect->x + dst_rect->w);
        int y1 = (short)(dst_rect->y + dst_rect->h);

        int src_stride = (int)(src_rect->w / dst_rect->w);
        x0 = MAX(dst_scissors->x, MIN(dst_scissors->w, x0));
        x1 = MAX(dst_scissors->x, MIN(dst_scissors->w, x1));
        y0 = MAX(dst_scissors->y, MIN(dst_scissors->h, y0));
        y1 = MAX(dst_scissors->y, MIN(dst_scissors->h, y1));

        if(src->format == NK_FONT_ATLAS_ALPHA8)
        {
        	unsigned char *src_alpha = (unsigned char *)src->pixels + (src->pitch * (int)src_rect->y);
        	src_alpha += (int)(src_rect->x + src_stride/2.);
        	nk_internal_blit_alpha_mask(dst->pixels, dst->pitch, src_alpha,
        		x0, y0, x1, y1, src->pitch, src_stride, *fg, background);
        }

        if(src->format == NK_FONT_ATLAS_RGBA32)
        {
#if 1
			unsigned char *src_rgba = (unsigned char *)src->pixels + (src->pitch * (int) src_rect->y);
			src_rgba += sizeof(unsigned)*(int)src_rect->x;
			for (int y = y0; y < y1; y++)
			{
				unsigned char *ptr = src_rgba;
				for (int x = x0; x < x1; x++)
				{
					unsigned pixel = ((unsigned int *)ptr)[0];
					struct nk_color col = nk_rawfb_int2color(pixel, src->pl);
					if (col.r || col.g || col.b)
					{
					    col.r = fg->r;
					    col.g = fg->g;
					    col.b = fg->b;
					}
					nk_rawfb_img_blendpixel(dst, x, y, col);

					ptr += 4*src_stride;
				}
				src_rgba += src->pitch;
			}
#else    
		    unsigned char *src_rgba = (unsigned char *)src->pixels + (src->pitch * (int)src_rect->y);
		    src_rgba += sizeof(unsigned)*(int)(src_rect->x + src_stride/2.);
		    nk_internal_blit(dst->pixels, dst->pitch, src_rgba,
		        x0, y0, x1, y1, src->pitch, src_stride*4, *fg, background);
#endif
		}
#endif
		return;
    }
    
    printf("Non-optimized stretch blit\n");    
    struct nk_color col;
    float xinc = src_rect->w / dst_rect->w;
    float yinc = src_rect->h / dst_rect->h;
    float xoff = src_rect->x, yoff = src_rect->y;


    /* Simple nearest filtering rescaling */
    /* TODO: use bilinear filter */
    for (j = 0; j < (short)dst_rect->h; j++) {
        for (i = 0; i < (short)dst_rect->w; i++) {
            if (dst_scissors) {
                if (i + (int)(dst_rect->x + 0.5f) < dst_scissors->x || i + (int)(dst_rect->x + 0.5f) >= dst_scissors->w)
                    continue;
                if (j + (int)(dst_rect->y + 0.5f) < dst_scissors->y || j + (int)(dst_rect->y + 0.5f) >= dst_scissors->h)
                    continue;
            }
#ifndef DISABLE_STRETCH_IMAGE
            col = nk_rawfb_img_getpixel(src, (int)xoff, (int) yoff);
            if (col.r || col.g || col.b)
            {
                col.r = fg->r;
                col.g = fg->g;
                col.b = fg->b;
            }
            nk_rawfb_img_blendpixel(dst, i + (int)(dst_rect->x + 0.5f), j + (int)(dst_rect->y + 0.5f), col);
#endif
            xoff += xinc;
        }
        xoff = src_rect->x;
        yoff += yinc;
    }
}

static void
nk_rawfb_font_query_font_glyph(nk_handle handle, const float height,
    struct nk_user_font_glyph *glyph, const nk_rune codepoint,
    const nk_rune next_codepoint)
{
    float scale;
    const struct nk_font_glyph *g;
    struct nk_font *font;
    NK_ASSERT(glyph);
    NK_UNUSED(next_codepoint);

    font = (struct nk_font*)handle.ptr;
    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    if (!font || !glyph)
        return;

    scale = height/font->info.height;
    g = nk_font_find_glyph(font, codepoint);
    glyph->width = (g->x1 - g->x0) * scale;
    glyph->height = (g->y1 - g->y0) * scale;
    glyph->offset = nk_vec2(g->x0 * scale, g->y0 * scale);
    glyph->xadvance = (g->xadvance * scale);
    glyph->uv[0] = nk_vec2(g->u0, g->v0);
    glyph->uv[1] = nk_vec2(g->u1, g->v1);
}

NK_API void
nk_rawfb_draw_text(const struct rawfb_context *rawfb,
    const struct nk_user_font *font, const struct nk_rect rect,
    const char *text, const int len, const float font_height,
    const struct nk_color fg)
{
    float x = 0;
    int text_len = 0;
    nk_rune unicode = 0;
    nk_rune next = 0;
    int glyph_len = 0;
    int next_glyph_len = 0;
    struct nk_user_font_glyph g;
    if (!len || !text) return;

    x = 0;
    glyph_len = nk_utf_decode(text, &unicode, len);
    if (!glyph_len) return;

    /* draw every glyph image */
    while (text_len < len && glyph_len) {
        struct nk_rect src_rect;
        struct nk_rect dst_rect;
        float char_width = 0;
        if (unicode == NK_UTF_INVALID) break;

        /* query currently drawn glyph information */
        next_glyph_len = nk_utf_decode(text + text_len + glyph_len, &next, (int)len - text_len);
        nk_rawfb_font_query_font_glyph(font->userdata, font_height, &g, unicode,
                    (next == NK_UTF_INVALID) ? '\0' : next);

        /* calculate and draw glyph drawing rectangle and image */
        char_width = g.xadvance;
        src_rect.x = g.uv[0].x * rawfb->font_tex.w;
        src_rect.y = g.uv[0].y * rawfb->font_tex.h;
        src_rect.w = (g.uv[1].x - g.uv[0].x) * rawfb->font_tex.w;
        src_rect.h = g.uv[1].y * rawfb->font_tex.h - g.uv[0].y * rawfb->font_tex.h;

        dst_rect.x = x + g.offset.x + rect.x;
        dst_rect.y = g.offset.y + rect.y;
        dst_rect.w = ceilf(g.width);
        dst_rect.h = ceilf(g.height);
        
        //corrections for integer ratios of (src/dst) widths
        float factor = (src_rect.w / dst_rect.w) / (int)(src_rect.w / dst_rect.w);
        dst_rect.w *= factor;
        char_width *= factor;

        /* Use software rescaling to blit glyph from font_text to framebuffer */
        nk_rawfb_stretch_image(&rawfb->fb, &rawfb->font_tex, &dst_rect, &src_rect, &rawfb->scissors, &fg);

        /* offset next glyph */
        text_len += glyph_len;
        x += char_width;
        glyph_len = next_glyph_len;
        unicode = next;
    }
}

NK_API void
nk_rawfb_drawimage(const struct rawfb_context *rawfb,
    const int x, const int y, const int w, const int h,
    const struct nk_image *img, const struct nk_color *col)
{
    struct nk_rect src_rect;
    struct nk_rect dst_rect;

    src_rect.x = img->region[0];
    src_rect.y = img->region[1];
    src_rect.w = img->region[2];
    src_rect.h = img->region[3];

    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.w = w;
    dst_rect.h = h;
    nk_rawfb_stretch_image(&rawfb->fb, &rawfb->font_tex, &dst_rect, &src_rect, &rawfb->scissors, col);
}

NK_API void
nk_rawfb_shutdown(struct rawfb_context *rawfb)
{
    if (rawfb) {
	nk_free(&rawfb->ctx);
	memset(rawfb, 0, sizeof(struct rawfb_context));
	free(rawfb);
    }
}

NK_API void
nk_rawfb_resize_fb(struct rawfb_context *rawfb,
                   void *fb,
                   const unsigned int w,
                   const unsigned int h,
                   const unsigned int pitch,
		   const rawfb_pl pl)
{
    rawfb->fb.w = w;
    rawfb->fb.h = h;
    rawfb->fb.pixels = fb;
    rawfb->fb.pitch = pitch;
    rawfb->fb.pl = pl;
}

NK_API void
nk_rawfb_render(const struct rawfb_context *rawfb,
                const struct nk_color clear,
                const unsigned char enable_clear)
{
    const struct nk_command *cmd;
    if (enable_clear)
        nk_rawfb_clear(rawfb, clear);

    nk_foreach(cmd, (struct nk_context*)&rawfb->ctx) {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            nk_rawfb_scissor((struct rawfb_context *)rawfb, s->x, s->y, s->w, s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            nk_rawfb_stroke_line(rawfb, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->line_thickness, l->color);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            nk_rawfb_stroke_rect(rawfb, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->line_thickness, r->color);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            nk_rawfb_fill_rect(rawfb, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            nk_rawfb_stroke_circle(rawfb, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            nk_rawfb_fill_circle(rawfb, c->x, c->y, c->w, c->h, c->color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            nk_rawfb_stroke_triangle(rawfb, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->line_thickness, t->color);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            nk_rawfb_fill_triangle(rawfb, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            nk_rawfb_stroke_polygon(rawfb, p->points, p->point_count, p->line_thickness,p->color);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            nk_rawfb_fill_polygon(rawfb, p->points, p->point_count, p->color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            nk_rawfb_stroke_polyline(rawfb, p->points, p->point_count, p->line_thickness, p->color);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            nk_rawfb_draw_text(rawfb, t->font, nk_rect(t->x, t->y, t->w, t->h),
                t->string, t->length, t->height, t->foreground);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            nk_rawfb_stroke_curve(rawfb, q->begin, q->ctrl[0], q->ctrl[1],
                q->end, 22, q->line_thickness, q->color);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR: {
	    const struct nk_command_rect_multi_color *q = (const struct nk_command_rect_multi_color *)cmd;
	    nk_rawfb_draw_rect_multi_color(rawfb, q->x, q->y, q->w, q->h, q->left, q->top, q->right, q->bottom);
	} break;
        case NK_COMMAND_IMAGE: {
            const struct nk_command_image *q = (const struct nk_command_image *)cmd;
            nk_rawfb_drawimage(rawfb, q->x, q->y, q->w, q->h, &q->img, &q->col);
        } break;
        case NK_COMMAND_ARC: {
        } break;
        case NK_COMMAND_ARC_FILLED: {
        } break;
        default: break;
        }
    } nk_clear((struct nk_context*)&rawfb->ctx);
}
#endif

