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

/* ------------------------------------------------------------------------ */
//## method Boolean ResultSet.next();

KMETHOD ResultSet_next(KonohaContext *kctx, KonohaStack *sfp)
{
	RETURNb_(knh_ResultSet_next(kctx, (kResultSet*)sfp[0].asObject));
}

/* ------------------------------------------------------------------------ */

static int knh_ResultSet_indexof_(KonohaContext *kctx, KonohaStack *sfp)
{
	kResultSet *o = (kResultSet*)sfp[0].o;
	if(IS_Int(sfp[1].asObject)) {
		size_t n = (size_t)sfp[1].intValue;
		if(!(n < o->column_size)) {
			//THROW_OutOfRange(ctx, sfp, sfp[1].ivalue, (o)->column_size);
			return -1;
		}
		return n;
	}
	else if(IS_String(sfp[1].asObject)) {
		int loc = knh_ResultSet_findColumn(kctx, o, S_text(sfp[1].asString));
		if(loc == -1) {
			//TODO();
			//KNH_STUPID(ctx, o, STUPID_NOTFOUND);
		}
		return loc;
	}
	//TODO();
	//KNH_STUPID(ctx, o, STUPID_NOTFOUND);
	return -1;
}

/* ------------------------------------------------------------------------ */
//## method Int ResultSet.getInt(dynamic n);

KMETHOD ResultSet_getInt(KonohaContext *kctx, KonohaStack *sfp)
{
	fprintf(stderr, "hogehogehoge\n");
	int n = knh_ResultSet_indexof_(kctx, sfp);
	kint_t res = 0;
	char data[16];
	if(n >= 0) {
		kResultSet *o = (kResultSet*)sfp[0].asObject;
		const char *p = o->databuf->text + (o)->column[n].start;
		switch((o)->column[n].ctype) {
		case knh_ResultSet_CTYPE__integer :
			memset(data, '\0', 16);
			memcpy(data, p, o->column[n].len);
			res = strtoll(data, NULL, 10);
			//res = *((kint_t*)p); break;
			break;
		case knh_ResultSet_CTYPE__float :
			res = (kint_t)(*((kfloat_t*)p)); break;
		case knh_ResultSet_CTYPE__null :
		default:
			//KSETv(NULL, sfp[_rix].o, KNH_NULVAL(CLASS_Int));
			break;
		}
	}
	RETURNi_(res);
}

/* ------------------------------------------------------------------------ */
//## method Float ResultSet.getFloat(dynamic n);

KMETHOD ResultSet_getFloat(KonohaContext *kctx, KonohaStack *sfp)
{
	int n = knh_ResultSet_indexof_(kctx, sfp);
	kfloat_t res = 0.0;
	if(n >= 0) {
		kResultSet *o = (kResultSet*)sfp[0].asObject;
		const char *p = o->databuf->text + o->column[n].start;
		switch((o)->column[n].ctype) {
		case knh_ResultSet_CTYPE__integer :
			res = (kfloat_t)(*((kint_t*)p)); break;
		case knh_ResultSet_CTYPE__float :
			res = (*((kfloat_t*)p)); break;
		case knh_ResultSet_CTYPE__null :
		default:
			//KSETv(NULL, sfp[_rix].o, KNH_NULVAL(CLASS_Float));
			break;
		}
	}
	RETURNf_(res);
}

/* ------------------------------------------------------------------------ */
//## method String ResultSet.getString(dynamic n);

KMETHOD ResultSet_getString(KonohaContext *kctx, KonohaStack *sfp)
{
	int n = knh_ResultSet_indexof_(kctx, sfp);
	kResultSet* o = (kResultSet*)sfp[0].asObject;
	DBG_ASSERT(n < o->column_size);
	const char *p = o->databuf->text + o->column[n].start;
	switch(o->column[n].ctype) {
	case knh_ResultSet_CTYPE__integer :
		break;
		kbytes_t t = {o->column[n].len, {p}};
		RETURN_(KLIB new_kString(kctx, t.text, t.len, 0));
	case knh_ResultSet_CTYPE__float :
		break;
		//return new_String__float(kctx, (kfloat_t)(*((kfloat_t*)p)));
	case knh_ResultSet_CTYPE__text : {
		kbytes_t t = {o->column[n].len, {p}};
		//kbytes_t t = {{p}, o->column[n].len};
		//break;
		RETURN_(KLIB new_kString(kctx, t.text, t.len, 0));
		}
	case knh_ResultSet_CTYPE__null :
		break;
	}
	RETURN_(KLIB new_kString(kctx, "", 0, 0));
//	Object *v = KNH_NULL;
//	if(n >= 0) {
//		v = UPCAST(knh_ResultSet_getString(ctx, (kResultSet*)sfp[0].o, n));
//	}
//	RETURN_(v);
}

