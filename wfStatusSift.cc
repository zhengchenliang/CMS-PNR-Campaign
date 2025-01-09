#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <curl/curl.h>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>

#define _numCols_ 18
struct RowData
{
  std::string campaign;
  double col[_numCols_];
};

size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  reinterpret_cast<std::string*>(userp)->append(
    reinterpret_cast<char*>(contents),
    size * nmemb
  );
  return size * nmemb;
}

bool FetchHTML(const std::string &url, std::string &html)
{
  CURL* curl = curl_easy_init();
  if (!curl)
  {
    std::cerr << "Error: cannot init CURL.\n";
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  if (auto res = curl_easy_perform(curl); res != CURLE_OK)
    std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
  curl_easy_cleanup(curl);
  return true;
}

bool ParseSingleRow(const std::string &rowRaw, RowData &outRow)
{
  // 1) Replace objects {v:X,f:'<b>...'</b>} with X
  static const std::regex objectRegex(R"(\{v:\s*([+-]?[0-9]*\.?[0-9]+)\s*,f:'[^']*'\})");
  std::string rowFormal = std::regex_replace(rowRaw, objectRegex, "$1");
  std::cout << rowRaw << " -> " << rowFormal << std::endl;
  // 2) Extract campaign=...
  static const std::regex campRegex(R"(campaign=([^"]+))");
  std::smatch cm;
  if (std::regex_search(rowFormal, cm, campRegex))
    outRow.campaign = cm[1].str();
  else
    outRow.campaign = rowFormal; // fallback if not found
  std::cout << outRow.campaign << std::endl;
  // 3) Parse numeric tokens
  static const std::regex numberRegex(R"(,[0-9]+(\.[0-9]+)?)");
  std::sregex_iterator it(rowFormal.begin(), rowFormal.end(), numberRegex), end;
  std::vector<double> vals;
  for (; it != end; ++it)
  {
    try
    {
      std::string its = (*it).str().substr(1);
      vals.push_back(std::stod(its));
    }
    catch (...) { /* skip or warn */ }
  }
  if (vals.size() != _numCols_)
  {
    std::cerr << "Warning: need " << _numCols_ << " numeric columns, found " << vals.size() << "\n";
    return false;
  }
  // 4) Copy into outRow
  for (size_t i = 0; i < _numCols_; i++)
  {
    outRow.col[i] = vals[i];
  }

  return true;
}

bool ParseAllRows(const std::string &html, std::vector<RowData> &rows)
{
  static const std::regex bigRowRegex(R"(data\.addRow\s*\(\s*\[(.*?)\]\s*\)\s*;)");
  bool allOk = true;
  for (std::sregex_iterator b(html.begin(), html.end(), bigRowRegex), e; b != e; ++b)
  {
    RowData row;
    if (!ParseSingleRow((*b)[1].str(), row))
      allOk = false;
    else
      rows.push_back(row);
  }
  return allOk;
}

