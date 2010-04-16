/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008 WiredTiger Software.
 *	All rights reserved.
 *
 * $Id$
 */

#include "wt_internal.h"

static void __wt_bt_desc_stats(DB *, WT_PAGE_DESC *);

/*
 * __wt_bt_desc_stats --
 *	Fill in the statistics from the database description.
 */
static void
__wt_bt_desc_stats(DB *db, WT_PAGE_DESC *desc)
{
	WT_STATS *stats;

	stats = db->idb->dstats;

	WT_STAT_SET(stats, MAGIC, desc->magic);
	WT_STAT_SET(stats, MAJOR, desc->majorv);
	WT_STAT_SET(stats, MINOR, desc->minorv);
	WT_STAT_SET(stats, INTLMAX, desc->intlmax);
	WT_STAT_SET(stats, INTLMIN, desc->intlmin);
	WT_STAT_SET(stats, LEAFMAX, desc->leafmax);
	WT_STAT_SET(stats, LEAFMIN, desc->leafmin);
	WT_STAT_SET(stats, BASE_RECNO, desc->base_recno);
	WT_STAT_SET(stats, FIXED_LEN, desc->fixed_len);
}

/*
 * __wt_bt_desc_read --
 *	Read the descriptor structure from page 0.
 */
int
__wt_bt_desc_read(WT_TOC *toc)
{
	DB *db;
	WT_PAGE *page;
	WT_PAGE_DESC *desc;

	db = toc->db;

	/* Read the database's description page. */
	WT_RET(__wt_bt_page_in(toc, 0, 512, 0, &page));
	desc = (WT_PAGE_DESC *)WT_PAGE_BYTE(page);

	db->intlmax = desc->intlmax;		/* Update DB handle */
	db->intlmin = desc->intlmin;
	db->leafmax = desc->leafmax;
	db->leafmin = desc->leafmin;
	db->idb->root_addr = desc->root_addr;
	db->idb->root_size = desc->root_size;
	db->idb->free_addr = desc->free_addr;
	db->idb->free_size = desc->free_size;
	db->fixed_len = desc->fixed_len;

	__wt_bt_desc_stats(db, desc);		/* Update statistics. */

	__wt_bt_page_out(toc, &page, 0);
	return (0);
}

/*
 * __wt_bt_desc_write --
 *	Update the root addr.
 */
int
__wt_bt_desc_write(WT_TOC *toc)
{
	DB *db;
	IDB *idb;
	WT_FH *fh;
	WT_PAGE *page;
	WT_PAGE_DESC *desc;

	db = toc->db;
	idb = db->idb;
	fh = idb->fh;

	/* If the file size is 0, allocate a new page. */
	if (fh->file_size == 0)
		WT_RET(__wt_bt_page_alloc(toc, WT_PAGE_DESCRIPT, 512, &page));
	else
		WT_RET(__wt_bt_page_in(toc, 0, 512, 0, &page));

	desc = (WT_PAGE_DESC *)WT_PAGE_BYTE(page);
	desc->magic = WT_BTREE_MAGIC;
	desc->majorv = WT_BTREE_MAJOR_VERSION;
	desc->minorv = WT_BTREE_MINOR_VERSION;
	desc->intlmax = db->intlmax;
	desc->intlmin = db->intlmin;
	desc->leafmax = db->leafmax;
	desc->leafmin = db->leafmin;
	desc->base_recno = 0;
	desc->root_addr = idb->root_addr;
	desc->root_size = idb->root_size;
	desc->free_addr = idb->free_addr;
	desc->free_size = idb->free_size;
	desc->fixed_len = (u_int8_t)db->fixed_len;
	desc->flags = 0;
	if (F_ISSET(idb, WT_REPEAT_COMP))
		F_SET(desc, WT_PAGE_DESC_REPEAT);

	/*
	 * Update the page.
	 *
	 * XXX
	 * I'm not currently serializing updates to this page, and I'm pretty
	 * sure that's a bug -- review this when we start working btree splits.
	 */
	WT_PAGE_MODIFY(page);
	__wt_bt_page_out(toc, &page, 0);

	__wt_bt_desc_stats(db, desc);			/* Update statistics */

	return (0);
}
