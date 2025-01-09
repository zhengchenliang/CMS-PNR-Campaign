# CMS-PNR-Campaign

This repository contains utilities for managing campaigns in CMS PNR.

## `wfStatusSift.cc`

This script parses campaign data from an online source, filters entries based on specific criteria, and outputs the results in both ROOT and FPS formats.

### Features

1. **Fetch Campaign Data**: Retrieves campaign data from a specified URL using `cURL`.
2. **Parse HTML**: Extracts relevant campaign information using regex-based parsing.
3. **Write ROOT Files**: Stores parsed data in ROOT format for further analysis.
4. **Filter Campaigns**: Filters campaigns based on custom criteria and outputs the results to both a ROOT file and an FPS (flat text) file.
5. **Flexible Sifting**: Allows filtering campaigns that do not meet specific criteria, such as campaigns not starting with "CMSSW_".

### Usage

#### Prerequisites
Ensure the following libraries and tools are installed:
- **ROOT**: For handling `.root` files.
- **cURL**: For fetching HTML data.
- **C++17** or later.

#### Compilation
To compile the script:
```bash
g++ wfStatusSift.cc -o wfStatusSift -std=c++17 `root-config --cflags --libs` -lcurl
```
or
```bash
root -l -b -q wfStatusSift.cc
```

#### Execution
Run the compiled binary with an optional output tag:
```bash
./wfStatusSift [outputTag]
```
- **Default Output Tag**: `wfStatus`.
- **Outputs**:
  - `<outputTag>.root`: Contains all parsed campaign data.
  - `<outputTag>.fps`: Flat file with parsed campaign data.
  - `<outputTag>.sift.root`: Contains filtered campaign data.
  - `<outputTag>.sift.fps`: Flat file with filtered campaign data.

### Filtering Logic
The script filters campaigns based on the following criteria:
- **Campaign Name**: Excludes campaigns that do not start with `CMSSW_`.
- **Column Values**: Ensures specific columns (e.g., `newCount`, `assignmentApproved`) are zero for inclusion.

### File Outputs
1. **ROOT Files**:
   - Parsed data stored in a ROOT tree structure.
   - Filtered data stored in a separate ROOT file.

2. **FPS File**:
   - Flat text file with filtered data in a pipe-delimited format.

### Example Output
**Filtered FPS File Example**:
```
Campaign|LastReqDays|New|AssignmentApproved|Assigned|Acquired|Running-open|Running-closed|Completed|Closed-out|Announced|Normal-archived|Aborted|Aborted-Completed|Aborted-Achived|Failed|Rejected|Rejected-archived|Other||wfStatus
CMSSW_X_Y_Z|0.1|0|0|0|0|0|0|0|0|1|1|0|0|0|0|0|0|0
Total # Sifted = 42
```

### Contributions
Contributions are welcome! Please open an issue or pull request for any suggestions or bug fixes.

### License
This project is distributed under the MIT License. See `LICENSE` for details.
