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
unordered_map< string, int> wordCounts; // Global word count map
int wordcount = 0; // Total word count
int vowelcount = 0; // Count of words starting with vowels

// Function to process a chunk
void* processChunk(void* arg) 
{
    string* chunk = static_cast< string*>(arg);
    unordered_map< string, int> localWordCounts;
    int localwordcount = 0;
    int localvowelcount = 0;

     string word;
    for (char ch : *chunk) 
    {
        if (isalpha(ch)) 
        {
            word += tolower(ch); 
        } 
        
        else if (!word.empty()) 
        {            
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
    for (const auto& pair : localWordCounts)
    {
        wordCounts[pair.first] += pair.second;
    }
    pthread_mutex_unlock(&mutex);

    delete chunk; // Free the dynamically allocated chunk
    return nullptr;
}

// Function to split the file into chunks
vector< string> filetochunks(const  string& filePath, int chunkcount) 
{
   ifstream file(filePath,  ios::binary |  ios::ate);
   if (!file)
   {
        cerr << "Error opening file!" <<  endl;
        exit(1);
   }

    streamsize fileSize = file.tellg();
    file.seekg(0,  ios::beg);

    vector< string> chunks(chunkcount);
    streamsize chunkSize = fileSize / chunkcount;

    for (int i = 0; i < chunkcount; ++i) 
    {
         streamsize start = i * chunkSize;
         streamsize end = (i == chunkcount - 1) ? fileSize : (i + 1) * chunkSize;

        // Adjust start and end to avoid splitting words
        if (i != 0) 
        {
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

int main(int argc , char * argv[]) 
{
    if (argc != 2) 
    {
        cerr << "Usage: " << argv[0] << " <num_threads>" << endl;
        return 1;
    }

    string filePath =  "file.txt";
    int threadcount = stoi(argv[1]);

     //cout << "Enter the number of threads: ";
     //cin >> threadcount;

    // Split the file into chunks
     vector< string> chunks = filetochunks(filePath, threadcount);

    // Create and launch threads
    vector<pthread_t> threads(threadcount);
    for (int i = 0; i < threadcount; ++i)
    {
        string* chunk = new string(chunks[i]); // Dynamically allocate chunk
        pthread_create(&threads[i], nullptr, processChunk, chunk);
    }

    for (int i = 0; i < threadcount; ++i) 
    {
        pthread_join(threads[i], nullptr);
    }
    
    cout << "Total word count: " << wordcount <<  endl;
    cout << "Count of words starting with vowels: " << vowelcount <<  endl;

    // Find top 10 most frequent words
    vector< pair< string, int>> sortedwords(wordCounts.begin(), wordCounts.end());
    sort(sortedwords.begin(), sortedwords.end(), [](const auto& a, const auto& b) 
    {
        return a.second > b.second;
    });

    cout << "Top 10 most frequent words:" <<  endl;
    for (int i = 0; i < 10 && i < sortedwords.size(); ++i)
    {
         cout << sortedwords[i].first << ": " << sortedwords[i].second <<  endl;
    }

    // Clean up
    pthread_mutex_destroy(&mutex);

    return 0;
}
