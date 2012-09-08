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

#include <stdio.h>

#ifndef SQL_COMMON_H_
#define SQL_COMMON_H_

/* ------------------------------------------------------------------------ */
/* [bytes struct] */

typedef struct {
	size_t       len;
	union {
		const char *text;
		const unsigned char *utext;
		char *buf;
		//kchar_t *ubuf;
	};
} kbytes_t;


/* ------------------------------------------------------------------------ */
/* [bytes API] */

static inline kbytes_t new_bytes(char *c_buf)
{
	DBG_ASSERT(c_buf != NULL);
	kbytes_t t;
	t.utext = (unsigned char*)c_buf;
	t.len = strlen(t.text);
	return t;
}

#define B(c)      new_bytes((char*)c)

int knh_bytes_parseint(kbytes_t t, kint_t *value)
{
	kuint_t n = 0, prev = 0, base = 10;
	size_t i = 0;
	if(t.len > 1) {
		if(t.utext[0] == '0') {
			if(t.utext[1] == 'x') {
				base = 16; i = 2;
			}
			else if(t.utext[1] == 'b') {
				base = 2;  i = 2;
			}
			else {
				base = 8;  i = 1;
			}
		}else if(t.utext[0] == '-') {
			base = 10; i = 1;
		}
	}
	for(;i < t.len; i++) {
		int c = t.utext[i];
		if('0' <= c && c <= '9') {
			prev = n;
			n = n * base + (c - '0');
		}else if(base == 16) {
			if('A' <= c && c <= 'F') {
				prev = n;
				n = n * 16 + (10 + c - 'A');
			}else if('a' <= c && c <= 'f') {
				prev = n;
				n = n * 16 + (10 + c - 'a');
			}else {
				break;
			}
		}else if(c == '_') {
			continue;
		}else {
			break;
		}
		if(!(n >= prev)) {
			*value = 0;
			return 0;
		}
	}
	if(t.utext[0] == '-') n = -((kint_t)n);
	*value = n;
	return 1;
}

int knh_bytes_parsefloat(kbytes_t t, kfloat_t *value)
{
#if defined(K_USING_NOFLOAT)
	{
		kint_t v = 0;
		knh_bytes_parseint(t, &v);
		*value = (kfloat_t)v;
	}
#else
	*value = strtod(t.text, NULL);
#endif
	return 1;
}

/* ======================================================================== */
/* [struct] */
/* ------------------------------------------------------------------------ */
/* [Channel] */

#define _Public   kMethod_Public
#define _Const    kMethod_Const
#define _Coercion kMethod_Coercion
#define _Im kMethod_Immutable
#define _F(F)   (intptr_t)(F)

#define _KVi(T)  #T, TY_Int, T

//typedef struct knh_Channel_t {
//	kObjectHeader h;
//	kintptr_t sd;
//	struct kString *urn;
//	struct kInputStream  *in;
//	struct kOutputStream *out;
//} knh_Channel_t;

/* ------------------------------------------------------------------------ */
/* [Connection] */

typedef void kconn_t;
typedef const struct _kQueryDPI_t kQueryDSPI_t;
typedef const struct _kConnection kConnection;

struct _kConnection {
	KonohaObjectHeader  h;
	void                *db;
	const struct _kQueryDPI_t *dspi;
	kString             *urn;
};

#define CT_Connection     cConnection
#define TY_Connection     cConnection->typeId
#define IS_Connection(O)  ((O)->h.ct == CT_Connection)

/* ------------------------------------------------------------------------ */
/* [ResultSet] */

typedef const struct _kDBschema kDBschema;

struct _kDBschema {
	ktype_t   type;
	kushort_t ctype;
	kString   *name;
	size_t    start;
	size_t    len;
	int       dbtype;
};

typedef void kqcur_t;

typedef const struct _kResultSet kResultSet;
struct _kResultSet{
	KonohaObjectHeader  h;
	kConnection         *connection;
	kqcur_t             *qcur;
	void                (*qcurfree)(kqcur_t *); /* necessary if conn is closed before */
	kString             *tableName;
	//kclass_t            tcid;
	kushort_t           column_size;
	struct _kDBschema   *column;
	kBytes              *databuf;
	size_t              count;
};

#define CT_ResultSet     cResultSet
#define TY_ResultSet     cResultSet->typeId
#define IS_ResultSet(O)  ((O)->h.ct == CT_ResultSet)

/* ------------------------------------------------------------------------ */
/* [prototype define] */

