#include <iostream>
#include <fstream>
#include "ocilib.hpp"

using namespace std;
using namespace ocilib;

int main3(void) {

  ofstream of;
  of.open("out.tsv");

  try {
    Environment::Initialize();

    Connection con("localhost", "scott", "tiger");

    Statement st(con);
    st.Execute("select * from APEX_040000.WWV_FLOW_STEP_ITEMS");
    Resultset rs = st.GetResultset();
    auto colCount = rs.GetColumnCount();
    cout << colCount << '\n';
    for (int i=1; i <=colCount; i++){
      cout << rs.GetColumn(i).GetName()  << "|" << rs.GetColumn(i).GetFullSQLType() << '\n';
    }
    while (rs++) {
      
      for (int i = 1; i <= colCount; i++){
        of << rs.Get<ostring>(i);
        if (i < colCount){
          of << "\t";
        }
      }
      of << "\n";

    }

    std::cout << "=> Total fetched rows : " << rs.GetCount() << std::endl;

  }
  catch (std::exception &ex) {
    std::cout << ex.what() << std::endl;
  }

  Environment::Cleanup();

  of.close();

  return EXIT_SUCCESS;
}
