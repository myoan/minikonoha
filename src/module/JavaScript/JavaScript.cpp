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

#include <stdio.h>
#include <iconv.h>
#include <errno.h>
#include <minikonoha/minikonoha.h>
#include <minikonoha/sugar.h>
#include <minikonoha/klib.h>
#include <minikonoha/arch/minivm.h>
#ifdef HAVE_LIBV8
	#include <v8.h>
#endif

#define LOG_FUNCTION_NAME "p"

extern "C" {

#ifdef HAVE_LIBV8

static v8::Handle<v8::Value> JSLog(const v8::Arguments& args)
{
	if(args.Length() < 1) {
		return v8::Undefined();
	}

	v8::HandleScope scope;
	v8::Handle<v8::Value> arg = args[0];
	v8::String::Utf8Value value(arg);
	printf("%s\n", *value);

	return v8::Undefined();
}

v8::Persistent<v8::Context> createContext(){	
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	global->Set(v8::String::New("p"), v8::FunctionTemplate::New(JSLog));
	return v8::Context::New(NULL, global);
}

class JSContext{
public:
	v8::HandleScope handleScope;
	v8::Persistent<v8::Context> context;
	v8::Context::Scope contextScope;
	JSContext():context(createContext()),contextScope(v8::Context::Scope(context)){
	}
	~JSContext(){
		context.Dispose();
	}
};

static JSContext* globalJSContext = NULL;

#endif

typedef struct JSBuilder {
	struct KBuilderCommon common;
	kbool_t isIndentEmitted;
	kMethod *visitingMethod;
	int indent;
	KGrowingArray buffer;
	KBuffer jsCodeBuffer;
} JSBuilder;

/* ------------------------------------------------------------------------ */
/* [Statement/Expression API] */
static kNode* Node_getFirstBlock(KonohaContext *kctx, kNode *stmt)
{
	return SUGAR kNode_GetNode(kctx, stmt, KSymbol_BlockPattern, K_NULLBLOCK);
}

static kNode* Node_getElseBlock(KonohaContext *kctx, kNode *stmt)
{
	return SUGAR kNode_GetNode(kctx, stmt, KSymbol_else, K_NULLBLOCK);
}

static kNode* Node_getFirstExpr(KonohaContext *kctx, kNode *stmt)
{
	return SUGAR kNode_GetNode(kctx, stmt, KSymbol_ExprPattern, NULL);
}

static kMethod* CallNode_getMethod(kNode *expr)
{
	return expr->NodeList->MethodItems[0];
}

#define MN_opNOT  KMethodName_("!")
#define MN_opADD  KMethodName_("+")
#define MN_opSUB  KMethodName_("-")
#define MN_opMUL  KMethodName_("*")
#define MN_opDIV  KMethodName_("/")
#define MN_opMOD  KMethodName_("%")
#define MN_opEQ   KMethodName_("==")
#define MN_opNEQ  KMethodName_("!=")
#define MN_opLT   KMethodName_("<")
#define MN_opLTE  KMethodName_("<=")
#define MN_opGT   KMethodName_(">")
#define MN_opGTE  KMethodName_(">=")
#define MN_opLAND KMethodName_("&")
#define MN_opLOR  KMethodName_("|")
#define MN_opLXOR KMethodName_("^")
#define MN_opLSFT KMethodName_("<<")
#define MN_opRSFT KMethodName_(">>")

enum kSymbolPrefix{
	kSymbolPrefix_NONE,
	kSymbolPrefix_SET,
	kSymbolPrefix_GET,
	kSymbolPrefix_AT,
	kSymbolPrefix_IS,
	kSymbolPrefix_UNKNOWN,
	kSymbolPrefix_TO,
	kSymbolPrefix_DOLLAR,
};

static enum kSymbolPrefix KSymbol_prefixText_ID(ksymbol_t sym)
{
	size_t mask = ((size_t)(KSymbol_Attr(sym)) >> ((sizeof(ksymbol_t) * 8)-3));
	DBG_ASSERT(mask < 8);
	return (enum kSymbolPrefix)mask;
}

static void JSBuilder_EmitIndent(KonohaContext *kctx, KBuilder *builder)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	if(!jsBuilder->isIndentEmitted) {
		int i;
		for(i = 0; i < jsBuilder->indent; i++) {
			KLIB KBuffer_printf(kctx, &jsBuilder->jsCodeBuffer, "    ");
		}
		jsBuilder->isIndentEmitted = true;
	}
}

