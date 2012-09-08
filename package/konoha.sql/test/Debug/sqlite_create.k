K.import("konoha");
K.import("konoha.sql");

void main() {

	Connection con = new Connection("sqlite://tabletestdb.sqlite");

	con.query("CREATE TABLE t_test (id INTEGER, name VARCHAR(16));");
	con.query("INSERT INTO t_test (id, name) VALUES(1 , 'kuramitu'); ");
	con.query("INSERT INTO t_test (id, name) VALUES(2 , 'hamagami'); ");
	con.query("INSERT INTO t_test (id, name) VALUES(3 , 'ochiai'); ");
	
	ResultSet r = con.query("SELECT * FROM t_test;");
	while(r.next()) {
		p(r.getInt("id"));
		p(r.getString("name"));
	}
	con.close();
}

main();
