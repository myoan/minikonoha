K.import("konoha");
K.import("konoha.sql");

String USER_NAME = "root"; // Enter Your MySQL UserName
String PASSWD = "masaki89";     // Enter Your MySQL Password

void main() {

	Connection con = new Connection("mysql://" + USER_NAME + ":" + PASSWD + "@localhost:3306/information_schema");

	con.query("CREATE database foo;");
	con.query("use foo;");
	con.query("create table jpzipcode(oldpost text, newpost text not null, pref text, area text, addr text not null );");
	con.query("desc users");
	con.query("INSERT INTO jpzipcode(oldpost,newpost,pref,area,addr) VALUES('154','154-0002','Tokyo','Setagaya','simo');");
	con.query("INSERT INTO jpzipcode (oldpost,newpost, pref,area,addr) VALUES('468','468-0039', 'Aichi','nagoya','nishi');");

	ResultSet r = con.query("SELECT * FROM foo.jpzipcode");

	while(r.next()) {
		p(r.getString("oldpost"));
	}

	con.close();
}

main();