static void JSBuilder_EmitNewLineWith(KonohaContext *kctx, KBuilder *builder, const char* endline)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	JSBuilder_EmitIndent(kctx, builder);
	jsBuilder->isIndentEmitted = false;
	KLIB KBuffer_printf(kctx, &jsBuilder->jsCodeBuffer, "%s\n", endline);
}

static void JSBuilder_EmitString(KonohaContext *kctx, KBuilder *builder, const char* prefix, const char* str, const char* suffix)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	JSBuilder_EmitIndent(kctx, builder);
	KLIB KBuffer_printf(kctx, &jsBuilder->jsCodeBuffer, "%s%s%s", prefix, str, suffix);
}

static kbool_t JSBuilder_VisitBlockNode(KonohaContext *kctx, KBuilder *builder, kNode *block, void *thunk)
{
	size_t i;
	kbool_t ret = true;
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	JSBuilder_EmitNewLineWith(kctx, builder, "{");
	jsBuilder->indent++;
	for (i = 0; i < kArray_size(block->NodeList); i++) {
		kNode *stmt = block->NodeList->NodeItems[i];
		if(!SUGAR VisitNode(kctx, builder, stmt, thunk)) {
			ret = false;
			break;
		}
	}
	jsBuilder->indent--;
	JSBuilder_EmitString(kctx, builder, "}", "", "");
	return ret;
}

static kbool_t JSBuilder_VisitNode(KonohaContext *kctx, KBuilder *builder, kNode *expr, void *thunk, const char* prefix, const char* suffix)
{
	JSBuilder_EmitString(kctx, builder, prefix, "", "");
	kbool_t ret = SUGAR VisitNode(kctx, builder, expr, thunk);
	JSBuilder_EmitString(kctx, builder, suffix, "", "");
	return ret;
}

static int KMethodName_isBinaryOperator(KonohaContext *kctx, kmethodn_t mn)
{
	if(mn == MN_opADD ) return 1;
	if(mn == MN_opSUB ) return 1;
	if(mn == MN_opMUL ) return 1;
	if(mn == MN_opDIV ) return 1;
	if(mn == MN_opMOD ) return 1;
	if(mn == MN_opEQ  ) return 1;
	if(mn == MN_opNEQ ) return 1;
	if(mn == MN_opLT  ) return 1;
	if(mn == MN_opLTE ) return 1;
	if(mn == MN_opGT  ) return 1;
	if(mn == MN_opGTE ) return 1;
	if(mn == MN_opLAND) return 1;
	if(mn == MN_opLOR ) return 1;
	if(mn == MN_opLXOR) return 1;
	if(mn == MN_opLSFT) return 1;
	if(mn == MN_opRSFT) return 1;
	return 0;
}

static int KMethodName_isUnaryOperator(KonohaContext *kctx, kmethodn_t mn)
{
	if(mn == MN_opSUB) return 1;
	if(mn == MN_opNOT) return 1;
	return 0;
}

static kbool_t JSBuilder_VisitPushNode(KonohaContext *kctx, KBuilder *builder, kNode *expr, void *thunk)
{
	SUGAR VisitNode(kctx, builder, expr->NodeToPush, thunk);  // ADDED by kimio to pass compilation
	return true;
}

static kbool_t JSBuilder_VisitBoxNode(KonohaContext *kctx, KBuilder *builder, kNode *expr, void *thunk)
{
	SUGAR VisitNode(kctx, builder, expr->NodeToPush, thunk);  // ADDED by kimio to pass compilation
	return true;
}

