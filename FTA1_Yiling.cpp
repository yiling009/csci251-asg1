#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <array>
#include <iomanip>
#include <memory> // For smart pointers
#include <map>
#include <cmath> // For log10 and min/max
#include <algorithm> // For remove_if
#include <climits>
using namespace std;

struct CityData 
{
    std::pair<int, int> lowerLeftCoord = {INT_MAX, INT_MAX}; // Initialize to max integer values as this is the lower left corner (lower bound)
    std::pair<int, int> topRightCoord = {INT_MIN, INT_MIN}; // Initialize to min integer values as top right is always greater than lower left
    float avgAtmosphericPressure = 0.f; // Initialize to 0
    float avgCloudCover = 0.f; // Initialize to 0
};


struct GridCellInfo 
{
    bool isCity = false;
    int cityId = -1; // -1 is invalid ID
    float atmosphericPressure = 0.f; // integer input but allow to store as float when doing avg
    float cloudCover = 0.f; 

    static unsigned numberOfDigits; // Number of digits for city ID, used for padding when printing city map
    static unsigned leftPadding; // Number of digits for city ID, used for padding when printing city map
    static unsigned rightPadding; 

    string cityMapPrintCell(); // Print the cell for city map, pad with 2 spaces (left and right), will print blank space if it is not city
    string lmhMapPrintCell(bool cloud = false); // Print the cell for LMH map, pad with 2 spaces (left and right), will print blank space if it is not city
    string indMapPrintCell(bool cloud = false); // Print the cell for Atmospheric Pressure map, pad with 2 spaces (left and right), will print blank space if it is not city
};

string GridCellInfo::cityMapPrintCell() 
{
    ostringstream oss;

    // Check if the cell is a city
    if(cityId >= 0) 
    {
        // Format the city ID with padding and add it to the output stream
        // Format the output with the letter centered and padded with spaces
        oss << setw(leftPadding + 1) << setfill(' ') << ' ' << cityId << setw(rightPadding + 1) << ' ';
    } 
    
    else 
    {
        // If the cell is not a city, output a blank space with the same width
        oss << setw(leftPadding + 1) << setfill(' ') << ' ' << ' ' << setw(rightPadding + 1) << ' ';
    }

    // Return the formatted string
    return (oss.str());
}

string GridCellInfo::indMapPrintCell(bool cloud) 
{
    ostringstream oss;

    float value = 0;

    if (cloud) 
    {
        value = cloudCover;
    } 
    
    else 
    {
        value = atmosphericPressure;
    }
    oss << setw(leftPadding + 1) << setfill(' ') << ' ' << static_cast<int>(std::max(0.f, value-1)/10.f) << setw(rightPadding + 1) << ' ';

    // Return the formatted string
    return (oss.str());
}

char convertToLMHSymbol(float value) {
    if (value < 35) {
        return 'L'; // Low cloud cover
    } else if (value < 65) {
        return 'M'; // Medium cloud cover
    } else {
        return 'H'; // High cloud cover
    }
}


string GridCellInfo::lmhMapPrintCell(bool cloud) {
    ostringstream oss;
    char letter = ' ';

    float value = 0;
    if (cloud) 
    {
        value = cloudCover;
    } 
    
    else 
    {
        value = atmosphericPressure;
    }
    letter = convertToLMHSymbol(value);

    // Format the output with the letter centered and padded with spaces
    oss << setw(leftPadding + 1) << setfill(' ') << ' ' << letter << setw(rightPadding + 1) << ' ';

    // Return the formatted string
    return (oss.str());
}

int countNumberOfDigits(int number) 
{
    if (number == 0) return 1; // Log10 of 0 is undefined, so we handle it separately
    return static_cast<int>(log10(abs(number))) + 1; //log10 is from cmath
}

// Global variables
GridCellInfo** grid; // Global grid which houses all the information, 2d array
int gridXmin = 0, gridXmax = 0, gridYmin = 0, gridYmax = 0; 
unsigned GridCellInfo::numberOfDigits = 0; // Initialize number of digits for city ID to 0 digits
unsigned GridCellInfo::leftPadding = 0; // Initialize left padding for print to 0 space
unsigned GridCellInfo::rightPadding = 0; // Initialize right padding for print to 0 space
std::map<int, CityData> cityDataMap; // Map to store city data, key is city ID, value is CityData struct

