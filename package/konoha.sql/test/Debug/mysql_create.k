K.import("konoha");
K.import("konoha.sql");

String USER_NAME = "root"; // Enter Your MySQL UserName
String PASSWD = "ybkkbhc";     // Enter Your MySQL Password

void main() {

	Connection con = new Connection("mysql://" + USER_NAME + ":" + PASSWD + "@localhost:3306/information_schema");

	con.query("CREATE database konoha;");
	con.query("use konoha;");
	con.query("create table people(name text not null, age int, village text);");
	con.query("INSERT INTO people (name, age, village) VALUES('naruto', 24, 'KonohaGakure');");
	con.query("INSERT INTO people (name, age, village) VALUES('sakura', 24, 'KonohaGakure');");
	con.query("INSERT INTO people (name, age, village) VALUES('sasuke', 24, 'KonohaGakure');");
	con.query("INSERT INTO people (name, age, village) VALUES('gaara', 24, 'SunaGakure');");

	ResultSet r = con.query("SELECT * FROM konoha.people");

	while(r.next()) {
		p(r.getString("name"));
		p(r.getInt("age"));
		p(r.getString("area"));
	}

	con.close();
}

main();
