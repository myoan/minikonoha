// written by kimio kuramitsu

import("MiniKonoha.Syntax");

Expr TypeCheckTrue(Stmt stmt, Expr expr, Gamma gma, int typeid) {
	return stmt.message(ErrTag, "true was redefined");
}

addTypeCheck("true", TypeCheckTrue);
true;


