/*
 * Copyright © 2011 Saleem Abdulrasool <compnerd@compnerd.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#include <eds/edid.h>
#include <eds/hdmi.h>
#include <eds/cea861.h>

static void disp_edid1(const struct edid *const edid)
{
//	const struct edid_monitor_range_limits *monitor_range_limits = NULL;
	edid_monitor_descriptor_string monitor_serial_number = {0};
	edid_monitor_descriptor_string monitor_model_name = {0};
	bool has_ascii_string = false;
	char manufacturer[4] = {0};

//	struct edid_color_characteristics_data characteristics;
	uint8_t i;

	edid_manufacturer(edid, manufacturer);
//	characteristics = edid_color_characteristics(edid);

	for (i = 0; i < ARRAY_SIZE(edid->detailed_timings); i++)
	{
		const struct edid_monitor_descriptor * const mon = &edid->detailed_timings[i].monitor;

		if (!edid_detailed_timing_is_monitor_descriptor(edid, i))
		{
        	continue;
        }
		switch (mon->tag)
		{
			case EDID_MONTIOR_DESCRIPTOR_MANUFACTURER_DEFINED:
			{
            	/* This is arbitrary data, just silently ignore it. */
				break;
            }
			case EDID_MONITOR_DESCRIPTOR_ASCII_STRING:
            {
				has_ascii_string = true;
				break;
            }
			case EDID_MONITOR_DESCRIPTOR_MONITOR_NAME:
            {
				strncpy(monitor_model_name, (char *) mon->data, sizeof(monitor_model_name) - 1);
				*strchrnul(monitor_model_name, '\n') = '\0';
				break;
            }
#if 0
			case EDID_MONITOR_DESCRIPTOR_MONITOR_RANGE_LIMITS:
			{
				monitor_range_limits = (struct edid_monitor_range_limits *) &mon->data;
				break;
			}
#endif
			case EDID_MONITOR_DESCRIPTOR_MONITOR_SERIAL_NUMBER:
			{
				strncpy(monitor_serial_number, (char *) mon->data, sizeof(monitor_serial_number) - 1);
				*strchrnul(monitor_serial_number, '\n') = '\0';
				break;
			}
			default:
			{
				fprintf(stderr, "unknown monitor descriptor type 0x%02x\n", mon->tag);
				break;
			}
		}
	}
	printf("Monitor\n");
	printf("  Model name............... %s\n", *monitor_model_name ? monitor_model_name : "n/a");
	printf("  Manufacturer............. %s\n", manufacturer);

	if (has_ascii_string)
	{
		edid_monitor_descriptor_string string = {0};
		printf("General purpose ASCII string\n");

		for (i = 0; i < ARRAY_SIZE(edid->detailed_timings); i++)
		{
			const struct edid_monitor_descriptor * const mon = &edid->detailed_timings[i].monitor;

			if (!edid_detailed_timing_is_monitor_descriptor(edid, i))
			{
				continue;
			}
			if (mon->tag == EDID_MONITOR_DESCRIPTOR_ASCII_STRING)
			{
				strncpy(string, (char *) mon->data, sizeof(string) - 1);
				*strchrnul(string, '\n') = '\0';
				printf("  ASCII string............. %s\n", string);
			}
		}
		printf("\n");
	}
}

