/* KONOHA COPYRIGHT, LICENSE NOTICE, AND DISCRIMER */

/* Copyright (c)  2010-      Konoha Team konohaken@googlegroups.com */
/* All rights reserved. */

/* You may choose one of the following two licenses when you use konoha. */
/* See www.konohaware.org/license.html for further information. */

/* (1) GNU Lesser General Public License 3.0 (with KONOHA_UNDER_LGPL3) */
/* (2) Konoha Software Foundation License 1.0 */

/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS */
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER */
/* OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, */
/* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR */
/* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


/* ************************************************************************** */
/* LIST OF CONTRIBUTERS */
/*  kimio - Kimio Kuramitsu, Yokohama National University, Japan */
/*  uh    - Yutaro Hiraoka, Yokohama National University, Japan */
/*  yoan  - Motoki Yoan, Yokohama National University, Japan */
/* ************************************************************************** */

#include <sqlite3.h>
#include <stdio.h>

//------------------------------------------------------------------------ */

#define K_PATHHEAD_MAXSIZ    32

kbytes_t knh_bytes_skipPATHHEAD(kbytes_t path)
{
	fprintf(stderr, "===<<<knh_bytes_skipPATHHEAD>>>===\n");
	size_t i;
	for(i = 1; i < path.len; i++) {
		if(i == K_PATHHEAD_MAXSIZ) break;
		if(path.utext[i] == ':') {
			path.utext = path.utext + (i + 1);
			path.len = path.len - (i + 1);
			return path;
		}
	}
	return path;
}

static void knh_sqlite3_perror(KonohaContext* kctx, sqlite3 *db, int r)
{
	fprintf(stderr, "===<<<knh_knh_sqlite3_perror>>>===\n");
//	const char *msg = "sqlite3";
	if(r == SQLITE_PERM || r == SQLITE_AUTH) {
		//msg = "Security";
	}
	//KNH_SYSLOG(ctx, LOG_WARNING, msg, "sqlite3_error='%s'", sqlite3_errmsg(db));
}

void *SQLITE3_qopen(KonohaContext* kctx,  const char* db)
{
	fprintf(stderr, "===<<<SQLITE3_qopen>>>===\n");
	sqlite3 *db_sqlite3 = NULL;
//	db = knh_bytes_skipPATHHEAD(db);
	fprintf(stderr, "%s\n", db);
	db += 9;
	int r = sqlite3_open(db, &db_sqlite3);
	if (r != SQLITE_OK) {
		return NULL;
	}
	return (kconn_t*)db_sqlite3;
}

int SQLITE3_qnext(KonohaContext* kctx, kqcur_t *qcur, kResultSet *rs)
{
	fprintf(stderr, "@@@@@@@@@@@@ enter next\n");
	sqlite3_stmt *stmt = (sqlite3_stmt*)qcur;
	int r = sqlite3_step(stmt);
	if(SQLITE_ROW == r) {
		size_t i;
//		knh_ResultSet_initData(kctx, rs);
		for(i = 0; i < rs->column_size; i++) {
			int type = sqlite3_column_type(stmt, i);
			switch(type) {
				case SQLITE_INTEGER: {
					fprintf(stderr, "===<<<SQLITE_INTEGER>>>===\n");
					ResultSet_setInt(kctx, rs, i, (kint_t)sqlite3_column_int64(stmt, i));
					break;
				}
				case SQLITE_FLOAT: {
					ResultSet_setFloat(kctx, rs, i, (kfloat_t)sqlite3_column_double(stmt, i));
					break;
				}
				case SQLITE_TEXT: {
					kbytes_t t = {sqlite3_column_bytes(stmt, i), {(const char*)sqlite3_column_text(stmt,i)}};
					ResultSet_setText(kctx, rs, i, t);
					break;
				}
				case SQLITE_BLOB: {
					kbytes_t t = {sqlite3_column_bytes(stmt, i), {(const char*)sqlite3_column_blob(stmt,i)},};
					ResultSet_setBlob(kctx, rs, i, t);
					break;
				}
				case SQLITE_NULL:
				default: {
					ResultSet_setNULL(kctx, rs, i);
				}
			}
		}
		return 1;
	}
	else if (r != SQLITE_DONE) {
	   //
	}
	return 0;  /* NOMORE */
}

kqcur_t *SQLITE3_query(KonohaContext* kctx, void *db, const char* query,struct _kResultSet *rs)
{
	fprintf(stderr, "===<<<SQLITE3_query>>>===\n");
	fprintf(stderr, "query = %s\n", query);
	if(rs == NULL) {
		fprintf(stderr, "===<<<rs == NULL>>>===\n");
		int r = sqlite3_exec((sqlite3*)db, query, NULL, NULL, NULL);
		if(r != SQLITE_OK) {
			knh_sqlite3_perror(kctx, (sqlite3*)db, r);
		}
		return NULL;
	}
	else {
		fprintf(stderr, "===<<<rs != NULL>>>===\n");
		sqlite3_stmt *stmt = NULL;
		sqlite3_prepare((sqlite3*)db, query, strlen(query), &stmt, NULL);
	/* if (r != SQLITE_OK) { */
	/* 	sqlite3_finalize(stmt); */
	/* 	DBG_P("msg='%s", sqlite3_errmsg((sqlite3)db)); */
	/* 	return NULL; */
	/* } */
	/* 	r = sqlite3_reset(stmt); */
	/* if(r != SQLITE_OK) { */
	/* 	sqlite3_finalize(stmt); */
	/* 	return NULL; */
		size_t column_size = (size_t)sqlite3_column_count(stmt);
		DBG_P("column_size=%d", column_size);
		fprintf(stderr, "<SQLITE3_query> column_size = %d\n", column_size);

		knh_ResultSet_initColumn(kctx, rs, column_size);
		if(column_size == 0) {
			sqlite3_exec((sqlite3*)db, query, NULL, NULL, NULL);
		}else if(column_size > 0) {
			size_t i;
			for(i = 0; i < column_size; i++) {
				char *n = (char*)sqlite3_column_name(stmt, i);
				DBG_P("(%d) name = '%s'", i, n);
				if(n != NULL) {
					rs->column[i].dbtype = sqlite3_column_type(stmt, i);
					kString *s = KLIB new_kString(kctx, n, strlen(n), 0);
					KSETv(rs, rs->column[i].name, s);
				}
			}
		}
		return (kqcur_t*)stmt;
	}
}

void SQLITE3_qclose(void *hdr)
{
	fprintf(stderr, "===<<<SQLITE3_qclose>>>===\n");
	sqlite3_close((sqlite3*)hdr);
}

void SQLITE3_qfree(kqcur_t *qcur)
{
	fprintf(stderr, "===<<<SQLITE3_qfree>>>===\n");
	sqlite3_stmt *stmt = (sqlite3_stmt*)qcur;
	sqlite3_finalize(stmt);
}

const kQueryDSPI_t DB__sqlite3 = {
	K_DSPI_QUERY, "sqlite3",
	SQLITE3_qopen, SQLITE3_query, SQLITE3_qclose, SQLITE3_qnext, SQLITE3_qfree
};

//------------------------------------------------------------------------ */