void promptToEnterOnly() 
{
	char enter_key = ' ';
	do 
    {
		cout << endl;
		cout << "Press <Enter> to go back to main menu ... ";
		cin.clear(); // resets any error flags which may have been set
		cin.ignore(numeric_limits<streamsize>::max(), '\n'); // This will clear buffer before taking new
		enter_key = cin.get();
	} while (enter_key != '\n');
}

// Function prototypes
int mainMenu();
void allocateMemory(int colSize, int rowSize);
void deallocateMemory(int colSize, int rowSize);
void processCityData(const string& line, int fileDataType); // fileDataType: 0 is citylocation.txt, 1 is cloudcover.txt, 2 is pressure.txt
void printMap(int option);
void city_Location(const string& filename);
void cloud_Coverage(const string& filename);
void pressure_File(const string& filename);
void displaySummary();

int main(int argc, char *argv[]) 
{
    mainMenu(); // Call the mainMenu function
    deallocateMemory((gridXmax - gridXmin) + 1, (gridYmax - gridYmin) + 1); // Deallocate memory
}

void allocateMemory(int colSize, int rowSize) 
{
    grid = new GridCellInfo*[rowSize]; // Allocate memory for rows
    for (int x = 0; x < rowSize; x++) {
        grid[x] = new GridCellInfo[colSize]; // For each row, allocate memory for columns
    }
}

void deallocateMemory(int colSize, int rowSize) 
{
    for (int x = 0; x < rowSize; x++) 
    {
        delete[] grid[x]; // Delete columns
    }
    delete[] grid; // Delete rows
}

void processCityData(const string& line, int fileDataType) 
{ // fileDataType: 0 is citylocation.txt, 1 is cloudcover.txt, 2 is pressure.txt
    int xPos=-1, yPos=-1, cityID=-1, gridValue=-1, citySize=-1; // -1 is invalid value

    size_t dashPosition = line.find('-');
    if (dashPosition != string::npos) 
    {
        string beforeDash = line.substr(0, dashPosition);
        string afterDash = line.substr(dashPosition + 1); // e.g. 50-Big_City


        // Remove [] and space from beforeDash
        beforeDash.erase(remove_if(beforeDash.begin(), beforeDash.end(), [](char c) 
        {
            return c == ' ' || c == '[' || c == ']';
        }), beforeDash.end());

        // Extract coordinates

        size_t commaPosition = beforeDash.find(',');
        if (commaPosition != string::npos) 
        {
            string beforecomma = beforeDash.substr(0, commaPosition); // "10"
            string aftercomma = beforeDash.substr(commaPosition + 1); // "20"

            // Convert to integers
            xPos = stoi(beforecomma);
            yPos = stoi(aftercomma);

            // Validate coordinates by checking for out of bounds
            if (xPos < gridXmin || xPos > gridXmax || yPos < gridYmin || yPos > gridYmax) 
            {
                cerr << "Error: Coordinates (" << xPos << ", " << yPos << ") are out of bounds." << endl;
                return; // Do not process further by forcing empty return
            }

            xPos -= gridXmin; // Adjust x position to start from 0
            yPos -= gridYmin; // Adjust y position to start from 0
        }
        
        // Extract city ID and city size, e.g. "50-Big_City", only will find hyphen if it is a citylocation.txt file
        size_t hyphenPosition = afterDash.find('-');
        if (hyphenPosition != string::npos) 
        {
            string beforeHyphen = afterDash.substr(0, hyphenPosition); // "50"
            string afterHyphen = afterDash.substr(hyphenPosition + 1); // "Big_City"

            // Convert to integers
            cityID = stoi(beforeHyphen); // 50
            // Validate city ID
            if (cityID < 0) 
            {
                cerr << "Error: Invalid city ID." << endl;
            }

            // Determine city size
            int citySize;
            if (afterHyphen == "Big_City") 
            {
                citySize = 3;
            } else if (afterHyphen == "Mid_City") 
            {
                citySize = 2;
            } else if (afterHyphen == "Small_City") 
            {
                citySize = 1;
            } else 
            {
                cerr << "Error: Invalid city size." << endl;
            }

            grid[xPos][yPos].isCity = true; // Set the cell as a city
            grid[xPos][yPos].cityId = cityID; // Set the city ID

            cityDataMap[cityID].lowerLeftCoord = std::make_pair(
                std::min(xPos, cityDataMap[cityID].lowerLeftCoord.first),
                std::min(yPos, cityDataMap[cityID].lowerLeftCoord.second)
            ); // Set the lower left coordinate

            cityDataMap[cityID].topRightCoord = std::make_pair(
                std::max(xPos, cityDataMap[cityID].topRightCoord.first),
                std::max(yPos, cityDataMap[cityID].topRightCoord.second)
            ); // Set the lower left coordinate


        } else 
        {
            // Could either be cloudcover.txt or pressure.txt
            gridValue = stoi(afterDash);
            // Validate value
            if (gridValue < 0 || gridValue > 100) 
            {
                cerr << "Error: Invalid atmospheric pressure value." << endl;
            }
            if (fileDataType == 1) 
            {
                // Cloud cover
                grid[xPos][yPos].cloudCover = static_cast<float>(gridValue); // Explicitly cast to float for code readability
            } else if (fileDataType == 2) 
            {
                // Atmospheric pressure
                grid[xPos][yPos].atmosphericPressure = static_cast<float>(gridValue); // Explicitly cast to float for code readability
            }
        }
    }
}
bool inFile;