static kbool_t JSBuilder_VisitErrorNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	//JSBuilder_EmitString(kctx, builder, kString_text(kNode_GetObjectNULL(kctx, stmt, KSymbol_)), "", "");
	return true;
}
/*
static kbool_t JSBuilder_VisitNodiNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt)
{
	kNode *expr = Node_getFirstExpr(kctx, stmt);
	if(expr->node == KNode_Assign) {
		if(kNode_At(expr, 1)->node == KNode_Field){
			JSBuilder_EmitString(kctx, builder, "this.", "", "");
		}else{
			JSBuilder_EmitString(kctx, builder, "var ", "", "");
		}
	}
	SUGAR VisitNode(kctx, builder, expr);
	JSBuilder_EmitNewLineWith(kctx, builder, ";");
	return true;
}

static kbool_t JSBuilder_VisitNodeNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt)
{
	JSBuilder_VisitNode(kctx, builder, Node_getFirstNode(kctx, stmt));
	return true;
}
*/
static kbool_t JSBuilder_VisitReturnNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void *thunk)
{
	if(((JSBuilder *)builder)->visitingMethod->mn != 0) {
		JSBuilder_EmitString(kctx, builder, "return ", "", "");
	}
	kNode* expr = Node_getFirstExpr(kctx, stmt);
	if(expr != NULL && IS_Node(expr)) {
		SUGAR VisitNode(kctx, builder, expr, thunk);
	}
	JSBuilder_EmitNewLineWith(kctx, builder, ";");
	return true;
}

static kbool_t JSBuilder_VisitIfNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	JSBuilder_VisitNode(kctx, builder, Node_getFirstExpr(kctx, stmt), thunk, "if(", ") ");
	JSBuilder_VisitBlockNode(kctx, builder, Node_getFirstBlock(kctx, stmt), thunk);
	kNode *elseNode = Node_getElseBlock(kctx, stmt);
	if(elseNode != K_NULLBLOCK) {
		JSBuilder_EmitString(kctx, builder, "else", "", "");
		JSBuilder_VisitBlockNode(kctx, builder, elseNode, thunk);
	}
	JSBuilder_EmitNewLineWith(kctx, builder, "");
	return true;
}

static kbool_t JSBuilder_VisitWhileNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	JSBuilder_VisitNode(kctx, builder, Node_getFirstExpr(kctx, stmt), thunk, "while(", ") ");
	JSBuilder_VisitBlockNode(kctx, builder, Node_getFirstBlock(kctx, stmt), thunk);
	JSBuilder_EmitNewLineWith(kctx, builder, "");
	return true;
}

static kbool_t JSBuilder_VisitDoWhileNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	JSBuilder_EmitString(kctx, builder, "do", "", "");
	JSBuilder_VisitBlockNode(kctx, builder, Node_getFirstBlock(kctx, stmt), thunk);
	JSBuilder_VisitNode(kctx, builder, Node_getFirstExpr(kctx, stmt), thunk, "while(", ");");
	JSBuilder_EmitNewLineWith(kctx, builder, "");
	return true;
}

static kbool_t JSBuilder_VisitBreakNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void* thunk)
{
	JSBuilder_EmitString(kctx, builder, "break;", "", "");
	return true;
}

static kbool_t JSBuilder_VisitContinueNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void* thunk)
{
	JSBuilder_EmitString(kctx, builder, "continue;", "", "");
	return true;
}

static kbool_t JSBuilder_VisitThrowNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void* thunk)
{
	JSBuilder_VisitNode(kctx, builder, Node_getFirstExpr(kctx, node), thunk, "throw ", ";");
	return true;
}
/*
static kbool_t JSBuilder_VisitJumpNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	JSBuilder_EmitString(kctx, builder, "Jump", "", "");
	return true;
}
*/
static kbool_t JSBuilder_VisitTryNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	JSBuilder_EmitNewLineWith(kctx, builder, "try ");
	JSBuilder_VisitBlockNode(kctx, builder, Node_getFirstBlock(kctx, stmt), thunk);
	kNode *catchNode   = SUGAR kNode_GetNode(kctx, stmt, KSymbol_("catch"),   K_NULLBLOCK);
	kNode *finallyNode = SUGAR kNode_GetNode(kctx, stmt, KSymbol_("finally"), K_NULLBLOCK);
	if(catchNode != K_NULLBLOCK) {
		JSBuilder_EmitString(kctx, builder, "catch(e) ", "", "");
		JSBuilder_VisitBlockNode(kctx, builder, catchNode, thunk);
	}
	if(finallyNode != K_NULLBLOCK) {
		JSBuilder_EmitString(kctx, builder, "finally", "", "");
		JSBuilder_VisitBlockNode(kctx, builder, finallyNode, thunk);
	}
	JSBuilder_EmitNewLineWith(kctx, builder, "");
	return true;
}
/*
static kbool_t JSBuilder_VisitUndefinedNode(KonohaContext *kctx, KBuilder *builder, kNode *stmt, void* thunk)
{
	JSBuilder_EmitString(kctx, builder, "UNDEF", "", "");
	return false;
}
*/
static void JSBuilder_EmitKonohaValue(KonohaContext *kctx, KBuilder *builder, KClass* ct, KonohaStack* sfp)
{
	KBuffer wb;
	KLIB KBuffer_Init(&(kctx->stack->cwb), &wb);
	ct->p(kctx, sfp, 0, &wb);
	char *str = (char *)KLIB KBuffer_text(kctx, &wb, NonZero);
	JSBuilder_EmitString(kctx, builder, str, "", "");
	KLIB KBuffer_Free(&wb);
}

