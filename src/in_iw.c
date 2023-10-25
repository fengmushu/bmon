/*
 * in_netlink.c            Netlink input
 *
 * Copyright (c) 2001-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <bmon/bmon.h>
#include <bmon/input.h>
#include <bmon/element.h>
#include <bmon/attr.h>
#include <bmon/conf.h>
#include <bmon/input.h>
#include <bmon/utils.h>

#include <iwlib.h>
int iw_skfd = -1;

int iw_open(void)
{
	return iw_sockets_open();
}

void iw_close(void)
{
	close(iw_skfd);
}

void handle_iw(struct element *e, struct rtnl_link *link)
{
	const char *ifname = rtnl_link_get_name(link);
	struct wireless_info	info;

	memset((char *) &info, 0, sizeof(struct wireless_info));

	/* Get stats */
	if(iw_get_stats(iw_skfd, ifname, &(info.stats), &info.range, info.has_range) >= 0) {
		char rssi[32] = "-110", linkq[32]="98", noise[32] = "-99";
		const iwqual *qual = &info.stats.qual;

		// printf("updated %d, link %d, qual %d, noise %d\n", qual->updated, qual->qual, qual->level, qual->noise);
		if(qual->level != 0) {
			snprintf(linkq, sizeof(linkq), "%d", qual->qual);
			if(qual->updated & IW_QUAL_RCPI) { /* RCPI */
				/* RCPI = int{(Power in dBm +110)*2} for 0dbm > Power > -110dBm */
				if(!(qual->updated & IW_QUAL_LEVEL_INVALID)) {
					double	rcpilevel = (qual->level / 2.0) - 110.0;
					snprintf(rssi, sizeof(rssi), "%g", rcpilevel);
				}
			} else { /* dBm */
				if(qual->updated & IW_QUAL_LEVEL_UPDATED) {
					if(!(qual->updated & IW_QUAL_LEVEL_INVALID)) {
						int	dblevel = qual->level;
						int 	dbNoise = qual->noise;
						/* Implement a range for dBm [-192; 63] */
						if(qual->level >= 64)
							dblevel -= 0x100;
						if(qual->noise >= 64)
							dbNoise -=  0x100;
						snprintf(rssi, sizeof(rssi), "%d", dblevel);
						snprintf(noise, sizeof(noise), "%d", dbNoise);
					}
				} else { /* relative */
					if(!(qual->updated & IW_QUAL_LEVEL_INVALID)) {
						snprintf(rssi, sizeof(rssi), "%d", qual->level);
						snprintf(noise, sizeof(noise), "%d", qual->noise);
					}
				}
			}
		}

		element_update_info(e, "rssi", rssi);
		element_update_info(e, "linkq", linkq);
		element_update_info(e, "nosie", noise);
	}
}