void printMap(int option) {
    int x_range = (gridXmax - gridXmin) + 1;
    int y_range = (gridYmax - gridYmin) + 1;

    // Print border (top)
    cout << setw(GridCellInfo::numberOfDigits) << setfill(' ') << " ";
    for (int i = 0; i < x_range + 2; i++) {
        cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << ' ' << '#' << ' ';
    }
    cout << endl;

    // Print grid content
    for (int y = y_range - 1; y >= 0; y--) {
        cout << y << " # "; // Row label (y-axis)
        for (int x = 0; x < x_range; x++) {
            switch (option){
                case 1: //"Display City Map"
                    cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << grid[x][y].cityMapPrintCell();
                    break;
                case 2: //"Display Cloud Coverage Map (Cloudiness Index)"
                    cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << grid[x][y].indMapPrintCell(true);
                    break;
                case 3: //"Display Cloud Coverage Map (LMH Symbol)"
                    cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << grid[x][y].lmhMapPrintCell(true);
                    break;
                case 4: //"Display atmospheric pressure map (Pressure Index)"
                    cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << grid[x][y].indMapPrintCell(false);
                    break;
                case 5: //"Display atmospheric pressure map (LMH symbol)"
                    cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << grid[x][y].lmhMapPrintCell(false);
                    break;
                default:
                    break;
            }
        }
        cout << " #"; // Right border
        cout << endl;
    }

    // Print bottom border
    cout << setw(GridCellInfo::numberOfDigits) << setfill(' ') << " ";
    for (int x = 0; x < x_range +2; x++) {
        cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << ' ' << '#' << ' ';
    }
    cout << endl;

    // Print x-axis labels
    for (int i = 0; i < 4; i++) {
        cout << setw(GridCellInfo::leftPadding + 1) << " ";
    }
    for (int x= 0; x < x_range; x++) {
        cout << setw(GridCellInfo::leftPadding + 1) << setfill(' ') << ' ' << x << ' ';
    }
    cout << endl;
}

string studentID = "UOW9090307";
string studentName = "Yang Yiling";

