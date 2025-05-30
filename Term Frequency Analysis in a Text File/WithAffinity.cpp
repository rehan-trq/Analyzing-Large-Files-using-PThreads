// Rehan Tariq
//22i-0965
//CS-6A

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <pthread.h>
#include <cctype>

using namespace std;

// Global data structures
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
 unordered_map< string, int> wordCounts; // Global word count map (hash table)
int wordcount = 0; // Total word count
int uniquewordCount = 0; // Total number of unique words
struct ThreadData{
    string * chunk;
    int coreId;
};

// Function to process a chunk
void* processChunk(void* arg) {
    ThreadData* Data = static_cast< ThreadData*>(arg);
    string * chunk = Data->chunk;
    int coreId = Data->coreId;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    pthread_t currentThread = pthread_self();
    if (pthread_setaffinity_np(currentThread, sizeof(cpu_set_t), &cpuset) != 0) {
        cerr << "Failed to set thread affinity for core " << coreId << endl;
    }

     unordered_map< string, int> localWordCounts;
    int localwordcount = 0;

     string word;;
    for (char ch : *chunk) {
        if (isalpha(ch)) {
            word += tolower(ch); // Convert to lowercase
        } else if (!word.empty()) {
            // Update local counts
            localwordcount++;
            localWordCounts[word]++;
            word.clear();
        }
    }

    // Handle the last word if the chunk ends with a word
    if (!word.empty()) {
        localwordcount++;
        localWordCounts[word]++;
    }

    // Update global counts with thread safety
    pthread_mutex_lock(&mutex);
    wordcount += localwordcount;
    for (const auto& pair : localWordCounts) {
        wordCounts[pair.first] += pair.second;
    }
    pthread_mutex_unlock(&mutex);

    delete chunk; // Free the dynamically allocated chunk
    delete Data;
    return nullptr;
}

// Function to split the file into chunks
 vector< string> splitFileIntoChunks(const  string& filePath, int numChunks) 
 {
     ifstream file(filePath,  ios::binary |  ios::ate);
    if (!file) 
    {
         cerr << "Error opening file!" <<  endl;
        exit(1);
    }

    streamsize fileSize = file.tellg();
    file.seekg(0,  ios::beg);

     vector< string> chunks(numChunks);
     streamsize chunkSize = fileSize / numChunks;

    for (int i = 0; i < numChunks; ++i) 
    {
         streamsize start = i * chunkSize;
         streamsize end = (i == numChunks - 1) ? fileSize : (i + 1) * chunkSize;

        // Adjust start and end to avoid splitting words
        if (i != 0) 
        {
            file.seekg(start);
            while (file.get() != ' ' && file.tellg() < end) {}
            start = file.tellg();// Move to next non-alphanumeric character
        }


        streamsize size = end - start;
        chunks[i].resize(size);
        file.read(&chunks[i][0], size);
    }

    return chunks;
}

// Function to write results to a text file
void writeResultsToFile(const  string& outputFilePath) 
{
     ofstream outputFile(outputFilePath);
    if (!outputFile) 
    {
         cerr << "Error creating output file!" <<  endl;
        exit(1);
    }

    // Sort word counts in descending order of frequency
     vector< pair< string, int>> sortedWordCounts(wordCounts.begin(), wordCounts.end());
     sort(sortedWordCounts.begin(), sortedWordCounts.end(), [](const auto& a, const auto& b) 
     {
        return a.second > b.second;
    });

    // Write results to the file
    outputFile << "Total word count: " << wordcount << "\n";
    outputFile << "Total number of unique words: " << uniquewordCount << "\n\n";
    outputFile << "Term frequency for each word (sorted by frequency):\n";
    for (const auto& pair : sortedWordCounts) {
        outputFile << pair.first << ": " << pair.second << "\n";
    }

     cout << "Results written to " << outputFilePath <<  endl;
}

int main(int argc , char * argv[]) 
{
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <num_threads>" << endl;
        return 1;
    }

    string filePath =  "combined_output.txt";
    int threadcount = stoi(argv[1]);

    // Take input for file path and number of threads
    filePath = "file.txt";
   // cout << "Enter the number of threads: ";
   // cin >> threadcount;

    // Split the file into chunks
    vector< string> chunks = splitFileIntoChunks(filePath, threadcount);

    // Create and launch threads
     vector<pthread_t> threads(threadcount);
    for (int i = 0; i < threadcount; ++i) 
    {
        ThreadData* Data = new ThreadData{new string(chunks[i]), i}; // Assign each thread to a different core
        pthread_create(&threads[i], nullptr, processChunk, Data);
    }

    // Wait for all threads to finish
    for (int i = 0; i < threadcount; ++i) 
    {
        pthread_join(threads[i], nullptr);
    }

    // Calculate the total number of unique words
    uniquewordCount = wordCounts.size();

    // Output results to console
     cout << "Total word count: " << wordcount <<  endl;
     cout << "Total number of unique words: " << uniquewordCount <<  endl;

    // Write results to a text file
    writeResultsToFile("output_results(affinity).txt");

    // Clean up
    pthread_mutex_destroy(&mutex);

    return 0;
}
