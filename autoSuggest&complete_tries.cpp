/*
    Aurthor: labdhi Anand

    # AutoSuggestor and Completion for cross platform.
    # Auto Completion on TAB press.
    Tested in linux-> Manjaro and windows->11
    // Add any word you want to dictionary.txt file
*/

#include <iostream>
#include <fstream> //lets us read/write files (like dictionary.txt)
#include <string>
//following section is to check which OS is beign used and
#ifdef _WIN32
    #include <windows.h> //use windows.h for keyboard.
#elif __linux__ || __APPLE__
    #include <termios.h>//use termios and unistd for keyboard.
    #include <unistd.h> 
#endif 

#define ALPHABET_SIZE 60 //how big trie branches can be(letter&symbols)
#define MAX_SUGGESTION_SIZE 4 //how many sugesstions we show
#define DICTIONARY_FILE_NAME "dictionary.txt"

using namespace std; 
class TrieNode {
public:
    //The character at each level is given by the array index you picked at that level: so we dont needa store char explicityl
    TrieNode* children[ALPHABET_SIZE]; 
    bool isEndOfWord;
    TrieNode() {
        for (int i = 0; i < ALPHABET_SIZE; ++i) {
            children[i] = nullptr;
        }
        isEndOfWord = false;
    }
};

// helpers
int getArrayLength(const string*);
void clearScreen();

// Functioning Stuff
void insert(TrieNode*, string);
void possibleTextHelper(TrieNode*, string, string*, int&);
string* search(TrieNode* , const string&);

// Main Stuff
string* getSuggestions(TrieNode*, string, string*);
void InsertDictionary(TrieNode&, string);

//preprocessor conditional directive.
//tells compiler “Depending on what operating system this code is being compiled on, declare this function.”
#ifdef _WIN32 
    void windowsOperation(TrieNode&);
#endif

// Main Function
int main(){
    TrieNode autoCompletionNode;
    InsertDictionary(autoCompletionNode, DICTIONARY_FILE_NAME);

    // insert (&autoCompletionNode, "apple");
    // insert (&autoCompletionNode, "an");

    cout<< ">> ";
    #ifdef __WIN32
        windowsOperation(autoCompletionNode);
    #endif
    return 0;
}

// For Main Operation to take input
#ifdef _WIN32
void windowsOperation(TrieNode& autoCompletionNode){
    string userInput = "";
    // One character at a time setup
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); //“handle to standard input”.Think of it like: magic key you use to talk to the keyboard input stream.
    DWORD mode, numRead; //mode - number that remembers what the console settings currently are. Because we need to temporarily change the console into a special mode that lets us read keys one by one — but when we’re done, we want to put it back to how it was.
    //numRead - “How many input events did you actually read?” We only ask for 1 event at a time in this program.
    INPUT_RECORD input; //windows tells which key was just pressed by the user 


    string* result = new string[MAX_SUGGESTION_SIZE]; //allocayes memory to holds 4 suggestions
    while (69) { //basically like 'while true' - keep runninf infinitly
        string suggestionFormat = ""; //single string that we’ll use to format and display the suggestions nicely on the screen. Every time through the loop, we clear it ("") and build it up again.

        GetConsoleMode(hStdin, &mode); //save current console mode
        SetConsoleMode(hStdin, ENABLE_PROCESSED_INPUT); //enable special mode to process inout properly
        ReadConsoleInput(hStdin, &input, 1, &numRead); //read 1 key press into input
        SetConsoleMode(hStdin, mode); //restore og mode
        bool shiftKeyPressed = false; //keeps track of if shift key was pressed -> if yes then make letters uppercase
        
        if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) { //checks if input was a KEY_EVENT → a keyboard event && the otehr part gives eitehr true -> when key was pressed and false -> when key was released --> we only care if abotu the pressed down action
            if (input.Event.KeyEvent.wVirtualKeyCode == VK_TAB){ //.wVirtualKeyCode - no. that represents each key on keyboeard and VK_TAB virtual key code for TAB KEY
                userInput = result[0]; //if user pressed tab key, we replace userInput with first suggextion in result - autocomplete
            }
            else if (input.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT){
                shiftKeyPressed = true;
            }
            else if (input.Event.KeyEvent.wVirtualKeyCode == VK_BACK){ //if user pressed backsapce key
                userInput = userInput.substr(0, userInput.length()-1); //we remove last char of userinput
                if (userInput != ""){ //if teh userinput is still not emty we call getsuggestions( again to get updates suggestions for new userinput)
                    result = getSuggestions(&autoCompletionNode, userInput, &suggestionFormat);
                }
            }
            else {
                char inputChar = input.Event.KeyEvent.uChar.AsciiChar; //any otehr key - normal character
                inputChar = shiftKeyPressed? toupper(inputChar) : inputChar; //if shift was pressed earlier make this char uppercase
                shiftKeyPressed = false; //reset
                if (inputChar == '\r') { //if char is Enter key we exit loop
                    break;
                }
                userInput+=inputChar; //add new char to userinput

                //  autocomplete
                delete[] result; //delete old result ot free memory
                result = getSuggestions(&autoCompletionNode, userInput, &suggestionFormat); //get new suggestions
            }
            clearScreen();
            cout << ">> " << userInput << "\n" << suggestionFormat; 
            /* will output in the following format
            >> [what user typed]
                [suggestions separated by commas]
            */
        }
    }
    delete[] result; //ONCE ENTER PRESSED - LOOP ENDS AND DELETE result arr to free memory
}
#endif