static void JSBuilder_EmitConstValue(KonohaContext *kctx, KBuilder *builder, kObject *obj)
{
	KonohaStack sfp[1];
	sfp[0].asObject = obj;
	JSBuilder_EmitKonohaValue(kctx, builder, kObject_class(obj), sfp);
}

static void JSBuilder_EmitUnboxConstValue(KonohaContext *kctx, KBuilder *builder, KClass *ct, unsigned long long unboxVal)
{
	KonohaStack sfp[1];
	sfp[0].unboxValue = unboxVal;
	JSBuilder_EmitKonohaValue(kctx, builder, ct, sfp);
}

static kbool_t JSBuilder_VisitConstNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_EmitConstValue(kctx, builder, node->ObjectConstValue);
	return true;
}

static kbool_t JSBuilder_VisitUnboxConstNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_EmitUnboxConstValue(kctx, builder, KClass_(node->attrTypeId), node->unboxConstValue);
	return true;
}

static kbool_t JSBuilder_VisitNewNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_EmitString(kctx, builder, "new", "", "");
	return true;
}

static kbool_t JSBuilder_VisitNullNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_EmitString(kctx, builder, "null", "", "");
	return true;
}

static kbool_t JSBuilder_VisitLocalNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	kToken *tk = (kToken *)node->TermToken;
	JSBuilder_EmitString(kctx, builder, kString_text(tk->text), "", "");
	return true;
}

static kbool_t JSBuilder_VisitFieldNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	kToken *tk = (kToken *)node->TermToken;
	JSBuilder_EmitString(kctx, builder, kString_text(tk->text), "", "");
	return true;
}

static bool JSBuilder_importPackage(KonohaContext *kctx, kNameSpace *ns, kString *package, kfileline_t uline)
{
	KBaseTrace(trace);
	KImportPackage(ns, kString_text(package), trace);
	return true;
}

static kbool_t JSBuilder_VisitNodeParams(KonohaContext *kctx, KBuilder *builder, kNode *expr, void *thunk, int beginIndex, const char* delimiter, const char* leftBracket, const char* rightBracket)
{
	unsigned n = kArray_size(expr->NodeList);
	unsigned i;
	if(leftBracket) {
		JSBuilder_EmitString(kctx, builder, leftBracket, "", "");
	}
	for(i = beginIndex; i < n;) {
		SUGAR VisitNode(kctx, builder, kNode_At(expr, i), thunk);
		if(++i < n) {
			JSBuilder_EmitString(kctx, builder, delimiter, "", "");
		}
	}
	if(rightBracket) {
		JSBuilder_EmitString(kctx, builder, rightBracket, "", "");
	}
	return true;
}