int mainMenu() {
    int userOption;
    bool fileProcessed = false; // Flag to track if the file has been processed

    while (true) {
        cout << "Student ID: " << studentID << endl;
        cout << "Student Name: " << studentName << endl;
        cout << "--------------------------------------------------" << endl;
        cout << " Welcome to Weather Information Processing System" << endl;
        cout << "1.\tRead and Process configuration file" << endl;
        cout << "2.\tDisplay City Map" << endl;
        cout << "3.\tDisplay Cloudiness Index Coverage Map (Cloudiness Index)" << endl;
        cout << "4.\tDisplay Cloud Coverage Map (LMH Symbol)" << endl;
        cout << "5.\tDisplay Atmospheric Pressure Coverage Map (Pressure Index)" << endl;
        cout << "6.\tDisplay Atmospheric Pressure Coverage Map (LMH Symbol)" << endl;
        cout << "7.\tShow Weather Forecast Summary" << endl;
        cout << "8.\tExit " << endl;

        cout << "Please enter your choice (1-8): ";
        cin >> userOption;

        if (userOption == 1) {
            cout << "Please enter file name: " << endl;
            string fileName;
            cin >> fileName;

            ifstream inFile(fileName); // Read file based on input

            // Check if file cannot be opened
            if (!inFile.is_open()) {
                cout << "Error: Unable to open file! Please try again!\n" << endl;
                continue;
            }

            string line;
            string citylocFilePath;
            string cloudcoverageFilePath;
            string pressureFilePath;


            while (getline(inFile, line)) {
                size_t gridPosition = line.find("Grid");
                if (gridPosition == 0) {
                    size_t equalPosition = line.find('=');
                    if (equalPosition != string::npos) {
                        string beforeEqual = line.substr(0, equalPosition);
                        string afterEqual = line.substr(equalPosition + 1);

                        size_t dashPosition = afterEqual.find('-');
                        string beforeDash = afterEqual.substr(0, dashPosition);
                        string afterDash = afterEqual.substr(dashPosition + 1);

                        // Convert string to int to determine range
                        int min = stoi(beforeDash);
                        int max = stoi(afterDash);

                        // Setting grid range for x and y
                        if (beforeEqual == "GridX_IdxRange") {
                            gridXmin = min;
                            gridXmax = max;
                        } else if (beforeEqual == "GridY_IdxRange") {
                            gridYmin = min;
                            gridYmax = max;
                        }
                    }
                }
            }

            // Display grid ranges first
            cout << "Reading in GridX_IdRange: " << gridXmin << "-" << gridXmax << " ... done!" << endl;
            cout << "Reading in GridY_IdRange: " << gridYmin << "-" << gridYmax << " ... done!" << endl;
            cout << "\nStoring data from input file: " << endl;

            // Reset file pointer to the beginning to read file paths
            inFile.clear(); // Clear any error flags
            inFile.seekg(0, ios::beg); // Move file pointer to the beginning

            // Process file paths
            while (getline(inFile, line)) {
                size_t cityLocPos = line.find("citylocation.txt");
                size_t cloudcoverPos = line.find("cloudcover.txt");
                size_t pressurepos = line.find("pressure.txt");

                if (cityLocPos != string::npos) {
                    cout << "citylocation.txt...done" << endl;
                    citylocFilePath = line;
                } else if (cloudcoverPos != string::npos) {
                    cout << "cloudcover.txt...done" << endl;
                    cloudcoverageFilePath = line;
                } else if (pressurepos != string::npos) {
                    cout << "pressure.txt...done" << endl;
                    pressureFilePath = line;
                }
            }

            // Calculate padding and other setup
            GridCellInfo::numberOfDigits = countNumberOfDigits(gridXmax); // Calculate number of digits for city ID
            int totalPadding = GridCellInfo::numberOfDigits - 1;
            GridCellInfo::leftPadding = totalPadding / 2;
            GridCellInfo::rightPadding = totalPadding - GridCellInfo::leftPadding;

            int rowSize = (gridXmax - gridXmin) + 1;
            int colSize = (gridYmax - gridYmin) + 1;


            cout << "\nAll records successfully stored. Going back to main menu ...\n" << endl;           

            // Allocate memory for cityData, cloudData, and pressureData
            allocateMemory(colSize, rowSize);

            if (citylocFilePath.length() == 0) {
                cout << "City Location File Not Found" << endl;
            } else {
                city_Location(citylocFilePath);
            }

            if (cloudcoverageFilePath.length() == 0) {
                cout << "Cloud Cover File Not Found" << endl;
            } else {
                cloud_Coverage(cloudcoverageFilePath);
            }

            if (pressureFilePath.length() == 0) {
                cout << "Pressure File Not Found" << endl;
            } else {
                pressure_File(pressureFilePath);
            }
            inFile.close();

            // Process the average atmospheric pressure and cloud cover for each city
            for (const auto& cityData : cityDataMap) 
            {
                int cityId = cityData.first;
                CityData data = cityData.second;

                // Calculate the average atmospheric pressure and cloud cover for each city
                float totalAtmosphericPressure = 0.f;
                float totalCloudCover = 0.f;
                int totalCells = 0;

                for (int x = std::max(data.lowerLeftCoord.first - 1, gridXmin) - gridXmin; x <= std::min(data.topRightCoord.first + 1, gridXmax) - gridXmin; x++) 
                {
                    for (int y = std::max(data.lowerLeftCoord.second - 1, gridYmin) - gridYmin; y <= std::min(data.topRightCoord.second + 1, gridYmax) - gridYmin; y++) 
                    {
                        totalAtmosphericPressure += grid[x][y].atmosphericPressure;
                        totalCloudCover += grid[x][y].cloudCover;
                        totalCells++;
                    }
                }

                // Calculate the average atmospheric pressure and cloud cover
                data.avgAtmosphericPressure = totalAtmosphericPressure / static_cast<float>(totalCells);
                data.avgCloudCover = totalCloudCover / static_cast<float>(totalCells);

                // Update the city data
                cityDataMap[cityId] = data;
            }

            cout << "End of Option 1\n";
            fileProcessed = true; // Set the flag to true after processing the file
        } 
        
        else if (userOption >= 2 && userOption <= 7) 
        {
            if (!fileProcessed) 
            {
                cout << "Error: You must read and process the file first (Option 1)!\n" << endl;
            } else 
            {
                switch (userOption) 
                {
                    case 2:
                        cout << "Display City Map" << endl;
                        printMap(1);
                        promptToEnterOnly();
                        break;
                    case 3:
                        cout << "Display Cloud Coverage Map (Cloudiness Index)" << endl;
                        printMap(2);
                        promptToEnterOnly();
                        break;
                    case 4:
                        cout << "Display Cloud Coverage Map (LMH Symbol)" << endl;
                        printMap(3);
                        promptToEnterOnly();
                        break;
                    case 5:
                        cout << "Display atmospheric pressure map (Pressure Index)" << endl;
                        printMap(4);
                        promptToEnterOnly();
                        break;
                    case 6:
                        cout << "Display atmospheric pressure map (LMH symbol)" << endl;
                        printMap(5);
                        promptToEnterOnly();
                        break;
                    case 7:
                        cout << "7" << endl;
                        displaySummary();
                        promptToEnterOnly();
                        break;
                }
            }
        } else if (userOption == 8) {
            cout << "Exiting..." << endl;
            break;
        } else {
            cout << "Invalid choice. Please try again." << endl;
        }
    }

    return 0;
}

