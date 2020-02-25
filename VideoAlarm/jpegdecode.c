#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <setjmp.h>
#include "VideoAlarm.h"
#include "common/common.h"
#include "jpeg-6b/jpeglib.h"

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
  	jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;
METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;


  	(*cinfo->err->output_message) (cinfo);


 	longjmp(myerr->setjmp_buffer, 1);
}
int ReadJpegBuf(char * dataSrc,int iSize, VA_BITMAP *bitmap)
{
	struct jpeg_decompress_struct cinfo;
  	struct my_error_mgr jerr;
  	int return_value = 0;

  	JSAMPARRAY buffer;		/* Output row buffer */
  	int row_stride;		/* physical row width in output buffer */
  	cinfo.err = jpeg_std_error(&jerr.pub);
  	jerr.pub.error_exit = my_error_exit;
  	if (setjmp(jerr.setjmp_buffer)) {
   	 	jpeg_destroy_decompress(&cinfo);
    		return -1;//修正白屏报错，白屏后返回-1
  	}
  	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo,dataSrc,iSize);
   	 (void) jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space=JCS_GRAYSCALE;
 	(void) jpeg_start_decompress(&cinfo);

  	row_stride = cinfo.output_width * cinfo.output_components;
 	buffer = (*cinfo.mem->alloc_sarray)
	((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	bitmap->height = cinfo.output_height;
	bitmap->width = cinfo.output_width;
// 	bitmap->buf = (unsigned char *)malloc(bitmap->height * bitmap->width * 3);
	if(NULL == bitmap->buf)
		return -1;
 	while (cinfo.output_scanline < cinfo.output_height)
	{
 		int retval = jpeg_read_scanlines(&cinfo, buffer, 1);
 		if(cinfo.err->msg_code != 105){
 			return_value = -1;//检测花屏，如果出现花屏返回-1
// 			break;
 		}
		memcpy(bitmap->buf + cinfo.output_scanline * row_stride, buffer[0], row_stride);
	}
	(void) jpeg_finish_decompress(&cinfo);
  	jpeg_destroy_decompress(&cinfo);

  	return return_value;
}

