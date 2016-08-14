#include "oradump.hpp"
#include "base64.h"
#include <regex>

using namespace std;
using namespace ocilib;
using namespace boost::filesystem;

/*  
  nameless namespace to share 2 vars in different files
*/
namespace {

  static std::regex doubleQuotes("\"");

  void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty())
      return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
  }

  auto readFile = [](string& f, string& dest)->bool {
    string line;
    stringstream ss;
    ifstream ifs(f, ios::in | ios::binary | ios::ate);
    streampos size;
    if (ifs.is_open()) {
      ifs.seekg(0, ios::beg);
      while (getline(ifs, line)) {
        ss << line << "\n";
      }

      dest.assign(ss.str());
      return true;
    }

    return false;
  };

  auto createFolder = [](const char* f)->int {
    boost::filesystem::path dir(f);
    if (boost::filesystem::create_directory(dir)) {
      return 0;
    }
    return 1;
  };

  auto convertTypes = [](string& sqlType)->void {
    if (sqlType.find("NUMBER") != string::npos) {
      if (sqlType.find("(") == string::npos){
        sqlType = "INT";
      }
      else {
        replaceAll(sqlType, "NUMBER", "NUMERIC");
      }
    }

    replaceAll(sqlType, "DATE", "DATETIME");
    replaceAll(sqlType, "VARCHAR2", "VARCHAR");
  };

  auto createDatabase = [](ostream& o, string& d)->void {
    o << "use master\nGO\n\n"
      << "if db_id('" << d << "') is null\n"
      << "begin\n"
      << "\tcreate database " << d << "\n"
      << "end\nGO\n\n"
      << "use " << d << "\nGO\n\n";
  };

  auto createSchema = [](ostream& o, string& s)->void {
    o << "if not exists( \n"
      << "\tselect schema_name \n"
      << "\tfrom information_schema.schemata \n"
      << "\twhere schema_name = '" << s << "') \n"
      << "begin \n"
      << "\texec sp_executesql N'CREATE SCHEMA " << s << "'\n"
      << "end\nGO\n\n";
  };

  auto constructTable = [](ostream& of, ocilib::Resultset& rs, string& schema, string& basename)->void {
    auto colCount = rs.GetColumnCount();
    string tablename = "[" + schema + "].[" + basename + "]";

    of << "if object_id(N'" + tablename + "', N'U') is null\nbegin\n";
    of << "\tcreate table " << tablename << " (\n";

    //map<int, string> cols;
    for (int i = 1; i <= colCount; i++){
      auto sqlType = rs.GetColumn(i).GetFullSQLType();
      string colName = rs.GetColumn(i).GetName();

      //Translate Oracle to SQL types
      convertTypes(sqlType);

      //Lowercase everything
      std::transform(colName.begin(), colName.end(), colName.begin(), ::tolower);
      std::transform(sqlType.begin(), sqlType.end(), sqlType.begin(), ::tolower);

      of << "\t" << colName << " " << sqlType << (i < colCount ? "," : "") << '\n';
    }
    of << "\t) " << "\n"
      << "end\n";
    of << "else\n\t" << "truncate table " << tablename << "\nGO\n\n";
  };

  auto addBcp = [](ostream& of, string& server, string& database, string& schema, string& basename)->void{
    //-t, uses comma, keep default which is a \t
    of << "/*bcp example for\n\t" << basename << "\n*/\n" << "--" << "bcp " << database << "." << schema << "." << basename << " in ../out/" << basename << ".tsv" << " -c -S " << server << " -T -E\n\n";
  };

  auto addSqlCmd = [](ostream& of, string& server, string& basename)->void {
    of << "/*sqlcmd example for\n\t" << basename << "\n*/\n" << "--" << "sqlcmd -S " << server << "-E -i" << " sql/" << basename << ".sql\n\n";
  };

  auto prepareBulkPrereq = [](ocilib::Resultset& rs, string& filename)->int {

    boost::filesystem::path p(filename);

    string server = "clcwcdcdap001.nyumc.org";
    string database = "hack_dlar";
    string schema = "dbo";
    string outname("sql/");
    string basename = p.stem().generic_string();
    outname.append(basename + ".sql");
    ofstream of(outname);

    //Get the right schema
    if (basename.find("_hub_") != string::npos){
      schema = "hub";
    }
    if (basename.find("_aor_") != string::npos){
      schema = "aor";
    }

    //Create the database
    createDatabase(of, database);

    //Create the schema
    createSchema(of, schema);

    //Create the table
    constructTable(of, rs, schema, basename);

    //Add create table etc for SQL Server
    addSqlCmd(of, server, basename);

    //Add bcp command for bulk loading to SQL Server
    addBcp(of, server, database, schema, basename);

    of.close();

    return 0;

  };

  auto processFile = [](const unique_ptr<dumper>& d)->int {

    Semaphore_waiter_notifier w(thread_limiter);

    auto logger = spdlog::get("logger");
    stringstream ss;

    string userDefinedQuery;
    int ok = readFile(d->filename, userDefinedQuery);

    if (!ok){
      return 1;
    }

    boost::filesystem::path p(d->filename);
    string basename = p.stem().generic_string();

    string outFile("out/" + basename + ".tsv");
    
    ofstream of;
    of.open(outFile);

    Connection con(oraService, oraUser, oraPass);

    Statement st(con);
    
    try {

      st.Execute(userDefinedQuery);
    
      Resultset rs = st.GetResultset();
    
      auto colCount = rs.GetColumnCount();

      prepareBulkPrereq(rs, d->filename);

      //start timer
      auto t1 = std::chrono::high_resolution_clock::now();

      std::map<int, std::string> columns;
      //write header
      if (includeHeader){
        for (int i = 1; i <= colCount; i++){
          auto sqlType = rs.GetColumn(i).GetFullSQLType();
          columns[i] = sqlType;
          string colName = rs.GetColumn(i).GetName();
          of << colName;
          if (i < colCount){
            of << "\t";
          }
        }
        of << "\n";
      }      

      while (rs++) {

        for (int i = 1; i <= colCount; i++){

          auto cell = rs.Get<ostring>(i);

          /**
          Handle CLOB
          **/
          if (columns[i] == "CLOB"){
            cell = base64_encode((const unsigned char*)cell.c_str(), cell.size());
          }

          /*
          //put double quotes around strings
          auto c = cols.find(i);
          std::size_t isStr = c->second.find("VARCHAR");
          if (isStr != string::npos) {
          cell = "\"" + cell + "\"";
          }
          */

          //handle quotes
          cell = std::regex_replace(cell, doubleQuotes, "");

          std::replace(cell.begin(), cell.end(), '\r', '\s');
          std::replace(cell.begin(), cell.end(), '\n', '\s');

          of << cell;
          if (i < colCount){
            of << "\t";
          }
        }
        of << "\n";

      }

      ss << basename << " => total fetched rows : " << rs.GetCount();
      logger->info(ss.str());
    
      auto t2 = std::chrono::high_resolution_clock::now();
      ss.str("");
      ss << basename << " => elaspsed "
        << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
        << " milliseconds\n";
    
      logger->info(ss.str());
    
      //of.close();
      //con.Close();
    }
    catch (ocilib::Exception &ex){
      ss.str("");
      
      ss << basename << " => " << ex.GetOracleErrorCode() << " " << ex.GetMessageA();
      logger->error(ss.str());
    }
    
    of.close();
    con.Close();

    return 0;

  };

}