// for available suggestion, max 3
string* getSuggestions(TrieNode* root, string key, string* suggestionFormat){
    string* result = search(root, key);
    const int arrSize = getArrayLength(result);
    for (int i=0; i< arrSize; i++){
        *suggestionFormat+= result[i] + ", ";
    }
    return result;
}

// Add list of words to TrieNode
void InsertDictionary(TrieNode& autoCompletionNode, string fileName){ //autoCOmpletion Node is root of trie 
    ifstream dictionary(fileName); //open file using ifstream and call it variable
    if (!dictionary.is_open()) { //chekc if file successfully opened
        cerr << "Failed to open the file! \n"; //cerr stands for: C-stream error output - standard output stream in C++ used specifically for printing error messages.
        exit(0);
    }
    string word;
    while (dictionary >> word) { //reads words from dictionary and one by one puts them into 'word' variable
        insert(&autoCompletionNode, word); //inserts into trie each word
    }
    if (!dictionary.eof() && !dictionary.good()) { //if we didnt reach end of file && teh stream wasnt good then close file
        cerr << "Error: could not read file \n";
        dictionary.close();
        exit(0);
    }
    dictionary.close(); //close file
}

// Insert word to TrieNode
void insert(TrieNode* root, string key){
    TrieNode* currentNode = root;
    for (char curChar: key){
        const int index = curChar - 'A';
        if (currentNode->children[index] == nullptr){
            currentNode->children[index] = new TrieNode();
        }
        currentNode = currentNode->children[index];
    }
    currentNode->isEndOfWord = true;
}

// get all PossibleText from TrieNode, Basically get upto 3(max) or any number under
void possibleTextHelper(TrieNode* node, string key, string* possibleTexts, int& idx){ //node- where in trie we are now, key - letters collected so far, possible Texts is to store suggestions, idx is current idx in possibleTexts 
    if (idx>=3 || node == nullptr ) return; //already have 3 suggesstions or if current node is null we stop
    if (node->isEndOfWord){ //if a full word is gotten - > save key in possibletests
        possibleTexts[idx++] = key; //idx++ to store next suggetsio
    }
    for (int i=0; i<ALPHABET_SIZE; i++){ //loop over children to find more words that are possible 
        if (node->children[i] != nullptr){
        char c = 'A'+i;
        possibleTextHelper(node->children[i], key+c, possibleTexts, idx);
        }
    }
}

// search for possible words, calls -> possibleTextHelper for so.
string* search(TrieNode* root, const string& key) { //searches for all possible completions for th prefix tyoed so far
    TrieNode* currentNode = root; //key is prefix
    for (char c : key) {
        int index = c - 'A';
        if (currentNode->children[index] == nullptr) { //no child at this idx - prefix dont exist in tree
            return new string[3](); //return an empty array of 3 empty strings
        }
        currentNode = currentNode->children[index];
    }
    string* possibleTexts = new string[MAX_SUGGESTION_SIZE];
    int idx = 0;
    possibleTextHelper(currentNode, key, possibleTexts, idx);
    return possibleTexts;
}

// Helper Functions.
int getArrayLength(const string* arrPtr) { //counrts non empty strings in arrPtr 
    /* arrPtr = { "AP", "APPLE", "APT", "" }:
     then getarraylwngth will return the number of non empty suggestions
    */
    int count = 0;
    while (arrPtr[count] != ""){
        count++;
    }
    return count;
}
void clearScreen(){
    cout << "\033[2J\033[1;1H"; // Ascii key to clear console. [Platform Independent]
}
