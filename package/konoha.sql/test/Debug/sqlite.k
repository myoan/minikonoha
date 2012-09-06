K.import("konoha");
K.import("konoha.sql");

void main() {
	Connection con = new Connection("testdb.sqlite");
//	Connection con = new Connection("../package/konoha.sql/test/Debug/testdb.sqlite");
	ResultSet r = con.query("SELECT * FROM t_menu");
	while(r.next()) {
		p(r.getInt("id"));
		p(r.getString("name"));
	}
	con.close();
}

main();