/* ------------------------------------------------------------------------ */
//## method dynamic ResultSet.get(dynamic n);

//KMETHOD ResultSet_get(KonohaContext *kctx, KonohaStack *sfp)
//{
//	
//	int n = knh_ResultSet_indexof_(ctx, sfp);
//	Object *v = KNH_NULL;
//	if(n >= 0) {
//		kResultSet *o = (kResultSet*)sfp[0].o;
//		const char *p = BA_totext((o)->databuf) + (o)->column[n].start;
//		switch((o)->column[n].ctype) {
//		case knh_ResultSet_CTYPE__integer : {
//			kint_t val;
//			knh_memcpy(&val, p, sizeof(kint_t));
//			KNH_SETv(ctx, sfp[_rix].o, new_Int_(ctx, CLASS_Int, val));
//			RETURNi_((*((kint_t*)p)));
//		}
//		case knh_ResultSet_CTYPE__float : {
//			kfloat_t val;
//			knh_memcpy(&val, p, sizeof(kfloat_t));
//			KNH_SETv(ctx, sfp[_rix].o, new_Float_(ctx, CLASS_Float, val));
//			RETURNf_((*((kfloat_t*)p)));
//		}
//		case knh_ResultSet_CTYPE__text : {
//			kbytes_t t = {{BA_totext((o)->databuf) + (o)->column[n].start}, (o)->column[n].len};
//			v = UPCAST(new_S(t.text, t.len));
//			break;
//		}
//		case knh_ResultSet_CTYPE__bytes :
//			{
//				kBytes *ba = new_Bytes(ctx, BA_totext((o)->databuf) + (o)->column[n].start, (o)->column[n].len);
//				kbytes_t t = {{BA_totext((o)->databuf) + (o)->column[n].start}, (o)->column[n].len};
//				knh_Bytes_write(ctx, ba, t);
//				v = UPCAST(ba);
//			}
//			break;
//		default:
//			v = KNH_NULL;
//		}
//	}
//	RETURN_(v);
//}
//
/* ------------------------------------------------------------------------ */
//## method void ResultSet.%dump(OutputStream w, String m);
//
//static void knh_ResultSet__dump(KonohaContext *kctx, kResultSet *o, kOutputStream *w, kString *m)
//{
//	knh_putc(ctx, w, '{');
//	size_t n;
//	for(n = 0; n < (o)->column_size; n++) {
//		if(n > 0) {
//			knh_write_delim(ctx,w);
//		}
//		knh_write(ctx, w, S_tobytes((o)->column[n].name));
//		knh_printf(ctx, w, "(%d): ", n);
//		char *p = BA_totext((o)->databuf) + (o)->column[n].start;
//		switch((o)->column[n].ctype) {
//			case knh_ResultSet_CTYPE__null :
//				knh_write(ctx, w, STEXT("null"));
//				break;
//			case knh_ResultSet_CTYPE__integer :
//				knh_write_ifmt(ctx, w, KINT_FMT, (*((kint_t*)p)));
//				break;
//			case knh_ResultSet_CTYPE__float :
//				knh_write_ffmt(ctx, w, KFLOAT_FMT, (*((kfloat_t*)p)));
//				break;
//			case knh_ResultSet_CTYPE__text :
//				knh_write(ctx, w, B2(p, (o)->column[n].len));
//				break;
//			case knh_ResultSet_CTYPE__bytes :
//				knh_printf(ctx, w, "BLOB(%dbytes)", (o)->column[n].len);
//				break;
//		}
//	}
//	knh_putc(ctx, w, '}');
//}