std::string getLMHSymbol(double value) 
{
    if (value >= 0 && value < 35) 
    {
        return "L";
    } else if (value >= 35 && value < 65) 
    {
        return "M";
    } else if (value >= 65 && value < 100) 
    {
        return "H";
    }
    return "N/A";
}

void printArray(int** array_data, const string& option) 
{
    int x_range = (gridXmax - gridXmin) + 1;
    int y_range = (gridYmax - gridYmin) + 1;

    // Print border (top)
    cout << setw(3) << setfill(' ') << " ";
    for (int i = 0; i < x_range + 2; i++) 
    {
        cout << setw(3) << setfill(' ') << "#"; // Fill with ##
    }
    cout << endl;

    for (int y = y_range - 1; y >= 0; y--) 
    {
        cout << setw(3) << y;
        cout << setw(3) << " #";
        for (int x = 0; x < x_range; x++) 
        {
            if (array_data[x][y] != 0) 
            {
                if (option == "1") 
                {
                    cout << setw(3) << array_data[x][y];
                } 
                
                else if (option == "2" || option == "4") 
                {
                    cout << setw(3) << int(array_data[x][y] / 10);
                } 
                
                else if (option == "3" || option == "5") 
                {
                    cout << setw(3) << getLMHSymbol(array_data[x][y]);
                }
            } 
            
            else 
            {
                cout << setw(3) << " ";
            }
        }
        cout << setw(3) << '#';
        cout << endl;
    }
    cout << setw(3) << setfill(' ') << " ";

    for (int x = 0; x < x_range +2; x++) 
    {
        cout << setw(3) << setfill(' ') << ("#");
    }
    cout << endl;

    for (int i = 0; i < 2; i++) 
    {
        cout << setw(3) << " ";
    }

    for (int y = 0; y < y_range; y++) 
    {
        cout << setw(3) << y ;
    }
    cout << endl;
}

