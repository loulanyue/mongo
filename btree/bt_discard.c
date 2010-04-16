/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008 WiredTiger Software.
 *	All rights reserved.
 *
 * $Id$
 */

#include "wt_internal.h"

static void __wt_bt_page_discard_repl(ENV *, WT_REPL *);

/*
 * __wt_bt_page_reconcile --
 *	Move a page from its in-memory state out to disk.
 */
int
__wt_bt_page_reconcile(DB *db, WT_PAGE *page)
{
	return (__wt_page_write(db, page));
}

/*
 * __wt_bt_page_discard --
 *	Free all memory associated with a page.
 */
void
__wt_bt_page_discard(ENV *env, WT_PAGE *page)
{
	WT_COL_INDX *cip;
	WT_ROW_INDX *rip;
	u_int32_t i;
	void *bp, *ep;

	WT_ENV_FCHK_ASSERT(
	    env, "__wt_bt_page_discard", page->flags, WT_APIMASK_WT_PAGE);

	switch (page->hdr->type) {
	case WT_PAGE_COL_FIX:
	case WT_PAGE_COL_INT:
	case WT_PAGE_COL_VAR:
		WT_INDX_FOREACH(page, cip, i)
			if (cip->repl != NULL)
				__wt_bt_page_discard_repl(env, cip->repl);
		break;
	case WT_PAGE_DUP_INT:
	case WT_PAGE_DUP_LEAF:
	case WT_PAGE_ROW_INT:
	case WT_PAGE_ROW_LEAF:
		bp = (u_int8_t *)page->hdr;
		ep = (u_int8_t *)bp + page->size;
		WT_INDX_FOREACH(page, rip, i) {
			/*
			 * For each entry, see if the data was an allocation,
			 * that is, if it points somewhere other than the
			 * original page.  If it's an allocation, free it.
			 *
			 * For each entry, see if replacements were made -- if
			 * so, free them.
			 */
			if (rip->data < bp || rip->data >= ep)
				__wt_free(env, rip->data, rip->size);
			if (rip->repl != NULL)
				__wt_bt_page_discard_repl(env, rip->repl);
		}
		break;
	case WT_PAGE_DESCRIPT:
	case WT_PAGE_OVFL:
	default:
		break;
	}

	if (page->u.indx != NULL)
		__wt_free(env, page->u.indx, 0);

	__wt_free(env, page->hdr, page->size);
	__wt_free(env, page, sizeof(WT_PAGE));
}

/*
 * __wt_bt_page_discard_repl --
 *	Discard the replacement array.
 */
static void
__wt_bt_page_discard_repl(ENV *env, WT_REPL *repl)
{
	u_int16_t i;

	/* Free the data pointers and then the WT_REPL structure itself. */
	for (i = 0; i < repl->repl_next; ++i)
		if (repl->data[i].data != WT_DATA_DELETED)
			__wt_free(env, repl->data[i].data, repl->data[i].size);
	__wt_free(env, repl->data, repl->repl_size * sizeof(WT_SDBT));
	__wt_free(env, repl, sizeof(WT_REPL));
}