static inline void disp_cea861_video_data(const struct cea861_video_data_block * const vdb)
{
	printf("CE video identifiers (VICs) - supported resolutions\n");
	for (uint8_t i = 0; i < vdb->header.length; i++)
	{
		switch(vdb->svd[i].video_identification_code)
		{
			case 6:
			case 7:
			{
				printf ("VIDEO_STD_NTSC\n");
				break;
			}
			case 21:
			case 22:
			{
				printf ("VIDEO_STD_PAL\n");
				break;
			}
			case 2:
			case 3:
			{
				printf ("VIDEO_STD_480P\n");
				break;
			}
			case 17:
			case 18:
			{
				printf ("VIDEO_STD_576P\n");
				break;
			}
			case 4:
			{
				printf ("VIDEO_STD_720P60\n");
				break;
			}
			case 5:
			case 46:
			{
				printf ("VIDEO_STD_1080I60\n");
				break;
			}
			case 19:
			{
				printf ("VIDEO_STD_720P50\n");
				break;
			}
			case 40:
			{
				printf ("VIDEO_STD_1080I50\n");
				break;
			}
			case 34:
			{
				printf ("VIDEO_STD_1080P30\n");
				break;
			}
			case 32:
			{
				printf ("VIDEO_STD_1080P24\n");
				break;
			}
			case 33:
			{
				printf ("VIDEO_STD_1080P25\n");
				break;
			}
			case 31:
			{
				printf ("VIDEO_STD_1080P50\n");
				break;
			}
			case 16:
			{
				printf ("VIDEO_STD_1080P60\n");
				break;
			}
			case 93:
			case 98:
			{
				printf ("VIDEO_STD_2160P24\n");
				break;
			}
			case 94:
			case 99:
			{
				printf ("VIDEO_STD_2160P25\n");
				break;
			}
			case 95:
			case 100:
			{
				printf ("VIDEO_STD_2160P30\n");
				break;
			}
			case 96:
			case 101:
			{
				printf ("VIDEO_STD_2160P50\n");
				break;
			}
			default:
			{
				printf ("unknown / not supported mode: %d\n", vdb->svd[i].video_identification_code);
			}
		}

	}
	printf("\n");
}

static void disp_cea861(const struct edid_extension *const ext)
{
	const struct cea861_timing_block * const ctb = (struct cea861_timing_block *) ext;
	const uint8_t offset = offsetof(struct cea861_timing_block, data);
	uint8_t index = 0;

	printf("\n");

	if (ctb->revision >= 3)
	{
		do
		{
			const struct cea861_data_block_header *const header = (struct cea861_data_block_header *) &ctb->data[index];

			if (header->tag == CEA861_DATA_BLOCK_TYPE_VIDEO)
			{
				const struct cea861_video_data_block *const db = (struct cea861_video_data_block *) header;

				disp_cea861_video_data(db);
			}
			index = index + header->length + sizeof(*header);
		}
		while (index < ctb->dtd_offset - offset);
	}
}

/* parse edid routines */
static const struct edid_extension_handler
{
	void (*const inf_disp)(const struct edid_extension *const);
} edid_extension_handlers[] =
{
	[EDID_EXTENSION_CEA] = { disp_cea861 },
};

static void parse_edid(const uint8_t *const data)
{
	const struct edid *const edid = (struct edid *)data;
	const struct edid_extension *const extensions = (struct edid_extension *)(data + sizeof(*edid));

	disp_edid1(edid);

	for (uint8_t i = 0; i < edid->extensions; i++)
	{
		const struct edid_extension *const extension = &extensions[i];
		const struct edid_extension_handler *const handler = &edid_extension_handlers[extension->tag];

		if (!handler)
		{
			fprintf(stderr, "WARNING: block %u contains unknown extension (%#04x)\n", i, extensions[i].tag);
			continue;
		}
		if (handler->inf_disp)
		{
			(*handler->inf_disp)(extension);
		}
	}
}

unsigned char ahex2bin(unsigned char MSB, unsigned char LSB)
{
	if (MSB > '9')
	{
		MSB -= 7;
	}
	if (LSB > '9')
	{
		LSB -= 7;
	}
	return (MSB << 4) | (LSB & 0x0F);
}

extern int i2cmain(int bus, int quiet, uint8_t *buffer);

int main()
{
	uint8_t *buffer = NULL;
	unsigned char buf[2];
	int rv = EXIT_FAILURE;
	FILE *edid = NULL;
	int length = 0;

	buffer = calloc(256, 1);

	if (i2cmain(-1,1,buffer) == 0)
	{
		parse_edid(buffer);
		rv = EXIT_SUCCESS;
	}
	else
	{
		if ((edid = fopen("/sys/devices/virtual/stmcoredisplay/display0/hdmi0.0/edid", "r")) == NULL)
		{
			fprintf(stderr, "unable to read EDID data: %m\n");
			goto out;
		}
		while ((fread(buf, 1, 3, edid) == 3) && (length < 256))
		{
			if (buf[0] == 0x0a)
			{
				long pos = ftell(edid)-2;
				fseek(edid,pos,SEEK_SET);
				continue;
			}
			buffer[length] = ahex2bin(buf[0],buf[1]);
			length++;
		}
		parse_edid(buffer);
		rv = EXIT_SUCCESS;
	}

out:
	if (edid)
	{
		fclose(edid);
	}
	free(buffer);
	return rv;
}
// vim"ts=4