static void JSBuilder_ConvertAndEmitMethodName(KonohaContext *kctx, KBuilder *builder, kNode *expr, void *thunk, kNode *receiver, kMethod *mtd)
{
	KClass *globalObjectClass = KLIB kNameSpace_GetClassByFullName(kctx, kNode_ns(expr), "GlobalObject", 12, NULL);
	kbool_t isGlobal = (KClass_(receiver->attrTypeId) == globalObjectClass || receiver->attrTypeId == KType_NameSpace);
	const char *methodName = KSymbol_text(mtd->mn);
	if(receiver->attrTypeId == KType_NameSpace) {
		if(mtd->mn == KMethodName_("import")) {
			kString *packageNameString = (kString *)kNode_At(expr, 2)->ObjectConstValue;
			kNameSpace *ns = (kNameSpace *)receiver->ObjectConstValue;
			JSBuilder_importPackage(kctx, ns, packageNameString, expr->TermToken->uline);
			JSBuilder_EmitString(kctx, builder, "//", "", "");
			return;
		}
	}
	if(receiver->attrTypeId == KType_System && methodName[0] == 'p') {
		JSBuilder_EmitString(kctx, builder, LOG_FUNCTION_NAME, "", "");
	}else if(strcmp(KSymbol_text(mtd->mn), "new") == 0) {
		JSBuilder_EmitString(kctx, builder, "new ", KClass_text(KClass_(receiver->attrTypeId)), "");
	}else if(strcmp(KSymbol_text(mtd->mn), "newList") == 0) {
	}else if(strcmp(KSymbol_text(mtd->mn), "newArray") == 0) {
		JSBuilder_EmitString(kctx, builder, "new Array", "", "");
	}else{
		if(!isGlobal) {
			if(receiver->node == KNode_Null) {
				JSBuilder_EmitString(kctx, builder, KClass_text(KClass_(receiver->attrTypeId)), "", "");
			}
			else{
				SUGAR VisitNode(kctx, builder, receiver, thunk);
			}
		}
		switch(KSymbol_prefixText_ID(mtd->mn)) {
		case kSymbolPrefix_GET:
			if(kArray_size(expr->NodeList) > 2) {
				JSBuilder_VisitNode(kctx, builder, kNode_At(expr, 2), thunk, "[", "]");
			}
			else{
				if(!isGlobal) {
					JSBuilder_EmitString(kctx, builder, ".", "", "");
				}
				JSBuilder_EmitString(kctx, builder, methodName, "", "");
			}
			break;
		case kSymbolPrefix_SET:
			if(kArray_size(expr->NodeList) > 3) {
				JSBuilder_VisitNode(kctx, builder, kNode_At(expr, 2), thunk, "[", "]");
			}else{
				if(isGlobal) {
					JSBuilder_EmitString(kctx, builder, "var ", "", "");
				}
				else{
					JSBuilder_EmitString(kctx, builder, ".", "", "");
				}
			}
			JSBuilder_EmitString(kctx, builder, methodName, " = ", "");
			break;
		case kSymbolPrefix_TO:
			break;
		default:
			if(!isGlobal){
				JSBuilder_EmitString(kctx, builder, ".", "", "");
			}
			JSBuilder_EmitString(kctx, builder, methodName, "", "");
			break;
		}
	}
}

static kbool_t JSBuilder_VisitMethodCallNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	kMethod *mtd = CallNode_getMethod(node);
	kbool_t isArray = false;

	if(kArray_size(node->NodeList) == 2 && KMethodName_isUnaryOperator(kctx, mtd->mn)) {
		JSBuilder_EmitString(kctx, builder, KMethodName_Fmt2(mtd->mn), "(");
		SUGAR VisitNode(kctx, builder, kNode_At(node, 1), thunk);
		JSBuilder_EmitString(kctx, builder, ")", "", "");
	}
	else if(KMethodName_isBinaryOperator(kctx, mtd->mn)) {
		JSBuilder_VisitNodeParams(kctx, builder, node, thunk, 1, KSymbol_text(mtd->mn),"(", ")");
	}
	else{
		kNode *receiver = kNode_At(node, 1);
		/*if(mtd == jsBuilder->visitingMethod) {
			JSBuilder_EmitString(kctx, builder, "arguments.callee", "", "");
		}
		else*/ if(strcmp(KSymbol_text(mtd->mn), "newList") == 0) {
			isArray = true;
		}
		else {
			JSBuilder_ConvertAndEmitMethodName(kctx, builder, node, thunk, receiver, mtd);
		}
		switch(KSymbol_prefixText_ID(mtd->mn)) {
		case kSymbolPrefix_GET:
		case kSymbolPrefix_TO:
			break;
		case kSymbolPrefix_SET:
			if(kArray_size(node->NodeList) > 3) {
				SUGAR VisitNode(kctx, builder, kNode_At(node, 3), thunk);
			}else{
				SUGAR VisitNode(kctx, builder, kNode_At(node, 2), thunk);
			}
			break;
		default:
			JSBuilder_VisitNodeParams(kctx, builder, node, thunk, 2, ", ", isArray ? "[" : "(", isArray ? "]" : ")");
			break;
		}
	}
	return true;
}

