#pragma once

#include <fstream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <memory>
#include <boost/filesystem.hpp>
#include "ocilib.hpp"
#include "spdlog/spdlog.h"
#include "semaphore.hpp"

using namespace std;

class dumper {
public:
  string filename;
  dumper(string f) : filename(f) {};
  ~dumper() {};
};

class dumperVector {
  int counter = 0;
public:
  vector<unique_ptr<dumper>> m_vec;
  dumperVector(){}
  dumperVector(const initializer_list<string>& v){
    for (initializer_list<string>::iterator itr = v.begin(); itr != v.end(); ++itr){
      m_vec.push_back(unique_ptr<dumper>(new dumper(*itr)));
      counter++;
    }
  }
};

static const int maxConnnections = 20;
// global semaphore so that lambdas don't have to capture it
static Semaphore thread_limiter(maxConnnections);

static const char* SQL_FOLDER = "sql";
static const char* OUT_FOLDER = "out";

static string filename;
static string directory;
static string oraService = "test";
static string oraUser = "scott";
static string oraPass = "tiger";
static bool includeHeader = true;