//void knh_Connection_open(KonohaContext *kctx, kConnection *c, kString *urn);
//void knh_Connection_close(KonohaContext *kctx, kConnection *c);
//void knh_ResultSet_initData(KonohaContext *kctx, kResultSet *rs);
//kbool_t knh_ResultSet_next(KonohaContext *kctx, kResultSet *o);
//kString *knh_ResultSet_getName(KonohaContext *kctx, kResultSet *o, size_t n);
//int knh_ResultSet_findColumn(KonohaContext *kctx, kResultSet *o, kbytes_t name);
//kString* knh_ResultSet_getString(KonohaContext *kctx, kResultSet *o, size_t n);
//void knh_ResultSet_close(KonohaContext *kctx, kResultSet *o);
//
//KMETHOD knh_ResultSet_initColumn(KonohaContext *kctx, kResultSet *o, size_t column_size);
//KMETHOD ResultSet_setBlob(KonohaContext *kctx, kResultSet *o, size_t n, kbytes_t t);
//KMETHOD ResultSet_setFloat(KonohaContext *kctx, kResultSet *rs, size_t n, kfloat_t value);
//KMETHOD ResultSet_setInt(KonohaContext *kctx, kResultSet *rs, size_t n, kint_t value);
//KMETHOD ResultSet_setNULL(KonohaContext *kctx, kResultSet *o, size_t n);
//KMETHOD ResultSet_setName(KonohaContext *kctx, kResultSet *o, size_t n, kString *name);
//KMETHOD ResultSet_setText(KonohaContext *kctx, kResultSet *o, size_t n, kbytes_t t);

/* ------------------------------------------------------------------------ */
/* [driver] */
/* [TODO] Now, ResultSet driver is global value */
/* but it should be shared using anything without global value */

typedef struct knh_ResultSetDef_t {
	//KMETHOD (*initColumn)(KonohaContext *kctx, kResultSet *o, size_t column_size);
	//KMETHOD (*setBlob)(KonohaContext *kctx, kResultSet *o, size_t n, kbytes_t t);
	//KMETHOD (*setFloat)(KonohaContext *kctx, kResultSet *rs, size_t n, kfloat_t value);
	//KMETHOD (*setInt)(KonohaContext *kctx, kResultSet *rs, size_t n, kint_t value);
	//KMETHOD (*setNULL)(KonohaContext *kctx, kResultSet *o, size_t n);
	//KMETHOD (*setName)(KonohaContext *kctx, kResultSet *o, size_t n, kString *name);
	//KMETHOD (*setText)(KonohaContext *kctx, kResultSet *o, size_t n, kbytes_t t);
} knh_ResultSetDef_t;

static const knh_ResultSetDef_t ResultSetDef = {
	//knh_ResultSet_initColumn,
	//ResultSet_setBlob,
	//ResultSet_setFloat,
	//ResultSet_setInt,
	//ResultSet_setNULL,
	//ResultSet_setName,
	//ResultSet_setText,
};
/* ------------------------------------------------------------------------ */
/* K_DSPI_QUERY */

struct _kQueryDPI_t {
	int   type;
	const char *name;
	void* (*qopen)(KonohaContext* kctx, const char* db);
	kqcur_t* (*qexec)(KonohaContext* kctx, void* db, const char* query, struct _kResultSet* rs);
	void   (*qclose)(void* hdr);
	//kconn_t* (*qopen)(KonohaContext *kctx, kbytes_t);
	//kqcur_t* (*qexec)(KonohaContext *kctx, kconn_t *, kbytes_t, kResultSet*);
	//void   (*qclose)(KonohaContext *kctx, kconn_t *);
	int    (*qcurnext)(KonohaContext* kctx, kqcur_t * qcur, struct _kResultSet* rs);
	void   (*qcurfree)(kqcur_t *);
};

/* ------------------------------------------------------------------------ */
/* [Macros] */
#define knh_ResultSet_CTYPE__null    0
#define knh_ResultSet_CTYPE__integer 1
#define knh_ResultSet_CTYPE__float   2
#define knh_ResultSet_CTYPE__text    3  /* UTF-8*/
#define knh_ResultSet_CTYPE__bytes   4
#define knh_ResultSet_CTYPE__Object  5

#define RESULTSET_BUFSIZE 256
#define K_DSPI_QUERY 1

//#include "include/query.h"
//#include "include/dbapi.h"
//#include "include/mysql.h"
//#include "include/sqlite.h"
//#include "include/postgresql.h"

//const kQueryDSPI_t DB__mysql = {
//	K_DSPI_QUERY, "mysql",
//	MYSQL_qopen, MYSQL_query, MYSQL_qclose, MYSQL_qnext, MYSQL_qfree
//};

#endif /* SQL_COMMON_H_ */