void city_Location(const string& filename) 
{
    ifstream cityLocation(filename);
    string line;

    if (!cityLocation.is_open()) 
    {
        cout << "Unable to open file" << endl;
        return;
    }

    while (getline(cityLocation, line)) 
    {
        processCityData(line, 0);
    }

    cityLocation.close();
}

//access the data in cloudcover.txt
void cloud_Coverage(const string& filename) 
{
    ifstream cloudCoverage(filename);
    string line;

    if (!cloudCoverage.is_open()) 
    {
        cout << "Unable to open file" << endl;
        return;
    }

    while (getline(cloudCoverage, line)) 
    {
        processCityData(line, 1);
    }

    cloudCoverage.close();
}

//access the data in pressure.txt
void pressure_File(const string& filename) 
{
    ifstream pressureFile(filename);
    string line;

    if (!pressureFile.is_open()) 
    {
        cout << "Unable to open file" << endl;
        return;
    }

    while (getline(pressureFile, line)) 
    {
        processCityData(line, 2);
    }

    pressureFile.close();
}


void display_ASCII(int probability) 
{
    if (probability == 90) 
    {
        cout << "~~~~" << endl;
        cout << "~~~~~" << endl;
        cout << "\\\\\\\\\\" << endl << endl;
    } 
    else if (probability == 80) 
    {
        cout << "~~~~" << endl;
        cout << "~~~~~" << endl;
        cout << " \\\\\\\\" << endl << endl;
    } 
    else if (probability == 70) 
    {
        cout << "~~~~" << endl;
        cout << "~~~~~" << endl;
        cout << "  \\\\\\" << endl << endl;
    } else if 
    (probability == 60) 
    {
        cout << "~~~~" << endl;
        cout << "~~~~~" << endl;
        cout << "   \\\\" << endl << endl;
    } 
    else if (probability == 50) 
    {
        cout << "~~~~" << endl;
        cout << "~~~~~" << endl;
        cout << "    \\" << endl << endl;
    } 
    else if (probability == 40) 
    {
        cout << "~~~~" << endl;
        cout << "~~~~~" << endl << endl;
    } 
    else if (probability == 30) 
    {
        cout << "~~~" << endl;
        cout << "~~~~" << endl << endl;
    } 
    else if (probability == 20) 
    {
        cout << "~~" << endl;
        cout << "~~~" << endl << endl;
    } 
    else if (probability == 10) 
    {
        cout << "~" << endl;
        cout << "~~" << endl << endl;
    }
}

int rainchance(char acc, char ap) 
{
    int probability;

    if (ap == 'L' && acc == 'H') {
        probability = 90;
    } else if (ap == 'L' && acc == 'M') {
        probability = 80;
    } else if (ap == 'L' && acc == 'L') {
        probability = 70;
    } else if (ap == 'M' && acc == 'H') {
        probability = 60;
    } else if (ap == 'M' && acc == 'M') {
        probability = 50;
    } else if (ap == 'M' && acc == 'L') {
        probability = 40;
    } else if (ap == 'H' && acc == 'H') {
        probability = 30;
    } else if (ap == 'H' && acc == 'M') {
        probability = 20;
    } else if (ap == 'H' && acc == 'L') {
        probability = 10;
    } else {
        probability = 0;
    }
    return probability;
}


void displaySummary() {
    for(const auto& cityData : cityDataMap) 
    {
        int cityID = cityData.first;
        CityData data = cityData.second;
        string cityName;
        switch(cityID)
        {
            case 1:
                cityName = "Small_City";
                break;
            case 2:
                cityName = "Mid_City";
                break;
            case 3:
                cityName = "Big_City";
                break;
            default:
                break;
        }

        char ACC_symbol = convertToLMHSymbol(data.avgCloudCover);
        char AP_symbol = convertToLMHSymbol(data.avgAtmosphericPressure);
        int rainProbability = rainchance(ACC_symbol, AP_symbol); // Calculate rain probability

        std::cout << "City Name : " << cityName << "\n";
        std::cout << "City ID : " << cityID << "\n";
        std::cout << "Average Cloud Cover (ACC) : " << data.avgCloudCover << " (" << ACC_symbol << ")\n";
        std::cout << "Average Pressure (AP) : " << data.avgAtmosphericPressure << " (" << AP_symbol << ")\n";
        std::cout << "Probability of Rain (%) : " << rainProbability << "\n";
        display_ASCII(rainProbability);
    }
}