static kbool_t JSBuilder_VisitAndNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_VisitNodeParams(kctx, builder, node, thunk, 1, " && ", "(", ")");
	return true;
}

static kbool_t JSBuilder_VisitOrNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_VisitNodeParams(kctx, builder, node, thunk, 1, " || ", "(", ")");
	return true;
}

static kbool_t JSBuilder_VisitAssignNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_VisitNodeParams(kctx, builder, node, thunk, 1, " = ", NULL, NULL);
	return true;
}

static kbool_t JSBuilder_VisitDoneNode(KonohaContext *kctx, KBuilder *builder, kNode *node, void *thunk)
{
	JSBuilder_EmitString(kctx, builder, "/*FIXME*/ /* this may be bug. */", "", "");
	return true;
}

static kbool_t JSBuilder_VisitFunctionNode(KonohaContext *kctx, KBuilder *builder, kNode *expr, void *thunk)
{
	abort();/*FIXME*/
	return true;
}


static void compileAllDefinedMethods(KonohaContext *kctx)
{
	KRuntime *share = kctx->share;
	size_t i;
	for(i = 0; i < kArray_size(share->GlobalConstList); i++) {
		kObject *o = share->GlobalConstList->ObjectItems[i];
		if(kObject_class(o) == KClass_NameSpace) {
			kNameSpace *ns = (kNameSpace  *) o;
			size_t j;
			for(j = 0; j < kArray_size(ns->methodList_OnList); j++) {
				kMethod *mtd = ns->methodList_OnList->MethodItems[j];
				if(IS_NameSpace(mtd->LazyCompileNameSpace)) {
					KLIB kMethod_DoLazyCompilation(kctx, mtd, NULL, HatedLazyCompile|CrossCompile);
				}
			}
		}
	}
}

static void JSBuilder_EmitExtendFunctionCode(KonohaContext *kctx, KBuilder *builder)
{
	JSBuilder *self = (JSBuilder *)builder;
	JSBuilder_EmitNewLineWith(kctx, builder, "var __extends = this.__extends || function (d, b) {");
	self->indent++;
	JSBuilder_EmitNewLineWith(kctx, builder, "function __() { this.constructor = d; }");
	JSBuilder_EmitNewLineWith(kctx, builder, "__.prototype = b.prototype;");
	JSBuilder_EmitNewLineWith(kctx, builder, "d.prototype = new __();");
	self->indent--;
	JSBuilder_EmitNewLineWith(kctx, builder, "}");
}

static kbool_t JSBuilder_VisitClassFields(KonohaContext *kctx, KBuilder *builder, KClass *kclass)
{
	kushort_t i;
	KClassField *field = kclass->fieldItems;
	kObject *constList = kclass->defaultNullValue;
	for(i = 0; i < kclass->fieldsize; ++i) {
		JSBuilder_EmitString(kctx, builder, "this.", KSymbol_text(field[i].name), " = ");
		if(KType_Is(UnboxType, field[i].attrTypeId)) {
			JSBuilder_EmitUnboxConstValue(kctx, builder, KClass_(field[i].attrTypeId), constList->fieldUnboxItems[i]);
		}else{
			JSBuilder_EmitConstValue(kctx, builder, constList->fieldObjectItems[i]);
		}
		JSBuilder_EmitNewLineWith(kctx, builder, ";");
	}
	return true;
}

