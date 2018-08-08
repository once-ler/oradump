#include <iostream>
#include <boost/program_options.hpp>
#include "exports.hpp"

using namespace std;
using namespace boost::filesystem;

auto setupLogging = []()->void {
  
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>("oradump", "txt",0,0));
  auto combined_logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
  combined_logger->set_pattern("[%Y-%d-%m %H:%M:%S:%e] [%l] [thread %t] %v");
  spdlog::register_logger(combined_logger);

};

auto cleanup = []()->void {
  Environment::Cleanup();
  spdlog::get("logger")->flush();
  spdlog::drop_all();
};

void err_handler(OCI_Error *err) {
  printf(
    "code  : ORA-%05i\n"
    "msg   : %s\n"
    "sql   : %s\n",
    OCI_ErrorGetOCICode(err),
    OCI_ErrorGetString(err),
    OCI_GetSql(OCI_ErrorGetStatement(err))
    );

  spdlog::get("logger")->error("code: ORA-{0:05d} msg: {1:s} sql: {2:s}",
    OCI_ErrorGetOCICode(err),
    OCI_ErrorGetString(err),
    OCI_GetSql(OCI_ErrorGetStatement(err)));
  spdlog::get("logger")->flush();
}

// Make sure TNS_ADMIN env is pointing to correct tnsnames.ora file.
auto start = []()->int {

  createFolder(SQL_FOLDER);
  createFolder(OUT_FOLDER);
  setupLogging();
  
  if (!OCI_Initialize(err_handler, NULL, OCI_ENV_THREADED))
    return EXIT_FAILURE;
  
  //initialize orilib
  Environment::Initialize(Environment::Threaded);

  //user passed in filename, ignore directory
  if (!filename.empty()){
    auto d = make_unique<dumper>(filename);
    processFile(d);
    cleanup();
    return 0;
  }
  //user passed in directory
  if (!directory.empty()){
    path p(directory);
    directory_iterator end_itr;
    dumperVector vi;
    vector<unique_ptr<dumper>> v;
    for (directory_iterator itr(p); itr != end_itr; ++itr) {
      // If it's not a directory, list it. If you want to list directories too, just remove this check.
      if (is_regular_file(itr->path())) {
        string current_file = itr->path().string();
        v.push_back(unique_ptr<dumper>(new dumper(current_file)));
      }
    }
    parallel_for_each(v.begin(), v.end(), processFile);
  }

  cleanup();
  return 0;

};

int main(int argc, char* argv[]) {

  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  
  desc.add_options()
    ("help", "There are 2 parameters that you can pass")
    ("file", po::value<string>(), "file that contains Oracle scripts")
    ("dir", po::value<string>(), "directory where one or more files contain Oracle scripts")
    ("sid", po::value<string>(), "Oracle service name or server instance")
    ("user", po::value<string>(), "Oracle username")
    ("pass", po::value<string>(), "Oracle password")
    ("header", po::value<bool>(), "Include header in output");


  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  // Loop through each argument and print its number and value
  for (int nArg = 0; nArg < argc; nArg++)
    cout << "Parameter: " << nArg << " " << argv[nArg] << endl;

  if (vm.count("file")) {
    filename = (vm["file"].as<string>());
  }

  if (vm.count("dir")) {
    directory = (vm["dir"].as<string>());
  }

  if (vm.count("sid")) {
    oraService = (vm["sid"].as<string>());
  }

  if (vm.count("user")) {
    oraUser = (vm["user"].as<string>());
  }
  if (vm.count("pass")) {
    oraPass = (vm["pass"].as<string>());
  }
  if (vm.count("header")) {
    includeHeader = (vm["header"].as<bool>());
  }
  
  start();

  return EXIT_SUCCESS;
}