bool WriteTTree(const std::vector<RowData> &rows, const std::string &fileName)
{
  TFile out((fileName + ".root").c_str(), "RECREATE");
  if (out.IsZombie())
  {
    std::cerr << "Error: cannot create file " << fileName << ".root\n";
    return false;
  }
  TTree* tree = new TTree("statusTree", "statusTree");

  std::string b_campaign;
  double b_lastRequestDays;
  long b_newCount, b_assignmentApproved, b_assigned, b_acquired;
  long b_runningOpen, b_runningClosed, b_completed, b_closedOut, b_announced;
  long b_normalArchived, b_aborted, b_abortedCompleted, b_abortedArchived, b_failed;
  long b_rejected, b_rejectedArchived, b_other;
/*
  double b_num[_numCols_];
*/
  tree->Branch("campaign", &b_campaign);
  tree->Branch("lastRequestDays",      &b_lastRequestDays,     "lastRequestDays/D");
  tree->Branch("newCount",             &b_newCount,            "newCount/L");
  tree->Branch("assignmentApproved",   &b_assignmentApproved,  "assignmentApproved/L");
  tree->Branch("assigned",             &b_assigned,            "assigned/L");
  tree->Branch("acquired",             &b_acquired,            "acquired/L");
  tree->Branch("runningOpen",          &b_runningOpen,         "runningOpen/L");
  tree->Branch("runningClosed",        &b_runningClosed,       "runningClosed/L");
  tree->Branch("completed",            &b_completed,           "completed/L");
  tree->Branch("closedOut",            &b_closedOut,           "closedOut/L");
  tree->Branch("announced",            &b_announced,           "announced/L");
  tree->Branch("normalArchived",       &b_normalArchived,      "normalArchived/L");
  tree->Branch("aborted",              &b_aborted,             "aborted/L");
  tree->Branch("abortedCompleted",     &b_abortedCompleted,    "abortedCompleted/L");
  tree->Branch("abortedArchived",      &b_abortedArchived,     "abortedArchived/L");
  tree->Branch("failed",               &b_failed,              "failed/L");
  tree->Branch("rejected",             &b_rejected,            "rejected/L");
  tree->Branch("rejectedArchived",     &b_rejectedArchived,    "rejectedArchived/L");
  tree->Branch("other",                &b_other,               "other/L");
/*
  for (int i = 0; i < _numCols_; i++)
    tree->Branch(("col" + std::to_string(i)).c_str(), &b_num[i], ("col" + std::to_string(i)+"/D").c_str());
*/
  std::ofstream fpsFile(fileName + ".fps");
  fpsFile << "Campaign|LastReqDays|New|AssignmentApproved|Assigned|Acquired|Running-open|Running-closed|Completed|Closed-out|Announced|Normal-archived|Aborted|Aborted-Completed|Aborted-Achived|Failed|Rejected|Rejected-archived|Other||" << fileName << "\n";
  // Fill
  for (auto &r : rows)
  {
    b_campaign           = r.campaign;
    b_lastRequestDays    = r.col[0];
    b_newCount           = static_cast<long>(r.col[1]);
    b_assignmentApproved = static_cast<long>(r.col[2]);
    b_assigned           = static_cast<long>(r.col[3]);
    b_acquired           = static_cast<long>(r.col[4]);
    b_runningOpen        = static_cast<long>(r.col[5]);
    b_runningClosed      = static_cast<long>(r.col[6]);
    b_completed          = static_cast<long>(r.col[7]);
    b_closedOut          = static_cast<long>(r.col[8]);
    b_announced          = static_cast<long>(r.col[9]);
    b_normalArchived     = static_cast<long>(r.col[10]);
    b_aborted            = static_cast<long>(r.col[11]);
    b_abortedCompleted   = static_cast<long>(r.col[12]);
    b_abortedArchived    = static_cast<long>(r.col[13]);
    b_failed             = static_cast<long>(r.col[14]);
    b_rejected           = static_cast<long>(r.col[15]);
    b_rejectedArchived   = static_cast<long>(r.col[16]);
    b_other              = static_cast<long>(r.col[17]);
    // Look
    std::cout << r.campaign << ": ";
    for (int i = 0; i < _numCols_; i++)
    {
      std::cout << r.col[i] << " ";
    }
    std::cout << std::endl;
/*
    for (int i = 0; i < _numCols_; i++)
      b_num[i] = r.col[i];
*/
    tree->Fill();

    fpsFile << b_campaign << "|"
        << b_lastRequestDays << "|"
        << b_newCount << "|"
        << b_assignmentApproved << "|"
        << b_assigned << "|"
        << b_acquired << "|"
        << b_runningOpen << "|"
        << b_runningClosed << "|"
        << b_completed << "|"
        << b_closedOut << "|"
        << b_announced << "|"
        << b_normalArchived << "|"
        << b_aborted << "|"
        << b_abortedCompleted << "|"
        << b_abortedArchived << "|"
        << b_failed << "|"
        << b_rejected << "|"
        << b_rejectedArchived << "|"
        << b_other << "\n";
  }
  fpsFile.close();
  std::cout << "Filtered " << rows.size() << " entries into " << fileName + ".fps\n";
  tree->Write();
  out.Close();
  std::cout << "Wrote " << rows.size() << " entries to " << fileName + ".root\n";
  return true;
}

