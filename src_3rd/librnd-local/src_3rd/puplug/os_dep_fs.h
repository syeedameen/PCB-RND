extern char pup_path_sep;

/* === directory listing === */

/* open a directory for listing files */
void *pup_open_dir(const char *dirname);

/* read next file name from an open directory; handle is as returned
   by pup_open_dir(); returns NULL if no more entries to be read */
const char *pup_read_dir(void *handle);

/* close an open directory; handle is as returned by pup_open_dir() */
void pup_close_dir(void *handle);

