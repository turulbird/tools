#include "current.h"
#include "gfx.h"
#include "resize.h"
#include "pngw.h"
#include "fb_display.h"

extern const char NOMEM[];

#if defined(HAVE_SPARK_HARDWARE) || defined(HAVE_DUCKBOX_HARDWARE)
void FillRect(int _sx, int _sy, int _dx, int _dy, uint32_t color)
{
//	if (_dx <= 0 || _dy <= 0)
//		return;

	sync_blitter = 1;

	STMFBIO_BLT_DATA bltData;
	memset(&bltData, 0, sizeof(STMFBIO_BLT_DATA));

	bltData.operation  = BLT_OP_FILL;
	bltData.dstOffset  = 1920 * 1080 * 4;
	bltData.dstPitch   = DEFAULT_XRES * 4;

	bltData.dst_left   = _sx;
	bltData.dst_top    = _sy;
	bltData.dst_right  = _sx + _dx - 1;
	bltData.dst_bottom = _sy + _dy - 1;

	bltData.dstFormat  = SURF_ARGB8888;
	bltData.srcFormat  = SURF_ARGB8888;
	bltData.dstMemBase = STMFBGP_FRAMEBUFFER;
	bltData.srcMemBase = STMFBGP_FRAMEBUFFER;
	bltData.colour     = color;

	if (ioctl(fb, STMFBIO_BLT, &bltData ) < 0)
		perror("RenderBox ioctl STMFBIO_BLT");
	sync_blitter = 1;
}
#endif

void RenderBox(int _sx, int _sy, int _ex, int _ey, int rad, int col)
{
	if (_ex <= 0 || _ey <= 0)
	{
		return;
	}
	int F, R = rad, ssx = startx+_sx,ssy = starty + _sy,dxx =_ex-_sx, dyy =_ey - _sy, rx, ry, wx, wy;
#if !defined(HAVE_SPARK_HARDWARE) && !defined(HAVE_DUCKBOX_HARDWARE)
	int	count;
#endif

	uint32_t *pos = lbb + ssx + stride * ssy;
	uint32_t *pos0, *pos1, *pos2, *pos3, *i;
	uint32_t pix = bgra[col];

	if (dxx<0) 
	{
		printf("%s RenderBox called with dx < 0 (%d)\n", __plugin__, dxx);
		dxx=0;
	}

	int dyy_max = var_screeninfo.yres;
	if (ssy + dyy > dyy_max)
	{
		printf("[%s] %s called with height = %d (max. %d)\n", __plugin__, __func__, ssy + dyy, dyy_max);
		dyy = dyy_max - ssy;
	}

	if(R)
	{
#if defined(HAVE_SPARK_HARDWARE) || defined(HAVE_DUCKBOX_HARDWARE)
		if(sync_blitter) {
			sync_blitter = 0;
			if (ioctl(fb, STMFBIO_SYNC_BLITTER) < 0)
				perror("RenderBox ioctl STMFBIO_SYNC_BLITTER");
		}
#endif
		if(--dyy<=0)
		{
			dyy=1;
		}

		if(R==1 || R>(dxx/2) || R>(dyy/2))
		{
			R=dxx/10;
			F=dyy/10;	
			if(R>F)
			{
				if(R>(dyy/3))
				{
					R=dyy/3;
				}
			}
			else
			{
				R=F;
				if(R>(dxx/3))
				{
					R=dxx/3;
				}
			}
		}
		ssx=0;
		ssy=R;
		F=1-R;

		rx=R-ssx;
		ry=R-ssy;

		pos0=pos+(dyy-ry)*stride;
		pos1=pos+ry*stride;
		pos2=pos+rx*stride;
		pos3=pos+(dyy-rx)*stride;

		while (ssx <= ssy)
		{
			rx=R-ssx;
			ry=R-ssy;
			wx=rx<<1;
			wy=ry<<1;

			for(i=pos0+rx; i<pos0+rx+dxx-wx;i++)
				*i = pix;
			for(i=pos1+rx; i<pos1+rx+dxx-wx;i++)
				*i = pix;
			for(i=pos2+ry; i<pos2+ry+dxx-wy;i++)
				*i = pix;
			for(i=pos3+ry; i<pos3+ry+dxx-wy;i++)
				*i = pix;

			ssx++;

			pos2-=stride;
			pos3+=stride;

			if (F<0)
			{
				F+=(ssx<<1)-1;
			}
			else   
			{ 
				F+=((ssx-ssy)<<1);
				ssy--;

				pos0-=stride;
				pos1+=stride;
			}
		}
		pos+=R*stride;
	}

#if defined(HAVE_SPARK_HARDWARE) || defined(HAVE_DUCKBOX_HARDWARE)
	FillRect(startx + _sx, starty + _sy + R, dxx + 1, dyy - 2 * R + 1, pix);
#else
	for (count=R; count<(dyy-R); count++)
	{
		for(i=pos; i<pos+dxx;i++)
			*i = pix;
		pos+=stride;
	}
#endif
}

/******************************************************************************
 * PaintIcon
 ******************************************************************************/

int paintIcon(const char *const fname, int xstart, int ystart, int xsize, int ysize, int *iw, int *ih)
{
FILE *tfh;
int x1, y1, rv=-1, alpha=0, bpp=0;

int imx,imy,dxo,dyo,dxp,dyp;
unsigned char *buffer=NULL;

	if((tfh=fopen(fname,"r"))!=NULL)
	{
		if(png_getsize(fname, &x1, &y1))
		{
			perror(__plugin__ " <invalid PNG-Format>\n");
			fclose(tfh);
			return -1;
		}
		// no resize
		if (xsize == 0 || ysize ==0)
		{
			xsize = x1;
			ysize = y1;
		}
		if((buffer=(unsigned char *) malloc(x1*y1*4))==NULL)
		{
			printf(NOMEM);
			fclose(tfh);
			return -1;
		}

		if(!(rv=png_load(fname, &buffer, &x1, &y1, &bpp)))
		{
			alpha=(bpp==4)?1:0;
			scale_pic(&buffer,x1,y1,xstart,ystart,xsize,ysize,&imx,&imy,&dxp,&dyp,&dxo,&dyo,alpha);

			fb_display(buffer, imx, imy, dxp, dyp, dxo, dyo, 0, alpha);
		}
		free(buffer);
		fclose(tfh);
	}
	*iw=imx;
	*ih=imy;
	return (rv)?-1:0;
}

void scale_pic(unsigned char **buffer, int x1, int y1, int xstart, int ystart, int xsize, int ysize,
			   int *imx, int *imy, int *dxp, int *dyp, int *dxo, int *dyo, int alpha)
{
	float xfact=0, yfact=0;
	int txsize=0, tysize=0;
	int txstart =xstart, tystart= ystart;
	
	if (xsize > (ex-xstart)) txsize= (ex-xstart);
	else  txsize= xsize; 
	if (ysize > (ey-ystart)) tysize= (ey-ystart);
	else tysize=ysize;
	xfact= 1000*txsize/x1;
	xfact= xfact/1000;
	yfact= 1000*tysize/y1;
	yfact= yfact/1000;
	
	if ( xfact <= yfact)
	{
		*imx=(int)x1*xfact;
		*imy=(int)y1*xfact;
	}
	else
	{
		*imx=(int)x1*yfact;
		*imy=(int)y1*yfact;
	}
	if ((x1 != *imx) || (y1 != *imy))
		*buffer=color_average_resize(*buffer,x1,y1,*imx,*imy,alpha);

	*dxp=0;
	*dyp=0;
	*dxo=txstart;
	*dyo=tystart;
}