void FilterStatusTree(const std::string& inputFileName, const TString outTag1)
{
  // 1) Open the input file
  TFile inFile(inputFileName.c_str(), "READ");
  if (inFile.IsZombie())
  {
    std::cerr << "Error: cannot open " << inputFileName << " for reading.\n";
    return;
  }
  // 2) Get the input tree
  TTree* inTree = dynamic_cast<TTree*>(inFile.Get("statusTree"));
  if (!inTree)
  {
    std::cerr << "Error: TTree 'statusTree' not found in " << inputFileName << "\n";
    inFile.Close();
    return;
  }
  // 3) Prepare output file and tree
  TFile outFile(outTag1 + ".sift.root", "RECREATE");
  if (outFile.IsZombie())
  {
    std::cerr << "Error: cannot create status_filtered.root.\n";
    inFile.Close();
    return;
  }
  TTree* outTree = inTree->CloneTree(0); // Clone structure only
  // 4) Set branch addresses for input tree
  std::string* b_campaign = nullptr;
  double b_lastRequestDays;
  Long64_t b_newCount, b_assignmentApproved, b_assigned, b_acquired;
  Long64_t b_runningOpen, b_runningClosed, b_completed, b_closedOut, b_announced;
  Long64_t b_normalArchived, b_aborted, b_abortedCompleted, b_abortedArchived, b_failed;
  Long64_t b_rejected, b_rejectedArchived, b_other;
  inTree->SetBranchAddress("campaign",            &b_campaign);
  inTree->SetBranchAddress("lastRequestDays",     &b_lastRequestDays);
  inTree->SetBranchAddress("newCount",            &b_newCount);
  inTree->SetBranchAddress("assignmentApproved",  &b_assignmentApproved);
  inTree->SetBranchAddress("assigned",            &b_assigned);
  inTree->SetBranchAddress("acquired",            &b_acquired);
  inTree->SetBranchAddress("runningOpen",         &b_runningOpen);
  inTree->SetBranchAddress("runningClosed",       &b_runningClosed);
  inTree->SetBranchAddress("completed",           &b_completed);
  inTree->SetBranchAddress("closedOut",           &b_closedOut);
  inTree->SetBranchAddress("announced",           &b_announced);
  inTree->SetBranchAddress("normalArchived",      &b_normalArchived);
  inTree->SetBranchAddress("aborted",             &b_aborted);
  inTree->SetBranchAddress("abortedCompleted",    &b_abortedCompleted);
  inTree->SetBranchAddress("abortedArchived",     &b_abortedArchived);
  inTree->SetBranchAddress("failed",              &b_failed);
  inTree->SetBranchAddress("rejected",            &b_rejected);
  inTree->SetBranchAddress("rejectedArchived",    &b_rejectedArchived);
  inTree->SetBranchAddress("other",               &b_other);
  // 5) Setup FPS output
  std::ofstream fpsFile(outTag1 + ".sift.fps");
  fpsFile << "Campaign|LastReqDays|New|AssignmentApproved|Assigned|Acquired|Running-open|Running-closed|Completed|Closed-out|Announced|Normal-archived|Aborted|Aborted-Completed|Aborted-Achived|Failed|Rejected|Rejected-archived|Other||" << outTag1 << "\n";
  // 6) Loop over entries and apply filtering
  Long64_t nEntries = inTree->GetEntries();
  int passCount = 0;
  for (Long64_t i = 0; i < nEntries; ++i)
  {
    inTree->GetEntry(i);
    // Filtering criteria
    bool criteria = !(b_campaign->rfind("CMSSW_", 0) == 0) // not starts with "CMSSW_"
      && (b_newCount == 0)
      && (b_assignmentApproved == 0)
      && (b_assigned == 0)
      && (b_acquired == 0)
      && (b_runningOpen == 0)
      && (b_runningClosed == 0)
      && (b_completed == 0)
      && (b_closedOut == 0)
      && (b_aborted == 0)
      && (b_abortedCompleted == 0)
      && (b_abortedArchived == 0)
      && (b_failed == 0)
      && (b_rejected == 0)
      && (b_rejectedArchived == 0)
      && (b_other == 0)
    ;
    if (criteria)
    {
      outTree->Fill();
      passCount++;

      fpsFile << *b_campaign << "|"
        << b_lastRequestDays << "|"
        << b_newCount << "|"
        << b_assignmentApproved << "|"
        << b_assigned << "|"
        << b_acquired << "|"
        << b_runningOpen << "|"
        << b_runningClosed << "|"
        << b_completed << "|"
        << b_closedOut << "|"
        << b_announced << "|"
        << b_normalArchived << "|"
        << b_aborted << "|"
        << b_abortedCompleted << "|"
        << b_abortedArchived << "|"
        << b_failed << "|"
        << b_rejected << "|"
        << b_rejectedArchived << "|"
        << b_other << "\n";
    }
  }
  // 7) Write the output tree and close files
  fpsFile << "Total # Sifted = " << passCount << std::endl;
  fpsFile.close();
  std::cout << "Filtered " << passCount << " entries into " << outTag1 + ".sift.fps\n";

  outTree->Write();
  outFile.Close();
  inFile.Close();
  std::cout << "Filtered " << passCount << " entries into " << outTag1 + ".sift.root\n";
}

int wfStatusSift(std::string outTag = "wfStatus")
{
  TString outTag1(outTag);
  std::string url = "https://dmytro.web.cern.ch/dmytro/cmsprodmon/status.php";
  std::string html;
  std::vector<RowData> rows;

  if (!FetchHTML(url, html))
  {
    std::cerr << "Failed to fetch HTML from " << url << "\n";
    return 1;
  }
  ParseAllRows(html, rows);

  if (!WriteTTree(rows, outTag)) return 1;

  std::cout << "Successfully parsed " << rows.size() << " rows.\n";

  FilterStatusTree(outTag + ".root", outTag1);

  return 0;
}