static void JSBuilder_EmitMethodHeader(KonohaContext *kctx, KBuilder *builder, kMethod *mtd)
{
	KClass *kclass = KClass_(mtd->typeId);
	KBuffer wb;
	KLIB KBuffer_Init(&(kctx->stack->cwb), &wb);
	kParam *params = kMethod_GetParam(mtd);
	unsigned i;
	if(mtd->typeId == KType_NameSpace) {
		KLIB KBuffer_printf(kctx, &wb, "var %s%s = function(", KMethodName_Fmt2(mtd->mn));
	}else if(strcmp(KSymbol_text(mtd->mn), "new") == 0) {
		KLIB KBuffer_printf(kctx, &wb, "function %s(", KClass_text(kclass));
	}else{
		compileAllDefinedMethods(kctx);
		if(kMethod_Is(Static, mtd)) {
			KLIB KBuffer_printf(kctx, &wb, "%s.%s%s = function(", KClass_text(KClass_(mtd->typeId)), KMethodName_Fmt2(mtd->mn));
		}else{
			KLIB KBuffer_printf(kctx, &wb, "%s.prototype.%s%s = function(", KClass_text(KClass_(mtd->typeId)), KMethodName_Fmt2(mtd->mn));
		}
	}
	for(i = 0; i < params->psize; ++i) {
		if(i != 0) {
			KLIB KBuffer_printf(kctx, &wb, ", ");
		}
		KLIB KBuffer_printf(kctx, &wb, "%s", KSymbol_text(params->paramtypeItems[i].name));
	}
	KLIB KBuffer_printf(kctx, &wb, ")");
	JSBuilder_EmitString(kctx, builder, KLIB KBuffer_text(kctx, &wb, EnsureZero), "", "");
	KLIB KBuffer_Free(&wb);
}

static void JSBuilder_EmitClassHeader(KonohaContext *kctx, KBuilder *builder, KClass *kclass)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	KClass *base = KClass_(kclass->superTypeId);
	if(base->typeId != KType_Object) {
		JSBuilder_EmitExtendFunctionCode(kctx, builder);
		JSBuilder_EmitString(kctx, builder, "var ", KClass_text(kclass), " = (function (_super) {");
	}else{
		JSBuilder_EmitString(kctx, builder, "var ", KClass_text(kclass), " = (function () {");
	}
	JSBuilder_EmitNewLineWith(kctx, builder, "");
	jsBuilder->indent++;
	if(base->typeId != KType_Object){
		JSBuilder_EmitString(kctx, builder, "__extends(", KClass_text(kclass), ", _super);");
		JSBuilder_EmitNewLineWith(kctx, builder, "");
	}
}

static void JSBuilder_EmitClassFooter(KonohaContext *kctx, KBuilder *builder, KClass *kclass)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	KClass *base = KClass_(kclass->superTypeId);
	JSBuilder_EmitString(kctx, builder, "return ", KClass_text(kclass), ";");
	JSBuilder_EmitNewLineWith(kctx, builder, "");
	jsBuilder->indent--;
	if(base->typeId != KType_Object) {
		JSBuilder_EmitString(kctx, builder, "})(", KClass_text(base), ");");
		JSBuilder_EmitNewLineWith(kctx, builder, "");
	}else{
		JSBuilder_EmitNewLineWith(kctx, builder, "})();");
	}
}



static void JSBuilder_Init(KonohaContext *kctx, KBuilder *builder, kMethod *mtd)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	kbool_t isConstractor = false;
	jsBuilder->visitingMethod = mtd;
	jsBuilder->isIndentEmitted = false;
	jsBuilder->indent = 0;
	KLIB KBuffer_Init(&jsBuilder->buffer, &jsBuilder->jsCodeBuffer);

	KClass *kclass = KClass_(mtd->typeId);
	KClass *base  = KClass_(kclass->superTypeId);

	if(mtd->mn != 0) {
		KLIB kMethod_SetFunc(kctx, mtd, NULL);
		if(strcmp(KSymbol_text(mtd->mn), "new") == 0) {
			isConstractor = true;
		}
		if(isConstractor) {
			JSBuilder_EmitClassHeader(kctx, builder, kclass);
		}
		JSBuilder_EmitMethodHeader(kctx, builder, mtd);
		JSBuilder_EmitNewLineWith(kctx, builder, " {");
		jsBuilder->indent++;
		if(isConstractor) {
			if(base->typeId != KType_Object) {
				JSBuilder_EmitNewLineWith(kctx, builder, "_super.call(this);");
			}
			JSBuilder_VisitClassFields(kctx, builder, kclass);
		}
	}else{
		compileAllDefinedMethods(kctx);
	}
}

