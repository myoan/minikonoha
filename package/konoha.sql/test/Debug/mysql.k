K.import("konoha");
K.import("konoha.sql");

String USER_NAME = "root"; // Enter Your MySQL UserName
String PASSWD = "masaki89";     // Enter Your MySQL Password

void main() {
	Connection con = new Connection("mysql://" + USER_NAME + ":" + PASSWD + "@localhost:3306/information_schema");
	ResultSet r = con.query("SELECT * FROM INFORMATION_SCHEMA.SCHEMATA");

	while(r.next()) {
		p(r.next());
		p(r.getString("SCHEMA_NAME"));
		}
	con.close();
}

main();
