/**
 * This program creates a graph (similar to a Deterministic Finite Automata) containing
 * every word of the input file and uses it to quickly search for sub-words.
 *
 * @author Bruno Ribeiro
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <cstring>
#include <algorithm>


#define IS_VALID(x)                     \
    ( ( (x) >= 'A' && (x) <= 'Z' ) ||   \
      ( (x) >= 'a' && (x) <= 'z' ) )


#define DEBUG_PARSE      0
#define DEBUG_PROCESS    0


using namespace std;


/**
 * This class represents a node of the graph. The final graph resembles
 * a Deterministic Finite Automata (DFA).
 */
class Node
{
    public:
        Node();

        Node(
            const string &word );

        ~Node();

        /**
         * @brief Build the graph
         */
        void parse(
            const string &value,
            size_t position = 0 );

        /**
         * @brief Returns a boolean value indicating if the given word is made up
         * of words in the graph.
         */
        bool isCompoundWord(
            const string &word,
            set<string> *output ) const;

    private:
        /**
         * @brief If the current node is a terminal, this field stores the word
         * with which this node matches.
         */
        string word;

        /**
         * @brief Indicates if the current node is a terminal.
         */
        bool isTerminal;

        /**
         * @brief Pointers to 'next' nodes for each valid character ('a' to 'z').
         */
        Node *next[26];

        /**
         * @brief Returns a boolean value indicating if the given word is made up
         * of words in the graph.
         *
         * This function traverses the graph looking for sub-words which
         * the given word is composed. The return value is @c true only if every
         * sub-word of the given word is in the list.
         */
        bool isCompoundWord(
            const char *word,
            size_t position,
            const Node *root,
            set<string> *output ) const;
};


Node::Node() : isTerminal(false)
{
    memset(next, 0, sizeof(next));
}


Node::Node(
    const string &word ) : word(word), isTerminal(false)
{
    memset(next, 0, sizeof(next));
}


Node::~Node()
{
    for (size_t i = 0; i < 26; ++i)
        if (next[i] != NULL) delete next[i];
}


void Node::parse(
    const string &value,
    size_t position )
{
    char current = value[position];

    if (current < 'a' || current > 'z') return;

    int index = current - 97;

    if (next[index] == NULL)
        next[index] = new Node();

    if (value[position + 1] != 0)
    {
        #if (DEBUG_PARSE == 1)
        if (next[index]->isTerminal)
            std::cout << current << ' ';
        else
            std::cout << (char)(current + 32) << ' ';
        #endif
        next[index]->parse(value, position + 1);
    }
    else
    {
        next[index]->isTerminal = true;
        next[index]->word = value;
        #if (DEBUG_PARSE == 1)
        std::cout << current << " = " << next[index]->word << std::endl;
        #endif
    }
}


bool Node::isCompoundWord(
    const string &word,
    set<string> *output ) const
{
    return isCompoundWord(word.c_str(), 0, this, output);
}


bool Node::isCompoundWord(
    const char *value,
    size_t position,
    const Node *root,
    set<string> *output ) const
{
    bool result = false;
    char current = value[position];

    // if we reach the end of the string and the current node is not
    // the end of a word, the validation must fail
    if (current == 0)
    {
        if (output != NULL && isTerminal && word != value) output->insert(word);
        return isTerminal && word != value;
    }

    // if we have the corresponding next node, we try to continue
    if (next[ current - 97 ] != NULL)
        result = next[ current - 97 ]->isCompoundWord(value, position + 1, root, output);
    // if the previous path failed and we are at the end of a word, we must try find
    // the next subword from the root
    if (isTerminal && !result)
    {
        #if (DEBUG_PROCESS == 1)
        std::cout << "From root looking for " << current << std::endl;
        #endif
        result = root->isCompoundWord(value, position, root, output);
    }

    if (output != NULL && isTerminal) output->insert(word);

    return result;
}


vector<string> *main_loadWords(
    const string &fileName )
{
    vector<string> *words = NULL;

    ifstream input(fileName.c_str());
    if (input.is_open())
    {
        char line[128];
        words = new vector<string>();

        while (input.good())
        {
            input.getline(line, sizeof(line));
            // skips empty lines
            if (strlen(line) == 0) continue;

            // clean and validates the current line
            for (size_t i = 0, t = strlen(line); i < t; ++i)
            {
                if (!IS_VALID(line[i])) continue;

                if (line[i] < 'a')
                    line[i] += 32;
                if (line[i] == '\r' || line[i] == '\n')
                {
                    line[i] = 0;
                    break;
                }
            }

            words->push_back(line);
        }
        // ensures the word list is sorted
        std::sort(words->begin(), words->end());
    }


    return words;
}



int main( int argc, char **argv )
{
    if (argc != 2 && argc != 3)
    {
        std::cerr << "Usage: quiz <input> [ <output> ]\n\n"
            "<input>   File containing the words. Only ASCII characters accepted (words\n"
            "          with non-ASCII characters will be ignored).\n"
            "<output>  Optional output file where the program could save the list of all\n"
            "          words which are concatenations of other sub-words that exist in the\n"
            "          input file.\n\n";
        return 1;
    }

    clock_t dataTime = clock();

    // loads words from input file
    vector<string> *words = main_loadWords(argv[1]);
    if (words == NULL)
    {
        std::cerr << "Can not load words from '" << argv[1] << "'" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << words->size() << " words" << std::endl << std::endl;

    dataTime = (clock() - dataTime) / (CLOCKS_PER_SEC / 1000);


    clock_t processTime = clock();

    // creates the graph parsing each word
    Node root;
    vector<string>::iterator first = words->begin();
    vector<string>::iterator last = words->end();
    for (; first != last; ++first)
    {
        root.parse(*first);
    }

    // checks if the user wants to save the list of compound words
    ofstream *output = NULL;
    if (argc == 3)
    {
        output = new ofstream(argv[2]);
        if (!output->is_open())
        {
            delete output;
            output = NULL;
        }
    }

    // processes all words in order to discover which ones are compound
    string longest;
    size_t length = 0;
    first = words->begin();
    last = words->end();
    for (; first != last; ++first)
    {
        // the current word is composed of other words in the list?
        if (root.isCompoundWord(*first, NULL))
        {
            if (output != NULL)
                (*output) << *first << std::endl;
            // checks if the current word is the longest until now
            if ((*first).length() > length)
            {
                longest = *first;
                length = (*first).length();
            }
        }
    }

    processTime = (clock() - processTime) / (CLOCKS_PER_SEC / 1000);

    // prints the result
    std::cout << std::endl << "The longest compound word is '" << longest << "'" << std::endl << std::endl;
    std::cout << "Sub-words of '" << longest << "':" << std::endl << "    ";
    set<string> subWords;
    root.isCompoundWord(longest, &subWords);
    set<string>::iterator it = subWords.begin();
    for (; it != subWords.end(); ++it)
        std::cout << *it << ' ';
    std::cout << std::endl;

    // prints additional information
    std::cout << std::endl;
    std::cout << "Preparation time: " << dataTime << " ms" << std::endl;
    std::cout << " Processing time: " << processTime << " ms" << std::endl;

    if (words != NULL) delete words;
    if (output != NULL)
    {
        output->close();
        delete output;
    }
}