static void JSBuilder_Free(KonohaContext *kctx, KBuilder *builder, kMethod *mtd)
{
	JSBuilder *jsBuilder = (JSBuilder *)builder;
	if(mtd->mn != 0) {
		jsBuilder->indent--;
		JSBuilder_EmitNewLineWith(kctx, builder, "}");
		if(strcmp(KSymbol_text(mtd->mn), "new") == 0) {
			KClass *kclass = KClass_(mtd->typeId);
			JSBuilder_EmitClassFooter(kctx, builder, kclass);
		}
	}
	const char* jsSrc = KLIB KBuffer_text(kctx, &jsBuilder->jsCodeBuffer, EnsureZero);
#ifdef HAVE_LIBV8
	kbool_t isCompileOnly = KonohaContext_Is(CompileOnly, kctx);
#else
#define isCompileOnly (1)
#endif
	if(isCompileOnly) {
		printf("%s\n", jsSrc);
	}
#ifdef HAVE_LIBV8
	else {
		v8::Handle<v8::String> v8Src = v8::String::New(jsSrc);
		v8::Handle<v8::Script> v8Script = v8::Script::Compile(v8Src);
		v8::Handle<v8::Value> result = v8Script->Run();
		v8::String::AsciiValue resultStr(result);
	}
#endif
	KLIB KBuffer_Free(&jsBuilder->jsCodeBuffer);
}

static struct KVirtualCode* V8_GenerateVirtualCode(KonohaContext *kctx, kMethod *mtd, kNode *block, int option)
{
	INIT_GCSTACK();
	JSBuilder builderbuf = {}, *builder = &builderbuf;
	kNameSpace *ns = kNode_ns(block);
	builder->common.api = ns->builderApi;
	builder->visitingMethod = mtd;
	JSBuilder_Init(kctx, (KBuilder *)builder, mtd);

	SUGAR VisitNode(kctx, (KBuilder *)builder, block, NULL);

	JSBuilder_Free(kctx, (KBuilder *)builder, mtd);
	RESET_GCSTACK();
	return NULL;
}

static KMETHOD KMethodFunc_RunVirtualMachine(KonohaContext *kctx, KonohaStack *sfp)
{
}

static void V8_SetMethodCode(KonohaContext *kctx, kMethodVar *mtd, KVirtualCode *vcode, KMethodFunc func)
{
	KLIB kMethod_SetFunc(kctx, mtd, func);
	mtd->vcode_start = vcode;
}

static KMethodFunc V8_GenerateMethodFunc(KonohaContext *kctx, KVirtualCode *vcode)
{
	return KMethodFunc_RunVirtualMachine;
}

static struct KVirtualCode* GetDefaultBootCode(void)
{
	return NULL;
}

static void InitStaticBuilderApi(struct KBuilderAPI *builderApi)
{
	builderApi->target = "JavaScript";
#define DEFINE_BUILDER_API(NAME) builderApi->visit##NAME##Node = JSBuilder_Visit##NAME##Node;
	KNodeList(DEFINE_BUILDER_API);
#undef DEFINE_BUILDER_API
	builderApi->GenerateVirtualCode = V8_GenerateVirtualCode;
	builderApi->GenerateMethodFunc  = V8_GenerateMethodFunc;
	builderApi->SetMethodCode        = V8_SetMethodCode;
	//builderApi->RunVirtualMachine   = KonohaVirtualMachine_Run;
}

static struct KBuilderAPI* GetDefaultBuilderAPI(void)
{
	static struct KBuilderAPI builderApi = {};
	if(builderApi.target == NULL) {
		InitStaticBuilderApi(&builderApi);
	}
	return &builderApi;
}

static void V8_DeleteVirtualMachine(KonohaContext *kctx)
{
}

// -------------------------------------------------------------------------

kbool_t LoadJavaScriptModule(KonohaFactory *factory, ModuleType type)
{
	static KModuleInfo ModuleInfo = {
		"JavaScript", K_VERSION, 0, "JavaScript",
	};
	factory->VirtualMachineInfo            = &ModuleInfo;
	factory->GetDefaultBootCode            = GetDefaultBootCode;
	factory->GetDefaultBuilderAPI          = GetDefaultBuilderAPI;
	factory->DeleteVirtualMachine          = V8_DeleteVirtualMachine;
#ifdef HAVE_LIBV8
	globalJSContext = new JSContext();
#endif

	return true;
}

} /* extern "C" */
