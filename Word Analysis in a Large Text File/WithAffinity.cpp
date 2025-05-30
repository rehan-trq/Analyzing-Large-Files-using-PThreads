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
#include <sched.h> // For CPU affinity

using namespace std;

// Global data structures
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
unordered_map<string, int> wordCounts; // Global word count map
int wordcount = 0; // Total word count
int vowelcount = 0; // Count of words starting with vowels

// Structure to pass multiple arguments to the thread function
struct ThreadData {
    string* chunk;
    int coreId; // CPU core ID for affinity
};

// Function to process a chunk
void* processChunk(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    string* chunk = data->chunk;
    int coreId = data->coreId;

    // Set CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    pthread_t currentThread = pthread_self();
    if (pthread_setaffinity_np(currentThread, sizeof(cpu_set_t), &cpuset) != 0) {
        cerr << "Failed to set thread affinity for core " << coreId << endl;
    }

    unordered_map<string, int> localWordCounts;
    int localwordcount = 0;
    int localvowelcount = 0;

    string word;
    for (char ch : *chunk) {
        if (isalpha(ch)) {
            word += tolower(ch); // Convert to lowercase
        } else if (!word.empty()) {
            // Update local counts
            localwordcount++;
            if (word[0] == 'a' || word[0] == 'e' || word[0] == 'i' || word[0] == 'o' || word[0] == 'u') {
                localvowelcount++;
            }
            localWordCounts[word]++;
            word.clear();
        }
    }

    // Handle the last word if the chunk ends with a word
    if (!word.empty()) {
        localwordcount++;
        if (word[0] == 'a' || word[0] == 'e' || word[0] == 'i' || word[0] == 'o' || word[0] == 'u') {
            localvowelcount++;
        }
        localWordCounts[word]++;
    }

    // Update global counts with thread safety
    pthread_mutex_lock(&mutex);
    wordcount += localwordcount;
    vowelcount += localvowelcount;
    for (const auto& pair : localWordCounts) {
        wordCounts[pair.first] += pair.second;
    }
    pthread_mutex_unlock(&mutex);

    delete chunk; // Free the dynamically allocated chunk
    delete data;  // Free the ThreadData structure
    return nullptr;
}

// Function to split the file into chunks
vector<string> splitFileIntoChunks(const string& filePath, int numChunks) {
    ifstream file(filePath, ios::binary | ios::ate);
    if (!file) {
        cerr << "Error opening file!" << endl;
        exit(1);
    }

    streamsize fileSize = file.tellg();
    file.seekg(0, ios::beg);

    vector<string> chunks(numChunks);
    streamsize chunkSize = fileSize / numChunks;

    for (int i = 0; i < numChunks; ++i) {
        streamsize start = i * chunkSize;
        streamsize end = (i == numChunks - 1) ? fileSize : (i + 1) * chunkSize;

        // Adjust start and end to avoid splitting words
        if (i != 0) {
            file.seekg(start);
            while (file.get() != ' ' && file.tellg() < end) {}
            start = file.tellg();
        }

        streamsize size = end - start;
        chunks[i].resize(size);
        file.read(&chunks[i][0], size);
    }

    return chunks;
}

int main(int argc, char* argv[]) 
{
    // Check if the correct number of arguments is provided
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <num_threads>" << endl;
        return 1;
    }

    // Hardcoded file path
    string filePath = "file.txt";

    // Number of threads from command-line argument
    int threadcount = stoi(argv[1]);

    // Split the file into chunks
    vector<string> chunks = splitFileIntoChunks(filePath, threadcount);

    // Create and launch threads
    vector<pthread_t> threads(threadcount);
    for (int i = 0; i < threadcount; ++i) {
        ThreadData* data = new ThreadData{new string(chunks[i]), i}; // Assign each thread to a different core
        pthread_create(&threads[i], nullptr, processChunk, data);
    }

    // Wait for all threads to finish
    for (int i = 0; i < threadcount; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Output results
    cout << "Total word count: " << wordcount << endl;
    cout << "Count of words starting with vowels: " << vowelcount << endl;

    // Find top 10 most frequent words
    vector<pair<string, int>> sortedWordCounts(wordCounts.begin(), wordCounts.end());
    sort(sortedWordCounts.begin(), sortedWordCounts.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    cout << "Top 10 most frequent words:" << endl;
    for (int i = 0; i < 10 && i < sortedWordCounts.size(); ++i) {
        cout << sortedWordCounts[i].first << ": " << sortedWordCounts[i].second << endl;
    }

    // Clean up
    pthread_mutex_destroy(&mutex);

    return 0;
}
