#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libs.h"
#include "log.h"
#include "db.h"
#include "dep.h"

/* parser detection */
int find_parser_expat(const char *name, int logdepth, int fatal);
