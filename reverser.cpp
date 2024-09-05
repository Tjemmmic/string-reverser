////////////////////////////////////////////////
//  Written by Michael Donovan Tjemmes        //
//  EMAIL: mdtjemmes@proton.me                //
//  LINKEDIN: linkedin.com/in/michaeltjemmes  //
//  GITHUB: github.com/Tjemmmic               //
////////////////////////////////////////////////
#include <string>
#include <iostream>
#include <thread>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <chrono>
using namespace std;
using ll = long long;

#define DEFAULT_THREAD_COUNT 4              // Used if no custom thread count is given when running executable
#define DEFAULT_INPUT_FILE "input.txt"      // Output file was specified to always be input.txt
#define DEFAULT_OUTPUT_FILE "output.txt"    // Output file was specified to always be output.txt

mutex writeLock;                            // Mutex for file write access and access to updating ID below
int writeID = 0;                            // ID of thread that has write access - maintains order of strings in output file

void stringReaderThread(int threadID, int threadCount, string fileName, ll startPosition, ll endPosition) {
	ifstream inputFile;												// Open input file - ensure it opens successfully
	inputFile.open(fileName);
	if (!inputFile.is_open()) {
		printf("Thread %d failed to open file %s", threadID, fileName.c_str());
		return;
	}

	inputFile.seekg(startPosition);											// Navigate to start of this thread's 'slice' and calculate the size of said 'slice'
	ll sliceSize = endPosition - startPosition;

	vector<string> reversedStrings;											// Vector to store the reversed strings as they are processed - keeps order

	while ((inputFile.tellg() < endPosition) && !inputFile.eof()) {							// We want to read only up to the endpoint of this thread's 'slice' or file end
		string word;

		if (inputFile.peek() == '\n') {										// Check if next character is a newline.
			reversedStrings.push_back("\n");								// We want to maintain the newlines and their
		}													// locations from input->output.
		else if (inputFile.peek() == ' ') {
			while (inputFile.peek() == ' ' && inputFile.peek() != -1) {					// Finds newlines even if following spaces.
				inputFile.seekg(inputFile.tellg() + (ll)1);
				if (inputFile.peek() == '\n') {
					reversedStrings.push_back("\n");
				}
			}
		}
		inputFile >> word;											// Once we are past whitespace and have found any newlines,
		word = string(word.rbegin(), word.rend());								// we simply read the next 'word' and reverse it.
		reversedStrings.push_back(word);									// Then we push it to the vector for later writing.
	}
	inputFile.close();

	while (writeID != threadID) {											// We wait for this thread to have access to the output file.
		this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	writeLock.lock();												// We take the mutex lock to enter critical section.
	ofstream outputFile;
	outputFile.open(DEFAULT_OUTPUT_FILE, ios_base::app);								// Open file for appending to add on to previous threads' progress.
	for (auto s : reversedStrings) {
		outputFile << ((s != "\n") ? s + " " : s);								// Write reversed string - with a space if its not a newline.
	}
	outputFile.close();
	writeID++;													// Increment writeID to notify next thread of its turn.
	writeLock.unlock();												// Unlock the mutex as we exit the critical section and end thread.
	printf("Ending Thread %d\n", threadID);
}

int main(int argc, char* argv[]) {
	using chrono::high_resolution_clock;
	using chrono::duration_cast;
	using chrono::milliseconds;

	auto startTime = high_resolution_clock::now();									// Make note of time to measure how long the program took to complete.

	int threadCount = DEFAULT_THREAD_COUNT;										// The number of threads defaults to DEFAULT_THREAD_COUNT unless a number is 
	if (argc == 1) {												// given when running the program from a console.
		printf("No Thread Count Specified, Will Use %d Threads\n", threadCount);
	}
	else if (atoi(argv[1]) > 0) {
		threadCount = atoi(argv[1]);
		printf("Program will use %d threads.\n", threadCount);
	}
	else {
		printf("Invalid Input");
		return -1;
	}

	ifstream inputFile;												// We open the input file to find out the size of the
	inputFile.open(DEFAULT_INPUT_FILE);										// 'chunk' or 'slice' each thread will be in charge of.
	if (!inputFile.is_open()) {
		printf("Error: Failed to open file %s", DEFAULT_INPUT_FILE);
		return -1;
	}
	auto fileSize = filesystem::file_size(DEFAULT_INPUT_FILE);
	auto sliceSize = fileSize / threadCount;

	ll betweenThreads = 0, storedStart = 0;										// Init variables for the loop below, including the vector that will store the threads.
	vector<thread> threadVec;

	for (int i = 1; i <= threadCount; i++) {									// For each Thread, we calculate the 'slice' of the file we want it to process.
		auto startPosition = sliceSize * i;
		inputFile.seekg(startPosition);										// We know the approximate starting point based on the thread we are on.

		string temp;												// We create a temporary string to read once to avoid
		inputFile >> temp;											// landing in the middle of a word/string.

		betweenThreads = inputFile.tellg();									// This new location is this thread's beginning and the previous thread's end.

		betweenThreads = (betweenThreads == -1) ? fileSize : betweenThreads;					// Make sure we aren't at the end of the file - fix if we are.
		printf("Thread %d: Starting at %lld and ending at %lld\n", i - 1, storedStart, betweenThreads);

		threadVec.push_back(thread(stringReaderThread, i - 1, threadCount, DEFAULT_INPUT_FILE, storedStart, betweenThreads)); // Spawns the 'previous' thread with this endpoint

		storedStart = betweenThreads;										// Store this value for the next iteration - it is the current thread's beginning.
	}

	ofstream outputFile;												// We open the output file to clear any previous content from it.
	outputFile.open(DEFAULT_OUTPUT_FILE);
	outputFile.close();

	for (auto& t : threadVec) {											// Spawns the Threads.
		t.join();
	}

	auto endTime = high_resolution_clock::now();									// Calculate how long the program took to run to completion.
	auto timeLapsed = duration_cast<milliseconds>(endTime - startTime);
	cout << "Program completed in " << timeLapsed.count() << "ms\n";

	return 0;
}
