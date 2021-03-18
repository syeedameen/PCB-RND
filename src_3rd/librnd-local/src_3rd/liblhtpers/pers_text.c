/*
    lhtpers - persistent lihata formatting
    Copyright (C) 2016  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project URLs: http://repo.hu/projects/lihata
                  svn://repo.hu/lihata


   Handle styled output for text nodes.
*/


/* called for each on-disk node that is a text or symlink, before printing the node */
static pre_fin_ctrl_t pers_event_te_pre(lht_pers_t *p, lht_perstyle_t *sp, lht_node_t *indoc_node, inst_t *parent_st)
{
	lhtpers_ev_res_t r;

	ilprintf(p->outf, "#text '%s' indoc='%s'\n", sp->name, indoc_node == NULL ? "<no node>" : indoc_node->name);

	/* check if the values are equivalent and replace the on-disk variant if they are not */
	if (p->events->text == NULL) {
		/* default method: case sensitive string comparison */
		if (strcmp(sp->text_data, indoc_node->data.text.value) == 0)
			r = LHTPERS_DISK;
		else
			r = LHTPERS_MEM;
	}
	else if (indoc_node != NULL)
		r = p->events->text(p->events->ev_ctx, sp, indoc_node, sp->text_data);
	else
		r = LHTPERS_DISK; /* no in-memory doc */

	switch(r) {
		case LHTPERS_DISK:
			return PF_PROCEED;
		case LHTPERS_MEM:
			free(sp->text_data);
			sp->text_data = lht_strdup(indoc_node->data.text.value);
			break;
		case LHTPERS_INHIBIT:
			return PF_INHIBIT;
	}

	/* can get here only if the user call messed up the return value */
	return PF_PROCEED;
}
