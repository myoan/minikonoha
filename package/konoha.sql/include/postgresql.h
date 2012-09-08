/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

// **************************************************************************
// LIST OF CONTRIBUTERS
//  kimio - Kimio Kuramitsu, Yokohama National University, Japan
//  uh    - Yutaro Hiraoka, Yokohama National University, Japan
//  yoan  - Motoki Yoan, Yokohama National University, Japan
// **************************************************************************

#include<libpq-fe.h>
//#include<postgresql/libpq-fe.h>

/* ------------------------------------------------------------------------ */
/* [postgresql datatype]    */
/* reference from pg_type.h */

#define  INT8OID   20
#define  INT2OID   21
#define  INT4OID   23
#define  FLOAT4OID   700
#define  FLOAT8OID   701
#define  VARCHAROID   1043
#define  DATEOID   1082
#define  TIMESTAMPOID   1114

/* ------------------------------------------------------------------------ */

//static void knh_sqlite3_perror(KonohaContext *kctx, sqlite3 *db, int r)
//{
//}

static void *POSTGRESQL_qopen(KonohaContext *kctx, const char* url)
{
	PGconn *conn;
	url += 13;
	conn = PQconnectdb(url);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
		return NULL;
	}
	return (void*)conn;
}

static int POSTGRESQL_qnext(KonohaContext *kctx, kqcur_t *qcur, struct _kResultSet *rs)
{
	PGresult* res = (PGresult*)qcur;
	size_t i, row_size = (size_t)PQnfields(res), column_size = PQntuples(res);
	if (row_size > 0) {
		kint_t ival;
		kfloat_t fval;
		for (i = 0; i < column_size; i++) {
			char *name = (char*)PQfname(res, i);
			fprintf(stderr, "(%lu) name = '%s'\n", i, name);
			Oid type = (Oid)rs->column[i].dbtype;
			switch (type) {
				case INT8OID:
				case INT2OID:
				case INT4OID:
					knh_bytes_parseint(B(PQgetvalue(res, 0, i)), &ival);
					ResultSet_setInt(kctx, rs, i, ival);
					break;
				case FLOAT4OID:
				case FLOAT8OID:
					knh_bytes_parsefloat(B(PQgetvalue(res, 0, i)), &fval);
					ResultSet_setFloat(kctx, rs, i, fval);
					break;
				case VARCHAROID:
					ResultSet_setText(kctx, rs, i, B((PQgetvalue(res, 0, i))));
					break;
				case DATEOID:
					break;
				case TIMESTAMPOID:
					break;
				default: 
					fprintf(stderr, "ERROR: [qnext]\n");
			}
		}
		return 1;
	} else {
		return 0; /* NOMORE */
	}
}

static kqcur_t *POSTGRESQL_query(KonohaContext *kctx, kconn_t *db, const char* sql, struct _kResultSet *rs)
{
	PGresult* res;
	if(rs == NULL) {
		res = PQexec((PGconn*)db, sql);
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			fprintf(stderr, "ERROR: %s", PQerrorMessage(db));
			// ktrace....
		}
		PQclear(res);
		return NULL;
	}
	else {
		res = PQexec((PGconn*)db, sql);
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			fprintf(stderr, "ERROR: %s", PQerrorMessage(db));
			// ktrace....
		}
		size_t column_size = (size_t)PQnfields(res);
		fprintf(stderr, "column_size=%lu\n", column_size);
		knh_ResultSet_initColumn(kctx, rs, column_size);
		if(column_size > 0) {
			size_t i;
			for(i = 0; i < rs->column_size; i++) {
				char *name = (char*)PQfname(res, i);
				fprintf(stderr, "(%lu) name = '%s'\n", i, name);
				if(name != NULL) {
					rs->column[i].dbtype = (int)PQftype(res, i);
					kString *s = KLIB new_kString(kctx, name, strlen(name), 0);
					DBG_ASSERT(i < rs->column_size);
					KSETv(rs, rs->column[i].name, s);
				}
			}
		}
		//PQclear(res);
		return (kqcur_t*)res;
	}
	return NULL;
}

static void POSTGRESQL_qclose(kconn_t *db)
{
	PQfinish(db);

}

static void POSTGRESQL_qfree(kqcur_t *qcur)
{
//	sqlite3_stmt *stmt = (sqlite3_stmt*)qcur;
//	sqlite3_finalize(stmt);
}

const kQueryDSPI_t DB__postgresql = {
	K_DSPI_QUERY, "postgresql",
	POSTGRESQL_qopen, POSTGRESQL_query, POSTGRESQL_qclose, POSTGRESQL_qnext, POSTGRESQL_qfree
};
