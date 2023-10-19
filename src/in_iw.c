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
	int rc;
	const char *ifname = rtnl_link_get_name(link);
	struct wireless_info	info;
	struct iwreq		wrq;

	memset((char *) &info, 0, sizeof(struct wireless_info));

	// rc = iw_get_basic_config(iw_skfd, ifname, &info.b);
	// if (rc < 0) {
	// 	struct ifreq ifr;

	// 	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	// 	if (ioctl(iw_skfd, SIOCGIFFLAGS, &ifr) < 0) {
	//		fprintf(stderr, "%s, iw dev not found\n", ifname);
	// 	} else {
	//		fprintf(stderr, "%s, iw not supported\n", ifname);
	// 	}
	// }
	/* Get ranges */
	if(iw_get_range_info(iw_skfd, ifname, &(info.range)) >= 0)
		info.has_range = 1;

	/* Get stats */
	if(iw_get_stats(iw_skfd, ifname, &(info.stats), &info.range, info.has_range) >= 0) {
		char rssi[32] = "-110";
		const iwqual *qual = &info.stats.qual;
		const iwrange *range = &info.range;

		info.has_stats = 1;
		if(info.has_range && ((qual->level != 0)
			|| (qual->updated & (IW_QUAL_DBM | IW_QUAL_RCPI)))) {
			if(qual->updated & IW_QUAL_RCPI) { /* RCPI */
				/* RCPI = int{(Power in dBm +110)*2} for 0dbm > Power > -110dBm */
				if(!(qual->updated & IW_QUAL_LEVEL_INVALID)) {
					double	rcpilevel = (qual->level / 2.0) - 110.0;
					snprintf(rssi, sizeof(rssi), "%g", rcpilevel);
				}
			} else { /* dBm */
				if((qual->updated & IW_QUAL_DBM)
					|| (qual->level > range->max_qual.level)) {
					if(!(qual->updated & IW_QUAL_LEVEL_INVALID)) {
						int	dblevel = qual->level;
						/* Implement a range for dBm [-192; 63] */
						if(qual->level >= 64)
							dblevel -= 0x100;
						snprintf(rssi, sizeof(rssi), "%d", dblevel);
					}
				} else { /* relative */
					if(!(qual->updated & IW_QUAL_LEVEL_INVALID)) {
						snprintf(rssi, sizeof(rssi), "%d/%d", qual->level, range->max_qual.level);
					}
				}
			}
		}

		element_update_info(e, "rssi", rssi);
	}
}