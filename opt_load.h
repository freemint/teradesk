/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                                     2003  Dj. Vukovic
 *
 * This file is part of Teradesk.
 *
 * Teradesk is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Teradesk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Teradesk; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

	if ((file = x_fopen(optname, O_DENYW | O_RDONLY, &error)) != NULL)
	{
		opt_n = sizeof(Options);
		tmp.version = 0;
		x_fread(file, &tmp, sizeof(int));

		/* HR 240103: load older cfg versions */

		if (   tmp.version >= MIN_VERSION
		    /* && tmp.version <  CFG_VERSION DjV 050 190503 */
		    && tmp.version <  0x202 /* this field was introduced in 2.02 */
									/* but there are later versions      */
		   )
		{
			memset(&tmp.V2_2, 0, sizeof(tmp.V2_2));
			opt_n -= sizeof(tmp.V2_2);
		}

		x_fclose(file);
	}

	if ((file = x_fopen(optname, O_DENYW | O_RDONLY, &error)) != NULL)
	{
		if ((n = x_fread(file, &tmp, opt_n)) == opt_n)
		{
			if (   tmp.version >= MIN_VERSION		/* DjV 005 120103 (was 0x119) */
			    && tmp.version <= CFG_VERSION
			    && tmp.magic   == MAGIC
			   )
			{
				extern FTYPE *filetypes; /* DjV 047 260403 */
				options = tmp;
				if (opt_n != sizeof(Options))		/* HR 240103 */
				{
					options.V2_2.fields = WD_SHSIZ | WD_SHDAT | WD_SHTIM | WD_SHATT; /* DjV 010 251202 HR 240103 */
					options.attribs = FA_SUBDIR | FA_SYSTEM; /* DjV 004 251202 HR 240103 */
				}
/* must not do this here, because of fixups made later 
				ins_shorts();     /* DjV 019 080103 put kbd shortcuts into menu texts */
*/
				wd_deselect_all();
				wd_default();

				if (tmp.cprefs & SAVE_COLORS)
					error = load_colors(file);
				if (error == 0)
					if ((error = dsk_load(file)) == 0)
						/* if ((error = ft_load(file)) == 0) DjV 047 260403 */
						if ((error = ft_load(file, &filetypes)) == 0) /* DjV 047 260403 */
							if ((error = icnt_load(file)) == 0)
								if ((error = app_load(file)) == 0)
									if ((error = prg_load(file)) == 0)
										error = wd_load(file);
			}
			else
			{
				alert_iprint(MVALIDCF);    /* DjV 035 240703 */
				x_fclose(file);
				return;
			}

			get_set_video(1);	/* DjV 007 030103 If read ok, set video state but do not change resolution */
		}
		else
		{
			error = (n < 0) ? (int) n : EEOF;
			alert_printf(1, ALOADCFG, fn_get_name(optname), get_message(error));
			x_fclose(file);
			return;
		}
		x_fclose(file);
	}

	/* Printer line */

	options.V2_2.plinelen = 80;

	/* Keyboard shortcuts have been changed a lot; some must be explicitely fixed now! */
	{
		int ik, ts[64];
		int *ks = &options.V2_2.kbshort[0];

		/* Copy old shortcuts back into a temporary buffer */

		memcpy( &ts, ks, (NITEM+2)*sizeof(int));

		/* Now set explicitely; first- part of File menu */

		ks[2] = ts[4];
		ks[3] = 0;
		ks[4] = ts[2];

		ks[10] = ts[11];
		ks[11] = ts[10];

		/* (View menu is completely compatible) */

		/* Part of Options menu */

		ks[35] = ts[43];
		ks[36] = 0;
		ks[37] = ts[35];
		ks[38] = ts[36];
		ks[39] = ts[38];
		ks[40] = 0;
		ks[41] = ts[40];
		ks[43] = ts[44];
		ks[44] = ts[45];
		ks[45] = 0;
		ks[46] = ks[47];
		ks[47] = ks[48];
		ks[48] == ks[49];
		ks[49] = 0;
		ks[50] = 0;
 
		ins_shorts();

	}


