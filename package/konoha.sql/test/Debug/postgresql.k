K.import("konoha");
K.import("konoha.sql");

void main() {
	Connection con = new Connection("postgresql://dbname = postgres");
	ResultSet r = con.query("select * from pg_database");
	//while(r.next()) {
	//	p(r.getString("SCHEMA_NAME"));
	//}
	con.close();
}

main();
