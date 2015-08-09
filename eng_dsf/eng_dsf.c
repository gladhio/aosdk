//
// Audio Overload
// Emulated music player
//
// (C) 2000-2008 Richard F. Bannister
//

//
// eng_dsf.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ao.h"
#include "eng_protos.h"
#include "corlett.h"
#include "dc_hw.h"
#include "aica.h"

#define DK_CORE	(1)

#if DK_CORE
#include "arm7.h"
#else
#include "arm7core.h"
#endif

static corlett_t	c = {0};

int dsf_lib(int libnum, uint8 *lib, uint64 size, corlett_t *c)
{
	// patch the file into ram
	uint32 offset = lib[0] | lib[1]<<8 | lib[2]<<16 | lib[3]<<24;
	memcpy(&dc_ram[offset], lib+4, size-4);

	return AO_SUCCESS;
}

int32 dsf_start(uint8 *buffer, uint32 length)
{
	uint32 lengthMS, fadeMS;

	// clear Dreamcast work RAM before we start scribbling in it
	memset(dc_ram, 0, 8*1024*1024);

	// Decode the current SSF
	if (corlett_decode(buffer, length, &c, dsf_lib) != AO_SUCCESS)
	{
		return AO_FAIL;
	}

	#ifdef DEBUG
	{
		FILE *f;

		f = fopen("dcram.bin", "wb");
		fwrite(dc_ram, 2*1024*1024, 1, f);
		fclose(f);
	}
	#endif

	#if DK_CORE
	ARM7_Init();
	#else
	arm7_init(0, 45000000, NULL, NULL);
	arm7_reset();
	#endif
	dc_hw_init();

	// now figure out the time in samples for the length/fade
	lengthMS = psfTimeToMS(c.inf_length);
	fadeMS = psfTimeToMS(c.inf_fade);

	corlett_length_set(lengthMS, fadeMS);
	return AO_SUCCESS;
}

int32 dsf_sample(stereo_sample_t *sample)
{
	#if DK_CORE
	ARM7_Execute((33000000 / 60 / 4) / 735);
	#else
	arm7_execute((33000000 / 60 / 4) / 735);
	#endif
	AICA_Update(NULL, NULL, sample);
	corlett_sample_fade(sample);

	return AO_SUCCESS;
}

int32 dsf_frame(void)
{
	return AO_SUCCESS;
}

int32 dsf_stop(void)
{
	return AO_SUCCESS;
}

int32 dsf_command(int32 command, int32 parameter)
{
	switch (command)
	{
		case COMMAND_RESTART:
			return AO_SUCCESS;

	}
	return AO_FAIL;
}

int32 dsf_fill_info(ao_display_info *info)
{
	info->title[1] = "Name: ";
	info->info[1] = c.inf_title;

	info->title[2] = "Game: ";
	info->info[2] = c.inf_game;

	info->title[3] = "Artist: ";
	info->info[3] = c.inf_artist;

	info->title[4] = "Copyright: ";
	info->info[4] = c.inf_copy;

	info->title[5] = "Year: ";
	info->info[5] = c.inf_year;

	info->title[6] = "Length: ";
	info->info[6] = c.inf_length;

	info->title[7] = "Fade: ";
	info->info[7] = c.inf_fade;

	return AO_SUCCESS;
}
