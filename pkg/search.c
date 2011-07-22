#include <sys/param.h>

#include <err.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <libutil.h>
#include <sysexits.h>

#include <pkg.h>

#include "search.h"

void
usage_search(void)
{
	fprintf(stderr, "usage: pkg search [-gxXcd] pattern\n\n");
	fprintf(stderr, "For more information see 'pkg help search'.\n");
}

int
exec_search(int argc, char **argv)
{
	char *pattern = NULL;
	match_t match = MATCH_EXACT;
	int  retcode = EPKG_OK;
	unsigned int field = REPO_SEARCH_NAME;
	int ch;
	char size[7], dbfile[MAXPATHLEN];
	struct pkgdb *db = NULL;
	struct pkgdb_it *it = NULL;
	struct pkg *pkg = NULL;
	struct pkg_remote_repo *repo = NULL;

	while ((ch = getopt(argc, argv, "gxXcd")) != -1) {
		switch (ch) {
			case 'g':
				match = MATCH_GLOB;
				break;
			case 'x':
				match = MATCH_REGEX;
				break;
			case 'X':
				match = MATCH_EREGEX;
				break;
			case 'c':
				field |= REPO_SEARCH_COMMENT;
				break;
			case 'd':
				field |= REPO_SEARCH_DESCRIPTION;
				break;
			default:
				usage_search();
				return (EX_USAGE);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage_search();
		return (EX_USAGE);
	}

	pattern = argv[0];

	pkg_remote_repo_init();
	pkg_remote_repo_load();


	/*
	 * TODO: Implement a feature to search only
	 * in a given repository instead of all
	 */
	while ((repo = pkg_remote_repo_next()) != NULL) {
		snprintf(dbfile, MAXPATHLEN, "%s.sqlite", repo->name);

		if ((retcode = pkgdb_open(&db, PKGDB_REMOTE, dbfile)) != EPKG_OK) {
			warnx("cannot open repository database: %s/%s\n", 
					pkg_config("PKG_DBDIR"), dbfile);
			warnx("skipping repository %s\n", repo->name);
			pkgdb_close(db);
			continue;
		}

		if ((it = pkgdb_rquery(db, pattern, match, field)) == NULL) {
			warnx("cannot query repository database: %s/%s\n",
					pkg_config("PKG_DBDIR"), dbfile);
			warnx("skipping repository %s\n", repo->name);
			pkgdb_it_free(it);
			pkgdb_close(db);
			continue;
		}

		while ((retcode = pkgdb_it_next(it, &pkg, PKG_LOAD_BASIC)) == EPKG_OK) {
			printf("Name:       %s\n", pkg_get(pkg, PKG_NAME));
			printf("Version:    %s\n", pkg_get(pkg, PKG_VERSION));
			printf("Origin:     %s\n", pkg_get(pkg, PKG_ORIGIN));
			printf("Arch:       %s\n", pkg_get(pkg, PKG_ARCH));
			printf("Maintainer: %s\n", pkg_get(pkg, PKG_MAINTAINER));
			printf("WWW:        %s\n", pkg_get(pkg, PKG_WWW));
			printf("Comment:    %s\n", pkg_get(pkg, PKG_COMMENT));
			printf("Repository: %s\n", repo->name);
			humanize_number(size, sizeof(size), pkg_new_flatsize(pkg), "B", HN_AUTOSCALE, 0);
			printf("Flat size:  %s\n", size);
			humanize_number(size, sizeof(size), pkg_new_pkgsize(pkg), "B", HN_AUTOSCALE, 0);
			printf("Pkg size:   %s\n", size);
			printf("\n");
		}
	}

	pkg_remote_repo_free();

	return (retcode);
}
