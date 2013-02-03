/****************************************************************************
 * Copyright (c) 2013, the Konoha project authors. All rights reserved.
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

Import("Type.Json");
Import("MiniKonoha.Map");
Import("JavaStyle.Object");
Import("Syntax.Null");
Import("Syntax.GlobalVariable");
Import("Syntax.CStyleWhile");
Import("JavaScript.String");
Import("JavaScript.Array");
Import("JavaScript.Regexp");
Import("dscript.subproc");
Import("Lib.Mecab");
Import("MiniKonoha.Sql");
Import("MiniKonoha.Syntax"); // for debug
Import("Deos.DCaseCloud");
Import("MiniKonoha.NameSpace");
Import("Syntax.JavaStyleClass");

API repo = new API("dcasedb", "dcasedb");

class RiskItem {
	String keyword;
	String[] riskSet;
	RiskItem() {
		this.keyword = "";
		this.riskSet = new String[];
	};
	void dump() {
		System.p("keyword: " + this.keyword);
		int i = 0;
		while (i < this.riskSet.getSize()) {
			System.p("risk: " + this.riskSet[i]);
			i = i + 1;
		}
	}
}

class RiskChecker {

	Connection con;
	DCaseArgument argument;
	String input;
	RiskItem[] risks;
	boolean hasRisk;

	RiskChecker(String usr, String pass, String input) {
		con = new Connection("mysql://" + usr + ":" + pass + "@localhost:3306");
		con.query("USE alertme_moc");
		this.input = input;
		this.risks = new RiskItem[];
		this.hasRisk = false;
	}
	
	@Private String[] _getNoun(String input) {
		Tagger m = new Tagger();
		String result = m.parse(input);
		String[] strs = result.split(/\n/);
		boolean isPreWordNoun = false;
		String preNoun = "";
		int i = 0;
		String[] ret = new String[];
		while(i < strs.getSize()) {
			String line = strs[i];
			if (line.search(/名詞/) > -1) {
				String[] noun = line.split(/[ \t]+/);
				ret.add(noun[0]);
				if (isPreWordNoun) {
					ret.add(preNoun + noun[0]);
				}
				preNoun = noun[0];
				isPreWordNoun = true;
			}
			else {
				isPreWordNoun = false;
				preNoun = "";
			}
			i =  i + 1;
		}
		return ret;
	}

	@Private DCaseArgument _createArgument() {
		int i = 0;
		DCaseArgument args = global.repo.CreateArgument(this.input);
		int args_id = args.argument_id;
		DBNode root = global.repo.GetArgument(args_id).GetRoot();
		DBNode strategy = global.repo.CreateDBNode(Json.parse("{\"type\": \"Strategy\", \"description\": \"リスクに分割して議論\"}"));
		global.repo.InsertLink(args_id, root, strategy);
		System.p("Argument id: " + args_id);

		while (i < this.risks.getSize()) {
			RiskItem r = this.risks[i];
			DBNode kw_goal;
			kw_goal = global.repo.CreateDBNode(Json.parse("{\"type\": \"Goal\", \"description\": \"" + r.keyword+ "\"}"));
			global.repo.InsertLink(args_id, strategy, kw_goal);
			int j = 0;
			while (j < r.riskSet.getSize()) {
				DBNode risk_goal = global.repo.CreateDBNode(Json.parse("{\"type\": \"Goal\", \"description\": \"" + r.riskSet[j] + "\"}"));
				global.repo.InsertLink(args_id, kw_goal, risk_goal);
				j = j + 1;
			}
			i = i + 1;
		}
		return args;
	}

	@Private String[] _getRisks(String input) {
		String[] strs = this._getNoun(input);
		String[] ret = new String[];
		int i = 0;
		boolean start_flag = true;
		while (i < strs.getSize()) {
			String noun = strs[i];
			ResultSet rs = this.con.query("select words FROM risk_expression, risk_keyword, risk_kw_relation where risk_expression.id = risk_kw_relation.risk_id AND risk_keyword.id = risk_kw_relation.kw_id AND risk_keyword.keyword = '" + noun + "'");
			if (rs.next()) {
				RiskItem r = new RiskItem();
				r.keyword = noun;
				r.riskSet.add(rs.getString("words"));
				while (rs.next()) {
					r.riskSet.add(rs.getString("words"));
				}
				r.dump();
				this.risks.add(r);
			}
			i = i + 1;
		}
		if (this.risks.getSize() > 0) {
			this.hasRisk = true;
		}
		return ret;
	}

	@Private DCaseArgument createArgument() {
		if (this.risks.getSize() == 0) {
			return null;
		}
		return this._createArgument();
	}

	@Public void checkRisk() {
		this._getRisks(this.input);
	}

	@Public boolean hasRisk() {
		System.p("hasRisk: " + this.hasRisk);
		return this.hasRisk;
	}

	@Public boolean isArgued() {
		Json result = global.repo.search(this.input);
		if (result.toString() == "[]") return false;
		int i = 0;
		int args_id = -1;
		while (i < result.getSize()) {
			args_id = result[i].getString("argument_id").toint();
			i = i + 1;
		}
		System.p(args_id);
		this.argument = global.repo.GetArgument(args_id);
		return true;
	}

	@Public boolean isAssured() {
		System.p("IsAssured: " + this.argument.IsAssured());
		return this.argument.IsAssured();
	}

	@Public void feedBack() {
	}
}

@Public void System.exit() {
	assert(false);
}

void main() {
	DCaseArgument args;
	//String input = "RAMディスクにデータをコピーする";
	String input = "kill -9 12345";
	RiskChecker checker = new RiskChecker("dcasedb", "dcasedb", input);
	System.p("Input: " + input + "\n");
	if (checker.isArgued()) {
		if (checker.isAssured()) {
			// run script
		}
		System.exit();
	}
	checker.checkRisk();
	if (!checker.hasRisk()) {
		// run script
	}
	else {
		args = checker.createArgument();
	}
}

main();

